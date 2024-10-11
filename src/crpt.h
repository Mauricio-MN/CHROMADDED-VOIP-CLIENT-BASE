#ifndef CRPT_H // include guard
#define CRPT_H

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <iostream>
#include <cstring>
#include <vector>

#define NEW_AES_GCM (ConfigImpl::getInstance().getCryptoKey(), ConfigImpl::getInstance().getCryptoIV())
#define TAG_SIZE EVP_GCM_TLS_TAG_LEN

class AES_GCM {
public:
    AES_GCM(std::vector<char> key, std::vector<char> iv) {
        keyVct.insert(keyVct.end(), key.begin(), key.end());
        if(keyVct.size() < 32){
            std::vector<unsigned char> extraSize(32 - keyVct.size());
            for(auto& val : extraSize){
                val = 0;
            }
            keyVct.insert(keyVct.end(), extraSize.begin(), extraSize.end());
        }
        ivVct.insert(ivVct.end(), iv.begin(), iv.end());

        keySize = key.size();
        ivSize = iv.size();
    }

    ~AES_GCM(){
    }

    std::vector<unsigned char> encrypt(std::vector<unsigned char> &plaintext) {

        // Inicializar o contexto do EVP_CIPHER
        EVP_CIPHER_CTX* ctxEncrypt = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctxEncrypt, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        // Definir a chave e o IV
        EVP_CIPHER_CTX_ctrl(ctxEncrypt, EVP_CTRL_GCM_SET_IVLEN, ivSize, nullptr);
        EVP_EncryptInit_ex(ctxEncrypt, nullptr, nullptr, keyVct.data(), ivVct.data());

        // Obter o tamanho do texto cifrado
        int ciphertext_len = plaintext.size() + EVP_CIPHER_CTX_block_size(ctxEncrypt);
        unsigned char* ciphertext = new unsigned char[ciphertext_len];

        // Criptografar o texto
        int len;
        EVP_EncryptUpdate(ctxEncrypt, ciphertext, &len, plaintext.data(), plaintext.size());
        ciphertext_len = len;

        // Finalizar a criptografia
        EVP_EncryptFinal_ex(ctxEncrypt, ciphertext + len, &len);
        ciphertext_len += len;

        // Obter o valor do MAC (tag)
        unsigned char tag[EVP_GCM_TLS_TAG_LEN];
        EVP_CIPHER_CTX_ctrl(ctxEncrypt, EVP_CTRL_GCM_GET_TAG, sizeof(tag), tag);

        // Construir a mensagem cifrada com a tag
        std::vector<unsigned char> result(ciphertext, ciphertext + ciphertext_len);
        result.insert(result.end(), tag, tag + sizeof(tag));

        delete[] ciphertext;
        EVP_CIPHER_CTX_free(ctxEncrypt);

        return result;
    }

    std::vector<unsigned char> decrypt(std::vector<unsigned char>& encrypted_message) {

        // Inicializar o contexto do EVP_CIPHER
        EVP_CIPHER_CTX* ctxDecrypt = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctxDecrypt, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        // Definir a chave e o IV
        EVP_CIPHER_CTX_ctrl(ctxDecrypt, EVP_CTRL_GCM_SET_IVLEN, ivSize, nullptr);
        EVP_DecryptInit_ex(ctxDecrypt, nullptr, nullptr, keyVct.data(), ivVct.data());

        // Obter o tamanho do texto decifrado
        int plaintext_len = encrypted_message.size() - EVP_GCM_TLS_TAG_LEN;
        unsigned char* plaintext = new unsigned char[plaintext_len];

        // Decifrar o texto
        int len;
        EVP_DecryptUpdate(ctxDecrypt, plaintext, &len, (const unsigned char*)encrypted_message.data(), encrypted_message.size() - EVP_GCM_TLS_TAG_LEN);
        plaintext_len = len;

        // Definir o valor do MAC (tag)
        unsigned char tag[EVP_GCM_TLS_TAG_LEN];
        memcpy(tag, encrypted_message.data() + encrypted_message.size() - EVP_GCM_TLS_TAG_LEN, EVP_GCM_TLS_TAG_LEN);
        EVP_CIPHER_CTX_ctrl(ctxDecrypt, EVP_CTRL_GCM_SET_TAG, EVP_GCM_TLS_TAG_LEN, tag);

        // Finalizar a decifragem
        int result = EVP_DecryptFinal_ex(ctxDecrypt, plaintext + len, &len);
        plaintext_len += len;

        // Verificar se a autenticação do MAC (tag) falhou
        if (result <= 0) {
            std::cerr << "Erro na autenticação do MAC (tag)" << std::endl;
            delete[] plaintext;
            return std::vector<unsigned char>();
        }

        std::vector<unsigned char> decrypted_message(plaintext, plaintext + plaintext_len);
        delete[] plaintext;
        EVP_CIPHER_CTX_free(ctxDecrypt);

        return decrypted_message;
    }

private:
    std::vector<unsigned char> keyVct;
    std::vector<unsigned char> ivVct;

    uint32_t keySize;
    uint32_t ivSize;
};

#endif