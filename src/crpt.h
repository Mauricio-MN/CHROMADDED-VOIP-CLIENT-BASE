#ifndef CRPT_H // include guard
#define CRPT_H

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <iostream>
#include <cstring>
#include <vector>

#define NEW_AES_GCM ((unsigned char*)ConfigImpl::getInstance().getCryptoKey().data(), (unsigned char*)ConfigImpl::getInstance().getCryptoIV().data())

class AES_GCM {
public:
    AES_GCM(const unsigned char* key, const unsigned char* iv) {
        keyVct.insert(keyVct.end(), key, key + 32);
        ivVct.insert(ivVct.end(), iv, iv + 16);
        key_ = keyVct.data();
        iv_ = ivVct.data();
    }

    std::vector<unsigned char> encrypt(std::vector<unsigned char> &plaintext) {
        // Inicializar o contexto do EVP_CIPHER
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);

        // Definir a chave e o IV
        EVP_EncryptInit_ex(ctx, NULL, NULL, key_, iv_);

        // Obter o tamanho do texto cifrado
        int ciphertext_len = plaintext.size() + EVP_CIPHER_CTX_block_size(ctx);
        unsigned char* ciphertext = new unsigned char[ciphertext_len];

        // Criptografar o texto
        int len;
        EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext.data(), plaintext.size());
        ciphertext_len = len;

        // Finalizar a criptografia
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        ciphertext_len += len;

        // Obter o valor do MAC (tag)
        unsigned char tag[EVP_GCM_TLS_TAG_LEN];
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, sizeof(tag), tag);

        // Limpar o contexto
        EVP_CIPHER_CTX_free(ctx);

        // Construir a mensagem cifrada com a tag
        std::vector<unsigned char> result(ciphertext, ciphertext + ciphertext_len);
        result.insert(result.end(), tag, tag + sizeof(tag));

        delete[] ciphertext;

        return result;
    }

    std::vector<unsigned char> decrypt(std::vector<unsigned char>& encrypted_message) {
        // Inicializar o contexto do EVP_CIPHERuk
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);

        // Definir a chave e o IV
        EVP_DecryptInit_ex(ctx, NULL, NULL, key_, iv_);

        // Obter o tamanho do texto decifrado
        int plaintext_len = encrypted_message.size() - EVP_GCM_TLS_TAG_LEN;
        unsigned char* plaintext = new unsigned char[plaintext_len];

        // Decifrar o texto
        int len;
        EVP_DecryptUpdate(ctx, plaintext, &len, (const unsigned char*)encrypted_message.data(), encrypted_message.size() - EVP_GCM_TLS_TAG_LEN);
        plaintext_len = len;

        // Definir o valor do MAC (tag)
        unsigned char tag[EVP_GCM_TLS_TAG_LEN];
        memcpy(tag, encrypted_message.data() + encrypted_message.size() - EVP_GCM_TLS_TAG_LEN, EVP_GCM_TLS_TAG_LEN);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, EVP_GCM_TLS_TAG_LEN, tag);

        // Finalizar a decifragem
        int result = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
        plaintext_len += len;

        // Limpar o contexto
        EVP_CIPHER_CTX_free(ctx);

        // Verificar se a autenticação do MAC (tag) falhou
        if (result <= 0) {
            std::cerr << "Erro na autenticação do MAC (tag)" << std::endl;
            delete[] plaintext;
            return std::vector<unsigned char>();
        }

        std::vector<unsigned char> decrypted_message(plaintext, plaintext + plaintext_len);
        delete[] plaintext;

        return decrypted_message;
    }

private:
    unsigned char* key_;
    unsigned char* iv_;
    std::vector<unsigned char> keyVct;
    std::vector<unsigned char> ivVct;
};

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