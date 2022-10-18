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

#include "osimports.h"

#include "cript.h"
#include "player.h"
#include "bufferParser.h"
#include "protocol.h"
#include "connection.h"

#define MSG_OOB 0x1       /* process out-of-band data */
#define MSG_PEEK 0x2      /* peek at incoming message */
#define MSG_DONTROUTE 0x4 /* send without using routing tables */
#define MSG_WAITALL 0x8   /* do not complete until packet is */
#define MSG_CONFIRM 0x0

#define PORT 3366
#define MAXLINE 1024

  void connection::connectInit(){
    int connectionState = -1;
    while(connectionState < 0){
      connectionState = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
      if(connectionState < 0){
        connectionAtmp++;
        if(connectionAtmp >= max_connection_attempt){
          connectionState = 2;
          error = 2;
          printf("\n Error : Connect to VOIP Failed \n");
        }
      } else {
        connectionState = 1;
      }
    }
  }

  void connection::init(char* ip, int ip_size)
  {
    connectionAtmp = 0;
    receiveAtmp = 0;
    error = 0;

    char* valid_ip = new char[ip_size];
    protocol::tools::bufferToData(valid_ip, ip_size, ip);

    if (iswindows)
    {
      WSADATA wsaData;

      int iResult;

      // Initialize Winsock
      iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

      if (iResult != 0)
      {
        printf("WSAStartup failed: %d\n", iResult);
        error = 1;
      }
    }

    char buffer[512];
    char *message = new char[37]{0, 1, 2, 3};
    int message_len = 37;

    int n;
    struct sockaddr_in servaddr;

    // clear servaddr
    memset(&servaddr, 0, sizeof(servaddr));
    const char* constIP = valid_ip;
    servaddr.sin_addr.s_addr = inet_addr(constIP);
    servaddr.sin_port = htons(443);
    servaddr.sin_family = AF_INET;

    // create datagram socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    connectInit();
  }

  void connection::handChacke(){
    protocol::data buffer = protocol::tovoipserver::constructHandChackeData(players::self::getMyID());
    send(buffer.getBuffer(), buffer.size, players::self::needEncrypt());
  }

  void connection::closeSocket(){
    close(sockfd);
  }

  void connection::receiveThread(){
    // request to send datagram
    // no need to specify server address in sendto
    // connect stores the peers IP and port
    char buffer[512];
    while (true)
    {
      // sendto(sockfd, message, message_len, 0, (struct sockaddr *)NULL, sizeof(servaddr));

      // waiting for response
      int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)NULL, NULL);
      if(n < 1){
        receiveAtmp++;
        if(receiveAtmp >= max_receive_attempt){
          return;
        }
        continue;
      }

      protocol::data data(n);

      protocol::tools::cutBuffer(data.getBuffer(), buffer, 0, n);

      bufferparser::parser(&data);
    }
  }

  int connection::getError(){
    return error;
  }

  void connection::send(char *buffer, int size, bool encrypt)
  {
    int sendAtmp = 0;
    bool trySend = true;

    while(trySend){
      int checkSend = 0;
      if(encrypt){
        unsigned char* reinterpreted = reinterpret_cast<unsigned char*>(buffer);
        std::vector<unsigned char> decrypted(reinterpreted, reinterpreted + size);
        std::vector<unsigned char> encrypted = crypt::encrypt(&decrypted);
        checkSend = sendto(sockfd, reinterpret_cast<const char*>(encrypted.data()), encrypted.size(), 0, (struct sockaddr *)NULL, sizeof(servaddr));
      } else {
        checkSend = sendto(sockfd, buffer, size, 0, (struct sockaddr *)NULL, sizeof(servaddr));
      }

      if(checkSend < 0){
        sendAtmp++;
        if(sendAtmp >= max_send_attempt){
          trySend = false;
          sf::sleep(sf::milliseconds(1));
        }
      } else {
        trySend = false;
      }

    }
  }