#ifndef CRIPT_H // include guard
#define CRIPT_H

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>

class crypt{

private:
    void aes_init();
    std::vector<unsigned char> aes_128_gcm_encrypt(std::string plaintext, std::string key);
    std::vector<unsigned char> aes_128_gcm_encrypt(std::vector<unsigned char> *buffer, std::string key);
    std::vector<unsigned char> aes_128_gcm_decrypt(std::vector<unsigned char> *ciphertext, std::string key);
    std::string key;

public:

    std::vector<unsigned char> encrypt(std::vector<unsigned char> *buffer);
    std::vector<unsigned char> decrypt(std::vector<unsigned char> *ciphertextvect);
    void updateKey(unsigned char key_bytes[16]);
    crypt(unsigned char key_bytes[16]);

};

class CryptImpl{
private:
    static bool initialized;
    static unsigned char key_bytes_[16];
public:
    static crypt& getInstance();

    static void fabric(unsigned char key_bytes[16]);
};

#endif