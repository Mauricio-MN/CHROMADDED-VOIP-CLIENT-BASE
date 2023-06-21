#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <string>

#include "osimports.h"

#include "cript.h"
#include "player.h"
#include "bufferParser.h"
#include "protocol.h"
#include "connection.h"
#include "data.h"

#include "socketdefs.h"
#include "globaldefs.h"

#define MSG_OOB 0x1       /* process out-of-band data */
#define MSG_PEEK 0x2      /* peek at incoming message */
#define MSG_DONTROUTE 0x4 /* send without using routing tables */
#define MSG_WAITALL 0x8   /* do not complete until packet is */
#define MSG_CONFIRM 0x0

#define MAXLINE 1024
#define IPv4Size 4

std::string constructStrIP(char *ip){
  std::string out;
  std::stringstream streamout;

  for(int i = 0; i < IPv4Size; i++){
    streamout << (int)ip[i];
    if(i < 3){
      streamout << ".";
    }
  }

  streamout >> out;

  return out;
}

data::buffer decrypt(data::buffer &buffer){
  data::buffer procbuff(buffer.getData() + 1 ,buffer.size() - 1);
  unsigned char* reinterpreted = reinterpret_cast<unsigned char*>(procbuff.getData());

  std::vector<unsigned char> encrypted(reinterpreted, reinterpreted + procbuff.size());
  std::vector<unsigned char> decrypted;
  std::vector<unsigned char> decryptval = CryptImpl::getInstance().decrypt(&encrypted);

  decrypted.insert(decrypted.end(), reinterpreted[0]);
  decrypted.insert(decrypted.end(), decryptval.begin(), decryptval.end());
  data::buffer output(reinterpret_cast<char*>(decrypted.data()), decrypted.size());
  return output;
}

data::buffer decrypt(char* buffer, int size){
  data::buffer data(buffer,size);
  return decrypt(data);
}

int Connection::getSocket(){
  return sockfd;
}

void Connection::startWSA(){
  WSAinitialized = crossSocketModule::startWSA(WSAinitialized);
}

void Connection::safeDeleteThread(){
  if(threadIsRuning){
    threadCanRun = false;
    while(threadIsRuning){
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    threadCanRun = true;
  }
}

Connection::Connection(char *ip, int port)
{
  receiveAtmp = 0;
  countRepeatError = 0;
  needWait = false;
  waitTimeMS = 0;
  maxAttempt = 0;
  sockfd = 0;
  threadIsRuning = false;
  threadCanRun = true;
  WSAinitialized = false;
  isConnected = false;

  servaddr = new struct sockaddr_in;
  memset(servaddr, 0, sizeof(sockaddr_in));
  start(ip, port);
}

Connection::~Connection(){
  delete servaddr;
}

  void Connection::start(char *ip, int port){

  startWSA();

  servaddr->sin_addr.s_addr = inet_addr(constructStrIP(ip).c_str());
  servaddr->sin_port = htons(port);
  servaddr->sin_family = AF_INET;

  sockaddr_in testaddr;

  memset((char*)&testaddr, 0, sizeof(testaddr));
  testaddr.sin_family = AF_INET; 
  testaddr.sin_port = htons(443); 
  testaddr.sin_addr.S_un.S_addr = inet_addr(constructStrIP(ip).c_str());

  finalcheck();
  }

  void Connection::finalcheck(){

    if(sockfd == SOCKET_ERROR || sockfd == 0){
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if(sockfd == SOCKET_ERROR){
        errorHandling();
        return;
      } else {
        isConnected = true;
      }
    }

    safeDeleteThread();
    std::thread(&Connection::receiveThread, this).detach();
  }

  void Connection::handChacke(){
    data::buffer buffer = protocol::tovoipserver::constructHandChackeData(player::SelfImpl::getInstance().getMyRegID(), player::SelfImpl::getInstance().getMyID());
    send(buffer, player::SelfImpl::getInstance().needEncrypt());
  }

  void Connection::closeSocket(){
    osCloseSocket(sockfd);
    isConnected = false;
    safeDeleteThread();
  }

  void Connection::receiveThread(){
    // request to send datagram
    // no need to specify server address in sendto
    // connect stores the peers IP and port
    threadIsRuning = true;
    char buffer[512];

    while (isConnected && threadCanRun)
    {
      // sendto(sockfd, message, message_len, 0, (struct sockaddr *)NULL, sizeof(servaddr));

      // waiting for response
      
      int len = sizeof(sockaddr_in);
      int n = recvfrom(sockfd, buffer, 512, 0, (struct sockaddr *)&servaddr, &len);
      
      if(n < 1){
        receiveAtmp++;
        errorHandling();
        if(receiveAtmp >= max_receive_attempt){
          threadIsRuning = false;
          isConnected = false;
          return;
        }
        continue;
      }

      data::buffer data;

      if(player::SelfImpl::getInstance().needEncrypt()){

        data::buffer data = decrypt(buffer, n);

      } else{
        data = data::buffer(buffer, n);
      }

      //bufferparser::getInstance()->parser(&data);
      BufferParserImpl::getInstance().parserBuffer(&data);
    }
    threadIsRuning = false;
  }

  void Connection::send(data::buffer &buffer, bool encrypt)
  {
    int sendAtmp = 0;
    bool trySend = true;

    while(trySend){
      int checkSend = 0;
      if(encrypt){
        unsigned char* reinterpreted = reinterpret_cast<unsigned char*>(buffer.getData());
        std::vector<unsigned char> decrypted(reinterpreted + 1, reinterpreted + buffer.size());
        std::vector<unsigned char> encrypted = CryptImpl::getInstance().encrypt(&decrypted);
        std::vector<unsigned char> outputData;
        outputData.insert(outputData.end(), reinterpreted[0]);
        outputData.insert(outputData.end(), encrypted.begin(), encrypted.end());

        checkSend = sendto(sockfd, reinterpret_cast<const char*>(outputData.data()), outputData.size(), 0, (struct sockaddr *)servaddr, sizeof(sockaddr_in));
      } else {
        checkSend = sendto(sockfd, buffer.getData(), buffer.size(), 0, (struct sockaddr *)servaddr, sizeof(sockaddr_in));
      }

      if(checkSend < 0){
        sendAtmp++;
        errorHandling();
        if(sendAtmp >= max_send_attempt){
          trySend = false;
        }
        sf::sleep(sf::milliseconds(1));
      } else {
        trySend = false;
      }

    }
  }

  bool Connection::getIsConnected(){
    return isConnected;
  }

  int Connection::getError(){
    std::lock_guard<std::mutex> guard(errorMutex);
    
    int err = errorSeverity;
    errorSeverity = ERROR_NO_ERROR;
    return err;
  }

  void Connection::errorHandling(){
    std::lock_guard<std::mutex> guard(errorMutex);

    int recLastError = lastError;
    lastError = crossSocketModule::getError();
    lastError = crossSocketModule::getError(lastError);
    maxAttempt = -1;

    if(lastError == recLastError){
      countRepeatError++;
      if(countRepeatError > maxAttempt){
        countRepeatError = 0;
        errorSeverity = ERROR_MAX_ATTEMPT;
        return;
      }
      if(needWait){
        sf::sleep(sf::milliseconds(waitTimeMS));
      }
    } else {
      countRepeatError = 0;
    }

    switch (lastError)
    {
    case CRMD_INVALID_OBJ:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_NOT_ENOUGH_MEMORY:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_OPERATION_ABORTED:
      errorSeverity = ERROR_MEDIUM;
      break;
    case CRMD_IO_INCOMPLETE:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 5;
      waitTimeMS = 3000;
      break;
    case CRMD_PERMISSION_DENIED:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_BAD_ADDRESS:
      closeSocket();
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      break;
    case CRMD_MANY_SOCKETS:
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 3000;
      break;
    case CRMD_RESOURCE_TEMP_UNAVAILABLE:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 10;
      waitTimeMS = 2000;
      break;
    case CRMD_NOW_IN_PROGRESS:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_ALREADY_IN_PROGRESS:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_IS_A_NON_SOCKET:
      isConnected = false;
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_MSG_TO_LONG:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_ADDRESS_ALREADY_IN_USE:
      closeSocket();
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      break;
    case CRMD_ADDRESS_NOT_AVAILABLE:
      closeSocket();
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      break;
    case CRMD_NETWORK_IS_DOWN:
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 10;
      waitTimeMS = 3000;
      break;
    case CRMD_NETWORK_IS_UNREACHABLE:
      errorSeverity = ERROR_MEDIUM;
      break;
    case CRMD_NETW_DROP_CONN_ON_RESET:
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      break;
    case CRMD_CONNECTION_ABORTED:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 3000;
      break;
    case CRMD_CONNECTION_RESET_BY_PEER:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 2000;
      break;
    case CRMD_NO_BUFFER_SPACE:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 5000;
      break;
    case CRMD_ALREADY_CONNECTED:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_IS_NOT_CONNECTED:
      finalcheck();
      break;
    case CRMD_SHUTDOWN:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_TIMEOUT:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_REFUSED:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 2000;
      break;
    case CRMD_HOST_DOWN:
      closeSocket();
      finalcheck();
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      needWait = true;
      maxAttempt = 10;
      waitTimeMS = 3000;
      break;
    case CRMD_HOST_UNREACH:
      // do something;
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      break;
    case CRMD_MANY_PROCESSES:
      errorSeverity = ERROR_MEDIUM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 2000;
      break;
    case CRMD_GRACEF_SHUTDOWN_IN_PROG:
      errorSeverity = ERROR_LOW;
      needWait = true;
      maxAttempt = 4;
      waitTimeMS = 2000;
      break;
    case CRMD_NO_MORE_RESULTS:
      errorSeverity = ERROR_LOW;
      break;
    case CRMD_SERVIC_PROVIDR_FAIL_INIT:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_SYS_CALL_FAILURE:
      errorSeverity = ERROR_SERIOUS;
      break;
    case CRMD_HOST_NOT_FOUND:
      errorSeverity = ERROR_ADDRESS_PROBLEM;
      needWait = true;
      maxAttempt = 3;
      waitTimeMS = 5000;
      break;
    case CRMD_TRY_AGAIN:
      errorSeverity = ERROR_LOW;
      needWait = true;
      maxAttempt = 10;
      waitTimeMS = 500;
      break;
    case CRMD_NO_RECOVERY:
      errorSeverity = ERROR_SERIOUS;
      break;

    case CRMD_WINDOWS_WSA_NOT_INITIALISED:
      errorSeverity = ERROR_SERIOUS;
      break;
    default:
      errorSeverity = ERROR_UNKNOW;
      break;
    }


  }

  bool ConnectionImpl::initialized = false;
  char *ConnectionImpl::ip_ = nullptr;
  int ConnectionImpl::port_ = 0;

  Connection& ConnectionImpl::getInstance(){
    if(initialized){
      static Connection instance(ip_, port_);
      return instance;
    } else {
      perror("fabric Connection!");
    }
  }

  void ConnectionImpl::fabric(char* ip, int port){
    if(initialized == false){
      ip_ = ip;
      port_ = port;
      initialized = true;
    }
  }

  int FakeServerSocket::sockfd;
  struct sockaddr_in FakeServerSocket::servaddr;
  struct sockaddr_in FakeServerSocket::cliaddr;

  void FakeServerSocket::init()
  {

    if (iswindows)
    {
      WSADATA wsaData;

      int iResult = 0;

      // Initialize Winsock
      iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

      if (iResult != 0)
      {
        printf("WSAStartup failed: %d\n", iResult);
      }
    }

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("socket creation failed");
      exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    //servaddr.sin_addr.s_addr = INADDR_ANY;
    overInetPton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));
    servaddr.sin_port = htons(CONN_DEBUG_PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }

    std::thread(receive).detach();
  }

  void FakeServerSocket::receive(){
    
    char buffer[MAXLINE];

    int len, n;

    len = sizeof(cliaddr); // len is value/result

    while (true)
    {
      memset(&buffer, 0, sizeof(buffer));
      n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                   0, (struct sockaddr *)&cliaddr,
                   &len);
      if(n < 0){
        perror("receive failed");
        int error = crossSocketModule::getError();
        continue;
      }

      buffer[n] = '\0';

      int sndmsg = sendto(sockfd, (const char *)buffer, n,
             MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
             len);
      if(sndmsg < 0){
        perror("send failed");
        int error = crossSocketModule::getError();
        continue;
      }
    }
  }