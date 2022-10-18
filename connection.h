#ifndef CONNECTION_H // include guard
#define CONNECTION_H

namespace connection {

    int init(int id, char* ip, int ip_size);
    void send(char *buffer, int size);

}


#endif