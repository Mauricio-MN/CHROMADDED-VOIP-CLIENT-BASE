
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <memory>
#include <future>

#include "osimports.h"

#include "player.h"
#include "data.h"
#include "socketUdp.h"

struct Task {
    std::function<void(protocol::Server&)> function;
    std::shared_ptr<protocol::Server> server;

    bool valid;

    Task(){
      valid = false;
    }

    Task(std::function<void(protocol::Server&)> f, std::shared_ptr<protocol::Server> s)
        : function(std::move(f)), server(s) {
          valid = true;
        }

    void run(){
      if(valid){
        function(*server.get());
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
};

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {

        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this,i] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        if (!tasks.empty()){
                          task = std::move(tasks.front());
                          tasks.pop();
                        } else {
                          if(stop){
                            return;
                          }
                        }

                    }
                    task.run();
                }
            });
        }
    }

    void enqueue(std::function<void(protocol::Server&)> f, std::shared_ptr<protocol::Server> s) {
        auto task = Task(f, s);

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::move(task));
        }

        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<Task> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

std::vector<std::atomic_bool> lossInfo(256);
std::atomic_int lastLossInfo(0);
std::atomic_int lastLossPerc(0);
std::atomic_int lastPing(0);
std::atomic_int medianPing(0);
std::atomic_int medianTrotling(0);

int ProtocolParser::getTrotling(){
  return medianTrotling.load();
}

int ProtocolParser::getLossPercent(){
  return lastLossPerc.load();
}

void updateSoundProcessingTimes(int ping, int initAudioDelay){
  if(ping >= 70){
      soundmanager::RecorderImpl::getInstance().setIntervalTime(100);
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay);
    } else if(ping >= 60){
      soundmanager::RecorderImpl::getInstance().setIntervalTime(60);
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay);
    } else if (ping >= 25){
      soundmanager::RecorderImpl::getInstance().setIntervalTime(40);
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay+25);
    } else {
      soundmanager::RecorderImpl::getInstance().setIntervalTime(40);
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay+20);
    }
}

void parseFunction(protocol::Server& serverReceived){

  if(serverReceived.has_invalidsession()){
    if(serverReceived.invalidsession()){
      ProtocolParser::invalidCalsFromServer += 1;
      if(ProtocolParser::invalidCalsFromServer.load() > 2){
        socketUdpImpl::getInstance().close();
        ProtocolParser::invalidCalsFromServer = 0;
        medianPing = 0;
        return;
      }
    }
  }



  //Packet inside packet and DUMP network loss controll
  if(serverReceived.extraclientmsg_size() > 0){
    int sampleTotalTime = 0;
    for(int i = 0; i < serverReceived.extraclientmsg_size(); i++){
      protocol::Server extraServer;
      auto bufferExtraRef = serverReceived.mutable_extraclientmsg();
      auto bufferExtra = bufferExtraRef->at(i);
      bool isParsed = extraServer.ParseFromArray(bufferExtra.data(), bufferExtra.size());
      if(isParsed){
        if(extraServer.has_sampletime()){
          sampleTotalTime += extraServer.sampletime();
        }
        parseFunction(extraServer);
      }
    }
    if(serverReceived.has_audionum()){
      if(serverReceived.audionum() < 20 && lastLossInfo >= 240){
        int lossPercentage = 0;
        int lossCount = 0;
        for(auto& info : lossInfo){
          if(!info) lossCount++;
          info = false;
        }
        lossPercentage = lossCount * 100 / 256;
        if(lastLossPerc > 0){
          lossPercentage = (lossPercentage + lastLossPerc) / 2;
        }
        lastLossPerc = lossPercentage;
      }
      lossInfo[serverReceived.audionum()] = true;
      lastLossInfo = serverReceived.audionum();
    }
  }

  if(serverReceived.has_packettime() && serverReceived.handshake() == false){
    using namespace std::chrono;

    google::protobuf::Timestamp* PacketTimestamp = serverReceived.mutable_packettime();
    google::protobuf::Timestamp nowTimestamp;
    high_resolution_clock::time_point timePoint = high_resolution_clock::now();

    // Converta os tempos para um formato comum
    system_clock::time_point serverTimePoint = time_point_cast<system_clock::time_point::duration>(system_clock::time_point(seconds(PacketTimestamp->seconds()) + nanoseconds(PacketTimestamp->nanos())));
    system_clock::time_point currentTimePoint = time_point_cast<system_clock::duration>(timePoint);
    
    //server no passado
    if(ProtocolParser::isNegativeTimeDiffServer){
      currentTimePoint -= ProtocolParser::getTimeDiffServer();
    } else {
      //server no futuro
      currentTimePoint += ProtocolParser::getTimeDiffServer();
    }

    auto millis = duration_cast<milliseconds>(currentTimePoint - serverTimePoint).count();
    if(millis == 0) millis = 1;
    if(millis < 0){
      millis = 1;
      //recalcule time server/client
      player::SelfImpl::getInstance().sendConnect();
    }

    if(lastPing == 0) lastPing = millis;
    int trotling = lastPing - millis;
    if(trotling < 0) trotling = trotling * -1;
    medianTrotling = (medianTrotling + trotling) / 2;
    trotling = medianTrotling.load();
    lastPing = millis;
    //std::cout << "TT: " << trotling << std::endl;

    medianPing = ((medianPing.load() * 4) + millis) / 5;
    std::cout << "Ping: " << medianPing << std::endl;
    millis = medianPing;
    int initAudioDelay = millis + (millis*10/100) + trotling; // millis + 10% + trotling
    
    updateSoundProcessingTimes((trotling + millis) / 3, initAudioDelay);
    
  }

  if(!serverReceived.has_handshake() || serverReceived.handshake() == false){
    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
      std::cout << "Player " << serverReceived.id() << " not exist, Added" << std::endl;
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.has_sampletime()){
        sampleTime = serverReceived.sampletime();
      }
      PLAYER player(new Player(sf::milliseconds(sampleTime)));
      player->id = serverReceived.id();
      player->setPosition(0,0,0);
      PlayersManagerImpl::getInstance().insertPlayer(player);

    }

    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
      std::cout << "Player " << serverReceived.id() << " not exist" << std::endl;
      return;
    }

    PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());

    SimpleCoords pos = player->getPosition();
    bool altCoord = false;
    if(serverReceived.has_coordx()){
        pos.x = serverReceived.coordx();
        altCoord = true;
    }
    if(serverReceived.has_coordy()){
        pos.y = serverReceived.coordy();
        altCoord = true;
    }
    if(serverReceived.has_coordz()){
        pos.z = serverReceived.coordz();
        altCoord = true;
    }
    if(serverReceived.has_isgroup()){
      if(serverReceived.isgroup()){
        player->setToListenGroup(true);
        altCoord = false;
      } else {
        player->setToListenGroup(false);
      }
    }
    if(altCoord){
      player->setPosition(pos.x,pos.y,pos.z);
    }

    if(serverReceived.has_audio()){
        int audioNum = 0;
        int sampleTime = SAMPLE_TIME_DEFAULT;
        if(serverReceived.has_audionum()){
          audioNum = serverReceived.audionum();
        }
        if(serverReceived.has_sampletime()){
          sampleTime = serverReceived.sampletime();
        }

        int sampleCount = sampleTimeGetCount(sampleTime, SAMPLE_RATE);
        data::buffer audio(serverReceived.audio().data(), serverReceived.audio().size());

        player->push(audioNum, audio, sampleTime);
        //std::cout << "Player " << player->id << "push audio: " << sampleTime << " ms " << audio.size() << " bytes" << std::endl;
    }

  } else {
    if(serverReceived.handshake()){
        if(serverReceived.id() == player::SelfImpl::getInstance().getMyID()){
            player::SelfImpl::getInstance().setConnect(true);
            if(serverReceived.has_packettime()){
              using namespace std::chrono;
              google::protobuf::Timestamp* PacketTimestamp = serverReceived.mutable_packettime();
              system_clock::time_point serverTimePoint = time_point_cast<system_clock::time_point::duration>(system_clock::time_point(seconds(PacketTimestamp->seconds()) + nanoseconds(PacketTimestamp->nanos())));
              ProtocolParser::calcTimeDiffServer_andSet(serverTimePoint);
            }
        }
    }
  }
}

ThreadPool parserThreadPool(4);

void ProtocolParser::m_Parser_only THREAD_POOL_ARGS_NAMED{

  bool success = false;
  protocol::Server serverReceived = threadPoolL.pop(threadId, success);

  if(!success){
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }

  if(serverReceived.has_invalidsession()){
    if(serverReceived.invalidsession()){
      socketUdpImpl::getInstance().close();
    }
  }

  if(!serverReceived.has_handshake() || serverReceived.handshake() == false){
    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.has_sampletime()){
        sampleTime = serverReceived.sampletime();
      }
      PLAYER player(new Player(sf::milliseconds(sampleTime)));
      player->id = serverReceived.id();
      player->setPosition(0,0,0);
      PlayersManagerImpl::getInstance().insertPlayer(player);

    }

    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
      return;
    }

    PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());

    SimpleCoords pos = player->getPosition();
    bool altCoord = false;
    if(serverReceived.has_coordx()){
        pos.x = serverReceived.coordx();
        altCoord = true;
    }
    if(serverReceived.has_coordy()){
        pos.y = serverReceived.coordy();
        altCoord = true;
    }
    if(serverReceived.has_coordz()){
        pos.z = serverReceived.coordz();
        altCoord = true;
    }
    if(altCoord){
      player->setPosition(pos.x,pos.y,pos.z);
    }

    if(serverReceived.has_audio()){
        int audioNum = 0;
        int sampleTime = SAMPLE_TIME_DEFAULT;
        if(serverReceived.has_audionum()){
          audioNum = serverReceived.audionum();
        }
        if(serverReceived.has_sampletime()){
          sampleTime = serverReceived.sampletime();
        }

        data::buffer audio(serverReceived.audio().data(), serverReceived.audio().size());

        player->push(audioNum, audio, sampleTime);
    }

  } else {
    if(serverReceived.handshake()){
        if(serverReceived.id() == player::SelfImpl::getInstance().getMyID()){
            player::SelfImpl::getInstance().setConnect(true);
        }
    }
  }
  
}

void ProtocolParser::parse(protocol::Server& serverReceived){
  //std::thread(&ProtocolParser::m_Parser_only, this, serverReceived).detach();
  std::shared_ptr<protocol::Server> sptr(new protocol::Server(serverReceived));
  parserThreadPool.enqueue(&parseFunction, sptr);
  //threadPool.push(serverReceived);
  //std::thread(&ProtocolParser::parser_Finder, this, serverReceived).detach();
}

data::parseThreadPoll& ProtocolParser::getPool(){
  return threadPool;
}

std::shared_mutex ProtocolParser::timeDiffServer_mutex = std::shared_mutex();
std::chrono::system_clock::duration ProtocolParser::timeDiffServer = std::chrono::system_clock::duration(std::chrono::milliseconds(0));

std::shared_mutex ProtocolParser::connectSendTime_mutex;
std::chrono::system_clock::time_point ProtocolParser::connectSendTime = 
      time_point_cast<std::chrono::system_clock::duration>(std::chrono::high_resolution_clock::now());

bool ProtocolParser::isNegativeTimeDiffServer = false;

std::atomic_int ProtocolParser::invalidCalsFromServer = std::atomic_int(0);

void ProtocolParser::setTimeDiffServer(std::chrono::system_clock::duration time, bool isNegative){
  timeDiffServer_mutex.lock();
  isNegativeTimeDiffServer = isNegative;
  if(timeDiffServer > std::chrono::system_clock::duration(std::chrono::milliseconds(0))){
    timeDiffServer = std::chrono::system_clock::duration((time + timeDiffServer) / 2);
  } else {
    timeDiffServer = time;
  }
  timeDiffServer_mutex.unlock();
}

std::chrono::system_clock::duration ProtocolParser::getTimeDiffServer(){
  std::chrono::system_clock::duration result;
  timeDiffServer_mutex.lock_shared();
  result = timeDiffServer;
  timeDiffServer_mutex.unlock_shared();
  return result;
}

void ProtocolParser::setTimeConnectSend(std::chrono::system_clock::time_point time){
  connectSendTime_mutex.lock();
  connectSendTime = time;
  connectSendTime_mutex.unlock();
}

std::chrono::system_clock::time_point ProtocolParser::getTimeConnectSend(){
  std::chrono::system_clock::time_point result;
  connectSendTime_mutex.lock_shared();
  result = connectSendTime;
  connectSendTime_mutex.unlock_shared();
  return result;
}

void ProtocolParser::calcTimeDiffServer_andSet(std::chrono::system_clock::time_point serverTime){
    
    using namespace std::chrono;

    high_resolution_clock::time_point timePoint = high_resolution_clock::now();
    system_clock::time_point currentTimePoint = time_point_cast<system_clock::duration>(timePoint);

    // Calcular a metade do tempo de ida e volta
    auto elapsedTime = currentTimePoint - getTimeConnectSend(); // Tempo decorrido desde que o pacote foi enviado
    auto halfPingPongTime = elapsedTime / 2; // Metade do tempo de ida e volta

    system_clock::duration diffServerClientTime;
    // Calcular o tempo do cliente em relação ao servidor
    system_clock::time_point aproxServerTimeByPing = serverTime + halfPingPongTime;
    if(aproxServerTimeByPing > currentTimePoint){
      diffServerClientTime = aproxServerTimeByPing - currentTimePoint;
      setTimeDiffServer(diffServerClientTime, false);
    } else {
      diffServerClientTime = currentTimePoint - aproxServerTimeByPing;
      setTimeDiffServer(diffServerClientTime, true);
    }

    int ping = duration_cast<milliseconds>(halfPingPongTime).count();
    medianPing = ((medianPing.load() * 4) + ping) / 5;
    std::cout << "Ping recalc: " << medianPing << std::endl;
    ping = medianPing;
    int initAudioDelay = ping + (ping*20/100); // millis + 20%
    
    updateSoundProcessingTimes(duration_cast<milliseconds>(halfPingPongTime).count(), initAudioDelay);

}

ProtocolParser::ProtocolParser(){
  threadPool.insertThread(&ProtocolParser::m_Parser_only);
  threadPool.insertThread(&ProtocolParser::m_Parser_only);
  threadPool.insertThread(&ProtocolParser::m_Parser_only);
  threadPool.insertThread(&ProtocolParser::m_Parser_only);
  
  //threadPool.insertThread(&ProtocolParser::m_Parser_only);
  //threadPool.insertThread(&ProtocolParser::m_Parser_only);
}

ProtocolParser::~ProtocolParser(){
  threadPool.stopAll();
}