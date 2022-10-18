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
  static int my_id;
  static int sockfd;
  static int error;
  static struct sockaddr_in servaddr;

  static void connectInit();

  static void closeSocket();

  static void receiveThread();

public:
  static void init(int id, char* ip, int ip_size);

  static int getError();

  static void send(char *buffer, int size, bool encrypt);

};


#endif