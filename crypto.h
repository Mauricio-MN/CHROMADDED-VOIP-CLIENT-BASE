#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <iostream>
#include <vector>

class AES_GCM {
private:

std::vector<unsigned char> _key;
std::vector<unsigned char> _iv;

public:
    AES_GCM(const unsigned char* key, const unsigned char* iv)
        : key_(key), iv_(iv) {}

    std::string encrypt(const std::string& plaintext) {
        // Inicializar o contexto do EVP_CIPHER
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);

        // Definir a chave e o IV
        EVP_EncryptInit_ex(ctx, NULL, NULL, key_, iv_);

        // Obter o tamanho do texto cifrado
        int ciphertext_len = plaintext.length() + EVP_CIPHER_CTX_block_size(ctx);
        unsigned char* ciphertext = new unsigned char[ciphertext_len];

        // Criptografar o texto
        int len;
        EVP_EncryptUpdate(ctx, ciphertext, &len, (const unsigned char*)plaintext.c_str(), plaintext.length());
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
        std::string encrypted_message(reinterpret_cast<char*>(ciphertext), ciphertext_len);
        encrypted_message += std::string(reinterpret_cast<char*>(tag), sizeof(tag));

        delete[] ciphertext;

        return encrypted_message;
    }

    std::string decrypt(const std::string& encrypted_message) {
        // Inicializar o contexto do EVP_CIPHER
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);

        // Definir a chave e o IV
        EVP_DecryptInit_ex(ctx, NULL, NULL, key_, iv_);

        // Obter o tamanho do texto decifrado
        int plaintext_len = encrypted_message.length() - EVP_GCM_TLS_TAG_LEN;
        unsigned char* plaintext = new unsigned char[plaintext_len];

        // Decifrar o texto
        int len;
        EVP_DecryptUpdate(ctx, plaintext, &len, (const unsigned char*)encrypted_message.c_str(), encrypted_message.length() - EVP_GCM_TLS_TAG_LEN);
        plaintext_len = len;

        // Definir o valor do MAC (tag)
        unsigned char tag[EVP_GCM_TLS_TAG_LEN];
        memcpy(tag, encrypted_message.c_str() + encrypted_message.length() - EVP_GCM_TLS_TAG_LEN, EVP_GCM_TLS_TAG_LEN);
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
            return std::string();
        }

        std::string decrypted_message(reinterpret_cast<char*>(plaintext), plaintext_len);
        delete[] plaintext;

        return decrypted_message;
    }

private:
    const unsigned char* key_;
    const unsigned char* iv_;
};

#endif