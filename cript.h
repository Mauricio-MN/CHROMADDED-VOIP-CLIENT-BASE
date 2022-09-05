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

    void encrypt(char* buffer, size_t size, vector<unsigned char> *ciphertextvect);
    void decrypt(vector<unsigned char> *ciphertextvect, char* buffer, size_t *size);
    int init(unsigned char key_bytes[16]);

}

#endif