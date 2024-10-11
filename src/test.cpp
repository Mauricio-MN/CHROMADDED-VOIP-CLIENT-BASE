#define DEBUG_MODE_LIB true

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <memory>
#include <opus/opus.h>
#include "protoParse.h"
#include "crmd.h"
#include "crpt.h"
#include "player.h"
#include "socketUdp.h"
#include "soundmanager.h"
#include "soundmanagerRecorder.h"
#include "opusmanager.h"
#include "proto/protocol.pb.h"
#include "memusage.h"
#include <SFML/Window/Keyboard.hpp>

bool sucess = true;
void sucessMSG(const char* module, const char* testFunction){
  if(sucess){
    std::cout << "Test passed on module: " << module << " | function: " << testFunction;
    std::cout << std::endl << std::endl;
  }
}

void fail(char* errorInfo, char* variable, char* expected, char* module){
  std::cout << "Error on " << errorInfo << 
  " | var: " << variable << " , expected var: " << expected << 
  " | on module: " << module;
  std::cout << std::endl << std::endl;
  sucess = false;
}

void fail(const char* errorInfo, const char* variable, const char* expected, const char* module){
  std::cout << "Error on " << errorInfo << 
  " | var: " << variable << " , expected var: " << expected << 
  " | on module: " << module;
  std::cout << std::endl << std::endl;
  sucess = false;
}

void testCrypt(){

  unsigned char* key = (unsigned char*)strdup("abcdefghijklmnopqrsasdry764jg651");
  unsigned char* iv = (unsigned char*)strdup("ajfrtyprewqsdrgt");

  ConfigImpl::getInstance().setCryptoKey(std::vector<char>(key, key + 32));
  ConfigImpl::getInstance().setCryptoIV(std::vector<char>(iv, iv + 16));

  AES_GCM crpt(ConfigImpl::getInstance().getCryptoKey(), ConfigImpl::getInstance().getCryptoIV());

  char* text = new char[35];
  text = strdup("o rato roeu a roupa do rei de roma");
  std::vector<unsigned char> textUchar(text,text+35);
  std::vector<unsigned char> encrypted = crpt.encrypt(textUchar);


  int encdata_size = encrypted.size();
  for(int i = 0; i < encdata_size; i++){
    std::cout << (int)encrypted.at(i);
  }
  std::cout << std::endl;


  std::vector<unsigned char> decrypted = crpt.decrypt(encrypted);
  std::cout << decrypted.data() << std::endl;

  char* finaltext = new char[decrypted.size() + 1];
  std::copy(decrypted.begin(), decrypted.end(), finaltext);

  if(strcmp( finaltext, text) != 0){
    fail("encode/decode(crypt)", "finaltext", "normal text", "crypt::getInstance()->");
  }

  sucessMSG("AES_GCM", "testCrypt");
}

void testPlayers(){
  
  PlayersManagerImpl::getInstance().insertPlayer(1,2,3,4);

  if(PlayersManagerImpl::getInstance().existPlayer(1) == false){
    fail("Player exist", "*" , "true", "PlayersManagerImpl::getInstance().insertPlayer/existPlayer");
  }

  PLAYER actualPlayer = PlayersManagerImpl::getInstance().getPlayer(1);

  if(actualPlayer.get() == nullptr){
    fail("Get Player", "*" , "Object", "PlayersManagerImpl::getInstance().getPlayer");
  }

  float X = actualPlayer->getPosition().x;
  float Y = actualPlayer->getPosition().y;
  float Z = actualPlayer->getPosition().z;

  if(X != 2 || Y != 3 || Z != 4){
    fail("Player position", "actualPlayer->getPosition()" , "{2,3,4}", "PlayersManagerImpl::getInstance().getPlayer / getPosition()");
  }

  actualPlayer->setPosition(1,2,3);

  X = actualPlayer->getPosition().x;
  Y = actualPlayer->getPosition().y;
  Z = actualPlayer->getPosition().z;

  if(X != 1 || Y != 2 || Z != 3){
    fail("Player position", "actualPlayer->soundStream->getPosition()" , "{1,2,3}", "PlayersManagerImpl::getInstance().move");
  }

  PlayersManagerImpl::getInstance().removePlayer(1);

  if(PlayersManagerImpl::getInstance().existPlayer(1) == true){
    fail("Player remove", "*" , "false", "PlayersManagerImpl::getInstance().removePlayer / existPlayer");
  }

  sucessMSG("players::", "testPlayers");
}

void waitListenSoundStream(soundmanager::NetworkAudioStream &stream){
  std::cout << "listen start, tap X to jump." << std::endl;

  while (stream.getStatus() == soundmanager::NetworkAudioStream::Playing)
  {
    sf::sleep(sf::milliseconds(1));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
    {
      stream.stop();
      continue;
    }
  }
}

void test_BufferParser_listen(){

  soundmanager::listener::movePos(0,0,0);
  std::cout << "recording..." << std::endl;
  sf::SoundBuffer sbuffer = soundmanager::RecorderImpl::getInstance().recordForTest();
  std::cout << "recorded" << std::endl;

  const sf::Int16* audio = sbuffer.getSamples();
  int audio_size = sbuffer.getSampleCount();

  PlayersManagerImpl::getInstance().insertPlayer(3,1,1,1);

  std::cout << "test opus encode/decode" << std::endl;
  soundmanager::NetworkAudioStream opusStream(sf::milliseconds(SAMPLE_TIME_DEFAULT), SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
  OpusManager opusManager(SAMPLE_RATE);

  int sampleSize = sampleTimeGetCount(sf::milliseconds(SAMPLE_TIME_DEFAULT), SAMPLE_RATE);

  int finalLen = 0;
  int totalLen = 0;
  int totalSample = 0;
  int j = 0;
  for (int i = 0; i < audio_size - sampleSize; i += sampleSize)
  {

    data::buffer encBuffer = opusManager.encode((sf::Int16 *)audio + i, sampleSize);
    data::buffer decBuffer = opusManager.decode(encBuffer, sampleSize);
    totalLen += encBuffer.size() / sizeof(sf::Int16);
    totalSample += sampleSize;

    if(decBuffer.size() != sampleSize * sizeof(sf::Int16)){
      fail("buffer audio", "encBuffer/decBuffer", "valid size", "opusManager.encode/decode");
      break;
    }

    if (finalLen == 0){ finalLen = encBuffer.size();
    } else { finalLen = (finalLen + encBuffer.size()) / 2; }

    //opusStream.insert(j, decBuffer);

    j++;
    if(j >= 256){
      j = 0;
    }
  }

  //opusStream.play();

  std::cout << "original len SAMPLES: " << sampleSize << std::endl;
  std::cout << "encoded  len average: " << finalLen << std::endl;
  std::cout << "original len TOTAL  : " << totalSample << std::endl;
  std::cout << "encoded  len TOTAL  : " << totalLen << std::endl;

  //waitListenSoundStream(opusStream);

  std::cout << "opus listen end." << std::endl;

  soundmanager::listener::movePos(1,1,1);
  PLAYER actPlayer = PlayersManagerImpl::getInstance().getPlayer(3);
  actPlayer->setPosition(1,1,1);

  std::cout << "test listen audio packets" << std:: endl;

  int k = 0;
  std::vector<protocol::Server> protocolsBuffers;
  std::vector<std::pair<int, data::buffer>> noParse;
  for (int i = 0; i < audio_size - sampleSize; i += sampleSize)
  {
    data::buffer encBuffer = opusManager.encode((sf::Int16 *)audio + i, sampleSize);

    protocol::Server cBuffer;

    cBuffer.set_id(3);
    cBuffer.set_audio(encBuffer.getData(), encBuffer.size());
    cBuffer.set_audionum(k);
    cBuffer.set_sampletime(SAMPLE_TIME_DEFAULT);

    protocolsBuffers.push_back(cBuffer);
    noParse.push_back(std::pair<int, data::buffer>(k, data::buffer((sf::Int16 *)audio + i, sampleSize)));

    k++;
    if(k >= 256) k = 0;
  }

  std::cout << "NO PARSE audio test, no player" << std::endl;

  sf::Clock clock;
  for(auto& cBuffer : noParse){
    std::thread(&soundmanager::NetworkAudioStream::insert, &opusStream, cBuffer.first, std::ref(cBuffer.second), SAMPLE_TIME_DEFAULT).detach();
    data::preciseSleep(((double)SAMPLE_TIME_DEFAULT) / 1000.0);
  }
  std::cout << "Time process " << clock.getElapsedTime().asMilliseconds() << std::endl;
  std::cout << "Time total   " << (noParse.size() * SAMPLE_TIME_DEFAULT) << std::endl;

  std::cout << "listen ending, tap X to jump." << std::endl;

  while(true){
    sf::sleep(sf::milliseconds(1));
    std::this_thread::yield();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      opusStream.stop();
      break;
    }
  }

  std::cout << "PARSE audio test" << std::endl;

  ProtocolParser protoParser;

  for(auto& cBuffer : protocolsBuffers){
    clock.restart();
    protoParser.parse(cBuffer);
    for(int i = 4; i < 8; i++){
      cBuffer.set_id(i);
      protoParser.parse(cBuffer);
    }
    int elapsed = clock.getElapsedTime().asMilliseconds();
    double sleepTime = ((double)SAMPLE_TIME_DEFAULT - (double) elapsed) / 1000.0;
    if(sleepTime < 0) sleepTime = 0;
    data::preciseSleep(sleepTime);
  }

  std::cout << "listen ending, tap X to jump." << std::endl;

  while(true){
    sf::sleep(sf::milliseconds(1));
    std::this_thread::yield();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      actPlayer->stop();
      break;
    }
  }

  std::cout << "NO PARSE audio test" << std::endl;

  for(auto& cBuffer : noParse){
    sf::Clock clock;
    //std::thread(&Player::push, actPlayer, cBuffer.first, std::ref(cBuffer.second), SAMPLE_TIME_DEFAULT).detach();
    actPlayer->push(cBuffer.first, cBuffer.second, SAMPLE_TIME_DEFAULT);
    data::preciseSleep(((double)SAMPLE_TIME_DEFAULT) / 1000.0);
    std::cout << "T : " << clock.getElapsedTime().asMilliseconds() << std::endl;
  }

  std::cout << "listen ending, tap X to jump." << std::endl;

  while(true){
    sf::sleep(sf::milliseconds(1));
    std::this_thread::yield();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      actPlayer->stop();
      break;
    }
  }

  std::cout << "listen ending, tap X to jump." << std::endl;

  while(actPlayer->isPlaying()){
    sf::sleep(sf::milliseconds(1));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      actPlayer->stop();
      continue;
    }
  }

  std::cout << "listening test ended. audio ok? (Y,N)";
  char* defaultCharExp = strdup("B");
  char resp = defaultCharExp[0];
  delete []defaultCharExp;
  char* expectd = strdup("YNyn");
  while(resp != expectd[0] && resp != expectd[1] && resp != expectd[2] && resp != expectd[3]){
    std::cin >> resp;
  }
  if(resp == expectd[0] || resp == expectd[2]){
    sucessMSG("bufferParser+listen", "test_BufferParser_listen");
  }else{
    fail("Listen audio packets", "NaN" , "Audio Noise PCM", "bufferParser+listen+players::manager+player Structure");
  }

}

void test_parser_protocol(){

  OpusManager opusManager(SAMPLE_RATE);
  std::cout << "Protocol parse" << std::endl;

  int an = 0;

  ProtocolParser parser;

  an = 0;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        protocol::Server server;
        std::vector<sf::Int16> audio(320);
        data::buffer encoded = opusManager.encode(audio.data(), audio.size());
        server.set_id(10);
        server.set_audio(encoded.getData(), audio.size());
        server.set_audionum(an);
        parser.parse(server);
        an++;
        if(an >= 256) an = 0;
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

}

void test_data_structure_memory_aux(data::buffer buffer){
  data::buffer data(buffer);
}

void test_data_structure_memory(){

  std::cout << "test memory" << std::endl;

  size_t lastSize = getCurrentRSS( );
  size_t peakSize    = getPeakRSS( );

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){

        char* testBuffer = new char[100];
        data::buffer bufferA(testBuffer, 100);
        delete[] testBuffer;
        data::buffer bufferB(100);
        data::buffer bufferC(bufferA);
        data::buffer bufferD(&bufferB);
        test_data_structure_memory_aux(bufferD);
      }

        sf::sleep(sf::milliseconds(1));

      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  test_parser_protocol();

  std::cout << "Check server" << std::endl;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        protocol::Server server;
        data::buffer audio(320);
        server.set_id(10);
        server.set_audio(audio.getData(), audio.size());
        server.clear_audio();
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  test_parser_protocol();

  std::cout << "Check queue" << std::endl;

  data::asyncQueue queue;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        protocol::Server server;
        std::vector<sf::Int16> audio(320);
        server.set_id(10);
        server.set_audio(audio.data(), audio.size());
        queue.push(server);
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  std::cout << "Check queue pop" << std::endl;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        bool success = false;
        protocol::Server server = queue.pop(success); 
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  test_parser_protocol();

  std::cout << "Audio queue" << std::endl;

  int an = 0;

  data::AudioQueue audQueue;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        std::vector<sf::Int16> audio(320);
        audQueue.push(an, audio);
        an++;
        if(an >= 256) an = 0;
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  std::cout << "Audio queue pop" << std::endl;

  for(int k = 0; k < 3; k++){

    for(int i = 0; i < 1000; i++){

      for(int j = 0; j < 10; j++){
        std::vector<sf::Int16> audio(320);
        audQueue.pop(audio);
      }
        sf::sleep(sf::milliseconds(1));
      }

      std::cout << "memory check: " << getCurrentRSS() << std::endl;
  }

  std::cout << "memory check: " << getCurrentRSS() << std::endl;

  test_parser_protocol();

  std::cout << "..." << std::endl;

    size_t currentSize = getCurrentRSS( );
    if(currentSize > lastSize + 50000){
      std::cout << "memory leak" << std::endl;
    } else {
      std::cout << "memory ok" << std::endl;
    }
  
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int main(int argc, char *argv[]){

  //geral test -> use debug audio in server
  /*
set iv 123456789012
remove player 3
remove player 4
remove player 5
add player 2 3 12345678901234567890123456789012 PadinBK
add player 1 4 12345678901234567890123456789012 MandyFairy
add player 3 5 12345678901234567890123456789012 Uchoa

  */
  unsigned char *key = new unsigned char[32];
  strcpy( (char*) key, "12345678901234567890123456789012" );
  
  int myID = 3;
  int regID = 2;

  std::string ip("127.0.0.1");

  if(argc > 1){
    if(argv[1][0] == "1"[0]){
        myID = 4;
        regID = 1;
        ip = "192.168.0.114";
    }
    if(argv[1][0] == "2"[0]){
        myID = 5;
        regID = 3;
        ip = "149.56.67.2";
    }
    if(argv[1][0] == "3"[0]){
      ip = "149.56.67.2";
    }
    int arg = 0;
    if(is_number(std::string(argv[1]))){
      arg = std::stoi(std::string(argv[1]));
      if(arg >= 6){
        ip = "149.56.67.2";
        myID = arg;
        regID = arg;
      }
    }
  }

  CRMD_SessionDTO mySession = {0};

  mySession.id = myID;
  mySession.secret_id = regID;
  mySession.hostname = ip.data();
  mySession.hostname_size = ip.size();
  mySession.port = 443;
  mySession.key = key;
  mySession.map = 0;
  mySession.x = 0;
  mySession.y = 0;
  mySession.z = 0;
  mySession.needEncrypt = true;
  mySession.needEffects = true;

  CRMD_init(mySession);
  std::cout << "recording";
  
  //CRMD_setListenRecAudio(true);
  CRMD_enableRecAudio();
  CRMD_enableSendAudio();
  CRMD_setEcho(true, 0.15, 8000);
  CRMD_enableAutoDetect();
  //CRMD_setAutoDetect(0.48f);
  CRMD_setMicVolume(4.5f);
  //CRMD_enableReverb(1.0f, 1.0f, 0.32f, 0.89f, 1.49f, 0.83f);
  while(true){
    CRMD_updateMyPos(0, 143.0, 126.0, 0.0);
    CRMD_updateMyDir(1.0f, 1.0f, 0.0f);
    CRMD_sendMyPos();
    CRMD_setUpVector(0.0f, 0.0f, 1.0f);
    //CRMD_enablePlayerEchoEffect(9000, 3, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    if(CRMD_needCallNewSession()){
      CRMD_init(mySession);
    }
    uint32_t* players = new uint32_t[400];
		uint32_t outsize = 0;
		CRMD_getAllPlayerTalking(players, 400, &outsize);
    delete[] players;
  }
  
  CRMD_disableRecAudio();

  return 0;

  testCrypt();

  player::SelfImpl::fabric(3, 3);

  PlayersManagerImpl::getInstance();

  testPlayers(); //parcial

  test_BufferParser_listen(); //parcial

  for(int i = 0; i < 3; i++){
    test_data_structure_memory();
  }

  return 0;

  /*
  //geral test -> use debug audio in server
  unsigned char *key = new unsigned char[32];
  strcpy( (char*) key, "abcdefghijklmnopqsrtuvwxyz123456" );
  
  int myID = 0;
  int regID = 123;

  std::string ip("127.0.0.1");

  CRMD_init(regID, myID, ip.data(), ip.size(), 443, key, 0,0,0, true);
  std::cout << "recording";
  CRMD_enableRecAudio();
  sleep(3000);
  CRMD_disableRecAudio();
  */
}