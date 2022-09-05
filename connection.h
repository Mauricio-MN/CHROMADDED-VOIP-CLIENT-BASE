#ifndef CONNECTION_H // include guard
#define CONNECTION_H

namespace connection {

    int init(int id);
    bool send(char *buffer, size_t size);

}


#endif