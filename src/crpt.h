#ifndef CRPT_H // include guard
#define CRPT_H

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <cstring>

class AESGCM256EncryptorDecryptor {
public:
    AESGCM256EncryptorDecryptor(const unsigned char* key) {
        memcpy(aes_key_, key, AESGCM256_KEY_SIZE);
        memset(iv_, 0, AESGCM256_IV_SIZE);  // IV fixo
    }

    void Encrypt(const unsigned char* plaintext, size_t plaintext_len, unsigned char* ciphertext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AESGCM256_IV_SIZE, nullptr);
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, aes_key_, iv_);
        
        int len;
        int ciphertext_len;
        
        EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
        ciphertext_len = len;
        
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        ciphertext_len += len;
        
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AESGCM256_TAG_SIZE, tag_);
        EVP_CIPHER_CTX_free(ctx);
    }

    bool Decrypt(const unsigned char* ciphertext, size_t ciphertext_len, unsigned char* plaintext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AESGCM256_IV_SIZE, nullptr);
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, aes_key_, iv_);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AESGCM256_TAG_SIZE, tag_);

        int len;
        int plaintext_len;

        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
        plaintext_len = len;

        int ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

        if (ret > 0) {
            plaintext_len += len;
        } else {
            std::cerr << "Erro na verificação da TAG. A descriptografia falhou." << std::endl;
        }

        EVP_CIPHER_CTX_free(ctx);
    }

private:
    static const int AESGCM256_KEY_SIZE = 32; // Tamanho da chave AES-GCM-256 em bytes
    static const int AESGCM256_IV_SIZE = 12;  // Tamanho do IV em bytes
    static const int AESGCM256_TAG_SIZE = 16; // Tamanho da TAG em bytes

    unsigned char aes_key_[AESGCM256_KEY_SIZE];
    unsigned char iv_[AESGCM256_IV_SIZE];
    unsigned char tag_[AESGCM256_TAG_SIZE];
};

#endif