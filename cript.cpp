// code snippets are licensed under Creative Commons CC-By-SA 3.0 (unless otherwise specified)
// http://www.zedwood.com/article/cpp-openssl-aes-gcm-code-sample

#include "cript.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>

namespace crypt{

using std::string;
using std::vector;
using std::cout;
using std::endl;

void aes_init()
{
    static int init=0;
    if (init==0)
    {
        EVP_CIPHER_CTX* e_ctx;
        EVP_CIPHER_CTX* d_ctx;
 
        //initialize openssl ciphers
        OpenSSL_add_all_ciphers();
 
        //initialize random number generator (for IVs)
        int rv = RAND_load_file("/dev/urandom", 32);
    }
}
 
std::vector<unsigned char> aes_128_gcm_encrypt(std::string plaintext, std::string key)
{
    aes_init();
 
    size_t enc_length = plaintext.length()*3;
    std::vector<unsigned char> output;
    output.resize(enc_length,'\0');
 
    unsigned char tag[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, sizeof(iv));
    std::copy( iv, iv+16, output.begin()+16);
 
    int actual_size=0, final_size=0;
    EVP_CIPHER_CTX* e_ctx = EVP_CIPHER_CTX_new();
    //EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
    EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
    EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext.data(), plaintext.length() );
    EVP_EncryptFinal(e_ctx, &output[32+actual_size], &final_size);
    EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    std::copy( tag, tag+16, output.begin() );
    std::copy( iv, iv+16, output.begin()+16);
    output.resize(32 + actual_size+final_size);
    EVP_CIPHER_CTX_free(e_ctx);
    return output;
}

std::vector<unsigned char> aes_128_gcm_encrypt(std::vector<unsigned char> buffer, std::string key)
{
    aes_init();
 
    size_t enc_length = buffer.size()*3;
    std::vector<unsigned char> output;
    output.resize(enc_length,'\0');
 
    unsigned char tag[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, sizeof(iv));
    std::copy( iv, iv+16, output.begin()+16);
 
    int actual_size=0, final_size=0;
    EVP_CIPHER_CTX* e_ctx = EVP_CIPHER_CTX_new();
    //EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
    EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
    EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)buffer.data(), buffer.size() );
    EVP_EncryptFinal(e_ctx, &output[32+actual_size], &final_size);
    EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    std::copy( tag, tag+16, output.begin() );
    std::copy( iv, iv+16, output.begin()+16);
    output.resize(32 + actual_size+final_size);
    EVP_CIPHER_CTX_free(e_ctx);
    return output;
}
 
std::string aes_128_gcm_decrypt(std::vector<unsigned char> ciphertext, std::string key)
{
    aes_init();
 
    unsigned char tag[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    std::copy( ciphertext.begin(),    ciphertext.begin()+16, tag);
    std::copy( ciphertext.begin()+16, ciphertext.begin()+32, iv);
    std::vector<unsigned char> plaintext; plaintext.resize(ciphertext.size(), '\0');
 
    int actual_size=0, final_size=0;
    EVP_CIPHER_CTX *d_ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
    EVP_DecryptUpdate(d_ctx, &plaintext[0], &actual_size, &ciphertext[32], ciphertext.size()-32 );
    EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
    EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);
    EVP_CIPHER_CTX_free(d_ctx);
    plaintext.resize(actual_size + final_size, '\0');
 
    return string(plaintext.begin(),plaintext.end());
}

string key;


void encrypt(char* buffer, size_t size, vector<unsigned char> *ciphertextvect){

    string plaintext = string(buffer, size);
    //text to encrypt
    //string plaintext= "elephants in space";
    //cout << plaintext << endl;
 
    //encrypt
    *ciphertextvect = aes_128_gcm_encrypt(plaintext, key);
}

void decrypt(vector<unsigned char> *ciphertextvect, char* buffer, size_t *size){
    string out = aes_128_gcm_decrypt(*ciphertextvect, key);
    size_t len = out.length();
    buffer = new char[len + 1];
    // copying the contents of the
    // string to char array
    strcpy(buffer, out.c_str());
    *size = len;
}



 
int init(unsigned char key_bytes[16])
{
    aes_init();
    string key = string((char *)key_bytes, sizeof(unsigned char)*16);
    return 1;
}

}