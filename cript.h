#ifndef CRIPT_H // include guard
#define CRIPT_H

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>

namespace crypt{

    using std::string;
    using std::vector;

    vector<unsigned char> encrypt(vector<unsigned char> *buffer);
    vector<unsigned char> decrypt(vector<unsigned char> *ciphertextvect);
    void updateKey(unsigned char key_bytes[16]);
    int init(unsigned char key_bytes[16]);

}

#endif