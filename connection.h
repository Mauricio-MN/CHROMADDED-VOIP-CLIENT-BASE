#ifndef CONNECTION_H // include guard
#define CONNECTION_H

#include <mutex>
#include "data.h"
#include "singleton.h"

#define CONN_DEBUG_IP [4]{127,0,0,1}
#define CONN_DEBUG_PORT   4345

class Connection
{
private:
  int max_connection_attempt;
  int max_receive_attempt;
  int max_send_attempt;

  int connectionAtmp;
  int receiveAtmp;
  int sockfd;
  struct sockaddr_in* servaddr;
  data::buffer servaddrData;

  bool WSAinitialized;

  bool isConnected;

  void errorHandling();
  
  int errorSeverity;
  int lastError;
  int countRepeatError;
  bool needWait;
  int waitTimeMS;
  int maxAttempt;

  bool threadIsRuning;
  bool threadCanRun;

  std::mutex errorMutex;

  void safeDeleteThread();

  void binding();

public:

  Connection(char* ip, int ip_size, int port);

  void start(char* ip, int ip_size, int port);

  void handChacke();

  int getError();

  void send(data::buffer &buffer, bool encrypt);

  void receiveThread();

  void closeSocket();

  bool getIsConnected();

  int getSocket();

  void startWSA();

};

class ConnectionImpl{
  private:
    static bool initialized;
    static char* ip_;
    static int ip_size_;
    static int port_;

  public:
  static Connection& getInstance();

  static void fabric(char* ip, int ip_size, int port);

};

SINGLETON_F_INIT(ConnectionImplT)
static char* ip;
static int ip_size;
static int port;
SINGLETON_F_INSTC_ARGS(Connection)
ip, ip_size, port
SINGLETON_F_ARGS
char* ip_, int ip_size_, int port_
SINGLETON_F_FUNC
SINGLETON_F_ARG_TO(ip_, ip)
SINGLETON_F_ARG_TO(ip_size_, ip_size)
SINGLETON_F_ARG_TO(port_, port)
SINGLETON_F_FUNC_END
SINGLETON_F_END

class FakeServerSocket{

private:
    static int sockfd;
    static struct sockaddr_in servaddr;
    static struct sockaddr_in cliaddr; 
  
public:

  static void init();
  static void receive();

};


#endif