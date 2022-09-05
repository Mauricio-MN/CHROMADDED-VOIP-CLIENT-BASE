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

#include "osimports.h"

#include "cript.h"
#include "player.h"
#include "bufferParser.h"
#include "protocol.h"

#define MSG_OOB 0x1       /* process out-of-band data */
#define MSG_PEEK 0x2      /* peek at incoming message */
#define MSG_DONTROUTE 0x4 /* send without using routing tables */
#define MSG_WAITALL 0x8   /* do not complete until packet is */
#define MSG_CONFIRM 0x0

#define PORT 3366
#define MAXLINE 1024

namespace connection
{

  int my_id;

  int sockfd;
  struct sockaddr_in servaddr;

  int init(int id)
  {
    my_id = id;

    if (iswindows)
    {
      WSADATA wsaData;

      int iResult;

      // Initialize Winsock
      iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

      if (iResult != 0)
      {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
      }
    }

    char buffer[512];
    char *message = new char[37]{0, 1, 2, 3};
    int message_len = 37;

    int sockfd, n;
    struct sockaddr_in servaddr;

    // clear servaddr
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(443);
    servaddr.sin_family = AF_INET;

    // create datagram socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
      printf("\n Error : Connect Failed \n");
      exit(0);
    }

    // sendto(sockfd, message, message_len, 0, (struct sockaddr *)NULL, sizeof(servaddr));

    int breack = 0;

    if (0 == 1)
    {
      char buffer[512];
      protocol::data data = protocol::tovoipserver::constructHandChackeData(id);
      char *message = (char *)"Hello Server";
      int message_len = sizeof("Hello Server");
      int n;

      // clear servaddr
      memset(&servaddr, 0, sizeof(servaddr));
      servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
      servaddr.sin_port = htons(PORT);
      servaddr.sin_family = AF_INET;

      // create datagram socket
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);

      // connect to server
      if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
      {
        printf("\n Error : Connect Failed \n");
        exit(0);
      }

      sendto(sockfd, data.buffer, data.size, 0, (struct sockaddr *)NULL, sizeof(servaddr));
    }
    // request to send datagram
    // no need to specify server address in sendto
    // connect stores the peers IP and port
    while (true)
    {
      // sendto(sockfd, message, message_len, 0, (struct sockaddr *)NULL, sizeof(servaddr));

      // waiting for response
      int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)NULL, NULL);
      puts(buffer);

      protocol::data data;
      data.buffer = new char[n];
      data.size = n;

      protocol::tools::cutBuffer(buffer, 0, data.buffer, n);

      for (size_t i = 0; i < n; i++)
      {
        printf("%02X ", buffer[i]);
        int b;
      }

      bufferparser::parser(&data);
    }

    // close the descriptor
    close(sockfd);
  }

  void send(char *buffer, size_t size)
  {
    sendto(sockfd, buffer, size, 0, (struct sockaddr *)NULL, sizeof(servaddr));
  }

}