#ifndef CONFIG_H // include guard
#define CONFIG_H

#include "vector"

class Config{
    private:

    bool needCryptography;

    bool has_Key;
    bool has_Iv;

    std::vector<char> cryptoKey;
    std::vector<char> cryptoIV;

    public:
    Config(){
        needCryptography = true;
        has_Key = false;
        has_Iv = false;
    }

    void setNeedCryptography(bool isNeed){
        needCryptography = isNeed;
    }

    bool isNeedCryptography(){
        return needCryptography;
    }

    std::vector<char> getCryptoKey(){
        if(!has_Key) perror("Crypto Key not found!");
        return cryptoKey;
    }

    void setCryptoKey(std::vector<char> key){
        cryptoKey = key;
        has_Key = true;
    }

    std::vector<char> getCryptoIV(){
        if(!has_Key) perror("Crypto Iv not found!");
        return cryptoIV;
    }

    void setCryptoIV(std::vector<char> iv){
        cryptoIV = iv;
        has_Iv = true;
    }

};

class ConfigImpl{

    public:

    static Config& getInstance(){
        static Config instance;
        return instance;
    }
};



#endif