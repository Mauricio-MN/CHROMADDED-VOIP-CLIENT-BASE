#ifndef CONNECTION_H // include guard
#define CONNECTION_H

class connection
{
private:
  static int max_connection_attempt;
  static int max_receive_attempt;
  static int max_send_attempt;

  static int connectionAtmp;
  static int receiveAtmp;
  static int sockfd;
  static int error;
  static struct sockaddr_in servaddr;

  static void connectInit();

public:
  static void init(char* ip, int ip_size);

  static void handChacke();

  static int getError();

  static void send(char *buffer, int size, bool encrypt);

  static void receiveThread();

  static void closeSocket();

};


#endif