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
#include "crmd.h"
#include "cript.h"
#include "player.h"
#include "protoParse.h"
#include "socketUdp.h"
#include "soundmanager.h"
#include "SoundCustomBufferRecorder.hpp"
#include "opusmanager.h"

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

  unsigned char* key = (unsigned char*)strdup("abcdefghijklmnopqrs");

  CryptImpl::fabric(key);

  char* text = new char[35];
  text = strdup("o rato roeu a roupa do rei de roma");
  std::vector<unsigned char> textUchar(text,text+35);
  std::vector<unsigned char> encrypted = CryptImpl::getInstance().encrypt(&textUchar);


  int encdata_size = encrypted.size();
  for(int i = 0; i < encdata_size; i++){
    std::cout << (int)encrypted.at(i);
  }
  std::cout << std::endl;


  std::vector<unsigned char> decrypted = CryptImpl::getInstance().decrypt(&encrypted);
  std::cout << decrypted.data() << std::endl;

  char* finaltext = new char[decrypted.size() + 1];
  std::copy(decrypted.begin(), decrypted.end(), finaltext);

  if(strcmp( finaltext, text) != 0){
    fail("encode/decode(crypt)", "finaltext", "normal text", "crypt::getInstance()->");
  }

  sucessMSG("crypt::getInstance()->", "testCrypt");
}

void testPlayers(){
  
  PlayersManagerImpl::getInstance().insertPlayer(1,2,3,4);

  if(PlayersManagerImpl::getInstance().existPlayer(1) == false){
    fail("Player exist", "*" , "true", "PlayersManagerImpl::getInstance().insertPlayer/existPlayer");
  }

  PLAYER actualPlayer = PlayersManagerImpl::getInstance().getPlayer(1);

  float X = actualPlayer->soundStream->getPosition().x;
  float Y = actualPlayer->soundStream->getPosition().y;
  float Z = actualPlayer->soundStream->getPosition().z;

  if(X != 2 || Y != 3 || Z != 4){
    fail("Player position", "actualPlayer->soundStream->getPosition()" , "{2,3,4}", "PlayersManagerImpl::getInstance().getPlayer / ->soundStream.getPosition()");
  }

  actualPlayer->move(1,2,3);

  X = actualPlayer->soundStream->getPosition().x;
  Y = actualPlayer->soundStream->getPosition().y;
  Z = actualPlayer->soundStream->getPosition().z;

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
      stream.stopEndclear();
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

  //new

  data::buffer bufferAudio;

  bufferAudio.insertArray((sf::Int16 *)audio, audio_size);
  int bufferAudio_size = bufferAudio.size();

  PlayersManagerImpl::getInstance().insertPlayer(3,1,1,1);

                                      //header    //id     //number  //sound
  char* packetAudioHeader = new char[6]{1,          3,0,0,0, 3};
  int   packetAudioHeader_size = 6;

  data::buffer packetAudio;
  packetAudio.insertArray(packetAudioHeader, 6);
  packetAudio.insert(bufferAudio);

  protocol::rcv_Audio_stc check_stc = protocol::fromvoipserver::constructRCVaudioData(packetAudio);

  int isProblemInSTC = false;
  for(int i = 0; i < (int)check_stc.audioData.size() ; i++){
    if(check_stc.audioData.getData()[i] != bufferAudio.getData()[i]){
      isProblemInSTC = true;
      break;
    }
  }

  if(isProblemInSTC){
    fail("Problem in buffer audio", "check_stc", "normal data", "data::buffer||//tools || ProtocolInfo::rcv_Audio_stc");
  }

  int samplesCount = bufferAudio.size() / sizeof(sf::Int16);
  sf::Int16 *bufferSamples = new sf::Int16[samplesCount];
  protocol::tools::bufferToData(bufferSamples, bufferAudio.size(), bufferAudio.getData());

  std::cout << "test opus encode/decode" << std::endl;

  soundmanager::NetworkAudioStream opusStream;

  int finalLen = 0;
  for (int i = 0; i < audio_size - 640; i += 640)
  {

    data::buffer encBuffer = OpusManagerImpl::getInstance().encode((sf::Int16 *)audio + i);
    data::buffer decBuffer = OpusManagerImpl::getInstance().decode(encBuffer);

    if(decBuffer.size() != 640 * sizeof(sf::Int16)){
      fail("Problem in buffer audio", "encBuffer/decBuffer", "valid size", "OpusManagerImpl::getInstance().encode/decode");
      break;
    }

    if (finalLen == 0){ finalLen = encBuffer.size();
    } else { finalLen = (finalLen + encBuffer.size()) / 2; }

    sbuffer.loadFromSamples(reinterpret_cast<sf::Int16*>((char*)decBuffer.getData()), decBuffer.size() / sizeof(sf::Int16),
      SAMPLE_CHANNELS, SAMPLE_RATE);
    opusStream.insert(sbuffer);
  }

  opusStream.play();

  std::cout << "original len        : " << bufferAudio.size() << std::endl;
  std::cout << "encoded  len average: " << finalLen << std::endl;

  waitListenSoundStream(opusStream);

  std::cout << "opus listen end." << std::endl;

  soundmanager::listener::movePos(1,1,1);
  PLAYER actPlayer = PlayersManagerImpl::getInstance().getPlayer(3);
  PlayersManagerImpl::getInstance().setWaitAudioPackets(4);

  std::cout << "test listen audio packets" << std:: endl;

  for(int i = 0; i < samplesCount - 640; i += 640){
    data::buffer encBuffer = OpusManagerImpl::getInstance().encode(bufferSamples + i);
    data::buffer ptcData(encBuffer.size() + 6);

    protocol::tools::mergeDatasArray(packetAudioHeader, packetAudioHeader_size, (char*)encBuffer.getData(), encBuffer.size(), (char*)ptcData.getData());
    //bufferparser::getInstance()->parser(&ptcData);
    BufferParserImpl::getInstance().parserBuffer(&ptcData);

    sf::sleep(sf::milliseconds(38));
  }

  std::cout << "listen start, tap X to jump." << std::endl;

  while(actPlayer->isPlaying()){
    sf::sleep(sf::milliseconds(1));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      actPlayer->soundStream->stopEndclear();
      continue;
    }
  }
  
  
  std::cout << "test listen auido packets end" << std:: endl;
  
  std::cout << "test connection packets:" << std:: endl;
  
  FakeServerSocket::init();
  
  CONN_DEBUG_IP_LOCAL_DECLARE
  ConnectionImpl::fabric(reinterpret_cast<char*>(CONN_DEBUG_IP_LOCAL), CONN_DEBUG_PORT);
  ConnectionImpl::getInstance();

  player::SelfImpl::frabric(3, 3, true);

  for(int i = 0; i < samplesCount - 640; i = i+640){
    data::buffer encBuffer = OpusManagerImpl::getInstance().encode(bufferSamples + i);
    data::buffer ptcData;

    ptcData.insert(packetAudioHeader, packetAudioHeader_size);
    ptcData.insert(encBuffer);

    ConnectionImpl::getInstance().send(ptcData, player::SelfImpl::getInstance().needEncrypt());

    sf::sleep(sf::milliseconds(38));
  }

  std::cout << "listen start, tap X to jump." << std::endl;

  while(actPlayer->isPlaying()){
    sf::sleep(sf::milliseconds(1));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
      actPlayer->soundStream->stopEndclear();
      continue;
    }
  }

  std::cout << "test connection packets end" << std:: endl;

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

        
        data::Audio_player_data playerDataA(&bufferD);
        data::Audio_player_data playerDataB(playerDataA);
        data::Audio_player_data playerDataC;
        playerDataC.init(playerDataB);

        
        protocol::rcv_Audio_stc audioStcA(100);
        protocol::rcv_Audio_stc audioStcB(audioStcA);
        protocol::rcv_Audio_stc audioStcC(&audioStcB);

        protocol::rcv_HandChacke_stc handStcA;
        handStcA.id = 10;
        

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

  testProtocolTools();

  testProtocol();

  testCrypt();

  testPlayers(); //parcial

  test_BufferParser_listen(); //parcial

  test_data_structure_memory();

  return 1;

  //geral test -> use debug audio in server
  unsigned char *key = new unsigned char[16];
  strcpy( (char*) key, "abcdefghijklmnop" );

  CONN_DEBUG_IP_LOCAL_DECLARE;
  
  int myID = 0;
  int regID = 123;

  crmd::init(regID, myID, CONN_DEBUG_IP_LOCAL, 443, key, 0,0,0, true);
  if(!crmd::initialized){
    fail("try initialize lib", "crmd::initialized", "true", "crmd::init()");
  }
  std::cout << "recording";
  crmd::enableRecAudio();
  sleep(3000);
  crmd::disableRecAudio();
}