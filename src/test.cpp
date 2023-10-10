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
#include "SoundCustomBufferRecorder.hpp"
#include "opusmanager.h"
#include "proto/protocol.pb.h"
#include "memusage.h"

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

  AES_GCM crpt(key, iv);

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

  PlayersManagerImpl::getInstance().setWaitAudioPackets(4);
  if(PlayersManagerImpl::getInstance().getWaitAudioPackets() != 4){
    fail("Wait Audio Packets", "players::waitQueueAudioCount" , "4", "players::setWaitAudioPackets");
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
  OpusManagerImpl::fabric(SAMPLE_RATE);

  int sampleSize = opusStream.getSampleCount();

  int finalLen = 0;
  int totalLen = 0;
  int totalSample = 0;
  int j = 0;
  for (int i = 0; i < audio_size - sampleSize; i += sampleSize)
  {

    data::buffer encBuffer = OpusManagerImpl::getInstance().encode((sf::Int16 *)audio + i, sampleSize);
    data::buffer decBuffer = OpusManagerImpl::getInstance().decode(encBuffer, sampleSize);
    totalLen += encBuffer.size() / sizeof(sf::Int16);
    totalSample += sampleSize;

    if(decBuffer.size() != sampleSize * sizeof(sf::Int16)){
      fail("buffer audio", "encBuffer/decBuffer", "valid size", "OpusManagerImpl::getInstance().encode/decode");
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
  PlayersManagerImpl::getInstance().setWaitAudioPackets(4);

  std::cout << "test listen audio packets" << std:: endl;

  player::SelfImpl::frabric(3, 3);

  int k = 0;
  std::vector<protocol::Server> protocolsBuffers;
  std::vector<std::pair<int, data::buffer>> noParse;
  for (int i = 0; i < audio_size - sampleSize; i += sampleSize)
  {
    data::buffer encBuffer = OpusManagerImpl::getInstance().encode((sf::Int16 *)audio + i, sampleSize);

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
    std::thread(&soundmanager::NetworkAudioStream::insert, &opusStream, cBuffer.first, std::ref(cBuffer.second)).detach();
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

  for(auto& cBuffer : protocolsBuffers){
    protocolParserImpl::getInstance().parse(cBuffer);
    for(int i = 4; i < 8; i++){
      cBuffer.set_id(i);
      protocolParserImpl::getInstance().parse(cBuffer);
    }
    data::preciseSleep(((double)SAMPLE_TIME_DEFAULT) / 1000.0);
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

void test_data_structure_memory_aux(data::buffer buffer){
  data::buffer data(buffer);
}

void test_data_structure_memory(){

  std::cout << "test memory" << std::endl;

  size_t lastSize = getCurrentRSS( );
  size_t peakSize    = getPeakRSS( );

  for(int k = 0; k < 60; k++){

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

    size_t currentSize = getCurrentRSS( );
    if(currentSize > lastSize + 50000){
      std::cout << "memory leak" << std::endl;
    } else {
      std::cout << "memory ok" << std::endl;
    }
  
}

int main(){

  /*
  PLAYER player = std::make_shared<Player>();
  player->id = 999;
  data::PlayerThread_BinaryTree test(player);

  PLAYER playerA = std::make_shared<Player>();
  playerA->id = 1;
  PLAYER playerB = std::make_shared<Player>();
  playerB->id = 2;
  PLAYER playerC = std::make_shared<Player>();
  playerC->id = 3;
  PLAYER playerD = std::make_shared<Player>();
  playerD->id = 4;
  PLAYER playerE= std::make_shared<Player>();
  playerE->id = 5;
  PLAYER playerF = std::make_shared<Player>();
  playerF->id = 6;

  test.insert(playerA, 1);
  test.insert(playerB, 2);
  test.insert(playerC, 3);
  test.insert(playerD, 4);
  test.insert(playerE, 5);
  test.insert(playerF, 6);

  data::PlayerThread_BinaryTree* rec = test.search(3);

  if(rec != nullptr){
    PLAYER rec3 = rec->getPlayer();

    if(rec3->id == playerC->id){
      std::cout << "apppp" << std::endl;
    }
  } else {
    std::cout << "aEEEE" << std::endl;
  }
  */

  testCrypt();

  testPlayers(); //parcial

  test_BufferParser_listen(); //parcial

  return 0;

  test_data_structure_memory();

  return 1;

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
}