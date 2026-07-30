
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
    std::function<void(ServerPacket&)> function;
    std::shared_ptr<ServerPacket> server;

    bool valid;

    Task(){
      valid = false;
    }

    Task(std::function<void(ServerPacket&)> f, std::shared_ptr<ServerPacket> s)
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
      worker_status.reserve(10);

      for(int i = 0; i < 10; i++){
        worker_status.emplace_back(std::make_unique<std::atomic<bool>>(true));
      }

      addWorkers(numThreads);
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void addWorkers(size_t numThreads) {
        for (size_t i = 0 + workers.size(); i < numThreads; ++i) {
            if(workers.size() >= 10) return;
            worker_status[i].get()->store(true);
            workers.emplace_back([this,i] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        if (!worker_status[i].get()->load()) {
                            return;
                        }

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

    void removeWorkers(size_t numToRemove) {
      size_t removed = 0;
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        size_t currentSize = workers.size();

        // Ajusta o número de threads a serem removidas
        if (numToRemove > currentSize) {
            numToRemove = currentSize;
        }
        
        for (int  i = (int)currentSize - 1; i >= 0 && removed < numToRemove; --i) { // size_t é unsigned, então usamos int para evitar underflow
            if (worker_status[i].get()->load()) {
                worker_status[i].get()->store(false); // Sinaliza a thread para encerrar
                ++removed;
            }
        }
      }

      condition.notify_all(); // Notifica todas as threads para verificar seu status

      // Aguarda o término das threads marcadas para remoção
      for (size_t i = workers.size() - 1; i >= 0 && numToRemove > 0; --i) {
          if (!worker_status[i].get()->load()) {
              if (workers[i].joinable()) {
                  workers[i].join(); // Espera a thread encerrar
              }
              workers.pop_back();    // Remove a thread do vetor
              --numToRemove;
          }
      }
    }


    void enqueue(std::function<void(ServerPacket&)> f, std::shared_ptr<ServerPacket> s) {
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
    std::vector<std::unique_ptr<std::atomic<bool>>> worker_status;
    std::queue<Task> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

std::atomic_int lastPing(0);
std::atomic_int medianPing(0);
std::atomic_int medianJitter(0);

int ProtocolParser::getJitter(){
  return medianJitter.load();
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
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay); // era +25
  } else {
      soundmanager::RecorderImpl::getInstance().setIntervalTime(40); // frame menor
      soundmanager::NetworkAudioStream::setInitAudioDelay(initAudioDelay); // era +20
  }
}

void parseFunction(ServerPacket& serverReceived){

  if(serverReceived.server_info.has_InvalidSession){
    if(serverReceived.server_info.InvalidSession){
      ProtocolParser::invalidCalsFromServer += 1;
      if(ProtocolParser::invalidCalsFromServer.load() > 2){
        socketUdpImpl::getInstance().close();
        ProtocolParser::invalidCalsFromServer = 0;
        medianPing = 0;
        std::cout << "Invalid Session!" << std::endl;
        return;
      }
    }
  }

  if(serverReceived.server_info.has_coordX){
      //std::cout << "RECEIVED X: " << serverReceived.server_info.coordX << std::endl;
  }

  // Packet inside packet and DUMP. Network loss controll  foi retirado por ser frágil, cada soundmanager faz seu calculo agora.
  if(serverReceived.extraClientBuffers.size() > 0){
    int sampleTotalTime = 0;

    for (auto& buffer : serverReceived.extraClientBuffers) {
      ServerPacket extraServer;

      pb_istream_t stream = pb_istream_from_buffer(buffer.data(), buffer.size());

      if (pb_decode(&stream, protocol_Server_fields, &extraServer.server_info)) {
          parseFunction(extraServer);
      }
    }
  }

  if(serverReceived.server_info.has_packetTime && serverReceived.server_info.has_handShake == false){
    using namespace std::chrono;

    google_protobuf_Timestamp& PacketTimestamp = serverReceived.server_info.packetTime;
    high_resolution_clock::time_point timePoint = high_resolution_clock::now();

    system_clock::time_point serverTimePoint = time_point_cast<system_clock::time_point::duration>(
      system_clock::time_point(seconds(PacketTimestamp.seconds) + nanoseconds(PacketTimestamp.nanos)));
    system_clock::time_point currentTimePoint = time_point_cast<system_clock::duration>(timePoint);

    // server no passado
    if(ProtocolParser::isNegativeTimeDiffServer){
      currentTimePoint -= ProtocolParser::getTimeDiffServer();
    } else {
      // server no futuro
      currentTimePoint += ProtocolParser::getTimeDiffServer();
    }

    auto millis = duration_cast<milliseconds>(currentTimePoint - serverTimePoint).count();
    if(millis == 0) millis = 1;
    if(millis < 0){
      millis = 1;
      player::SelfImpl::getInstance().sendConnect();
    }

    if(lastPing == 0) lastPing = millis;
    int Jitter = abs(millis - lastPing);
    medianJitter = medianJitter + (Jitter - medianJitter) / 8;

    lastPing = millis;

    medianPing = ((medianPing.load() * 4) + millis) / 5;
    std::cout << "Ping: " << medianPing << std::endl;
    millis = medianPing;
    int initAudioDelay = millis + (millis*10/100) + medianJitter;

    updateSoundProcessingTimes((Jitter + millis) / 3, initAudioDelay);
  }

  if(!serverReceived.server_info.has_handShake || serverReceived.server_info.handShake == false){
    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.server_info.id)){
      if(serverReceived.server_info.id == 0) return; // id 0 is invalid, ignore. TODO find a better solution for this

      std::cout << "Player " << serverReceived.server_info.id << " not exist, Added" << std::endl;
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.server_info.has_sampleTime){
        sampleTime = serverReceived.server_info.sampleTime;
      }
      PLAYER player(new Player(sf::milliseconds(sampleTime)));
      player->id = serverReceived.server_info.id;
      player->setPosition(0,0,0);
      PlayersManagerImpl::getInstance().insertPlayer(player);
    }

    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.server_info.id)){
      std::cout << "Player " << serverReceived.server_info.id << " not exist" << std::endl;
      return;
    }

    PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.server_info.id);

    SimpleCoords pos = player->getPosition();
    bool altCoord = false;
    if(serverReceived.server_info.has_coordX){
      pos.x = serverReceived.server_info.coordX;
      altCoord = true;
    }
    if(serverReceived.server_info.has_coordY){
      pos.y = serverReceived.server_info.coordY;
      altCoord = true;
    }
    if(serverReceived.server_info.has_coordZ){
      pos.z = serverReceived.server_info.coordZ;
      altCoord = true;
    }
    if(serverReceived.server_info.has_isGroup){
      if(serverReceived.server_info.isGroup){
        player->setToListenGroup(true);
        altCoord = false;
      } else {
        player->setToListenGroup(false);
      }
    }
    if(altCoord){
      player->setPosition(pos.x, pos.y, pos.z);
    }

    if(serverReceived.audioBuffer.size() > 0){
      int audioNum = 0;
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.server_info.has_audioNum){
        audioNum = serverReceived.server_info.audioNum;
      }
      if(serverReceived.server_info.has_sampleTime){
        sampleTime = serverReceived.server_info.sampleTime;
      }

      int sampleCount = sampleTimeGetCount(sampleTime, SAMPLE_RATE);
      data::buffer audio(serverReceived.audioBuffer.data(), serverReceived.audioBuffer.size());

      player->push(audioNum, audio, sampleTime);
    }

  } else {
    if(serverReceived.server_info.handShake){
      if(serverReceived.server_info.id == player::SelfImpl::getInstance().getMyID()){
        player::SelfImpl::getInstance().setConnect(true);
        if(serverReceived.server_info.has_packetTime){
          using namespace std::chrono;
          google_protobuf_Timestamp& PacketTimestamp = serverReceived.server_info.packetTime;
          system_clock::time_point serverTimePoint = time_point_cast<system_clock::time_point::duration>(
            system_clock::time_point(seconds(PacketTimestamp.seconds) + nanoseconds(PacketTimestamp.nanos)));
          ProtocolParser::calcTimeDiffServer_andSet(serverTimePoint);
        }
      }
    }
  }
}

ThreadPool parserThreadPool(4);

void ProtocolParser::m_Parser_only THREAD_POOL_ARGS_NAMED{

  bool success = false;
  ServerPacket serverReceived = threadPoolL.pop(threadId, success);

  if(!success){
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }

  if(serverReceived.server_info.has_InvalidSession){
    if(serverReceived.server_info.InvalidSession){
      socketUdpImpl::getInstance().close();
    }
  }

  if(!serverReceived.server_info.has_handShake || serverReceived.server_info.handShake == false){
    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.server_info.id)){
      if(serverReceived.server_info.id == 0) return; // id 0 is invalid, ignore. TODO find a better solution for this
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.server_info.has_sampleTime){
        sampleTime = serverReceived.server_info.sampleTime;
      }
      PLAYER player(new Player(sf::milliseconds(sampleTime)));
      player->id = serverReceived.server_info.id;
      player->setPosition(0,0,0);
      PlayersManagerImpl::getInstance().insertPlayer(player);

    }

    if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.server_info.id)){
      return;
    }

    PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.server_info.id);

    SimpleCoords pos = player->getPosition();
    bool altCoord = false;
    if(serverReceived.server_info.has_coordX){
        pos.x = serverReceived.server_info.coordX;
        altCoord = true;
    }
    if(serverReceived.server_info.has_coordY){
        pos.y = serverReceived.server_info.coordY;
        altCoord = true;
    }
    if(serverReceived.server_info.has_coordZ){
        pos.z = serverReceived.server_info.coordZ;
        altCoord = true;
    }
    if(altCoord){
      player->setPosition(pos.x,pos.y,pos.z);
    }

    if(serverReceived.audioBuffer.size() > 0){
        int audioNum = 0;
        int sampleTime = SAMPLE_TIME_DEFAULT;
        if(serverReceived.server_info.has_audioNum){
          audioNum = serverReceived.server_info.audioNum;
        }
        if(serverReceived.server_info.has_sampleTime){
          sampleTime = serverReceived.server_info.sampleTime;
        }

        data::buffer audio(serverReceived.audioBuffer.data(), serverReceived.audioBuffer.size());

        player->push(audioNum, audio, sampleTime);
    }

  } else {
    if(serverReceived.server_info.handShake){
        if(serverReceived.server_info.id == player::SelfImpl::getInstance().getMyID()){
            player::SelfImpl::getInstance().setConnect(true);
        }
    }
  }
  
}

void ProtocolParser::parse(ServerPacket& serverReceived){
  //std::thread(&ProtocolParser::m_Parser_only, this, serverReceived).detach();
  std::shared_ptr<ServerPacket> sptr(new ServerPacket(serverReceived));
  parserThreadPool.enqueue(&parseFunction, sptr);
  //threadPool.push(serverReceived.server_info);
  //std::thread(&ProtocolParser::parser_Finder, this, serverReceived.server_info).detach();
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