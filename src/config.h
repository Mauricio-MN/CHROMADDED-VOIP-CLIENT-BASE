#ifndef CONFIG_H // include guard
#define CONFIG_H

#include "vector"

class Config{
    private:

    bool needCryptography;

    std::vector<char> cryptoKey;
    std::vector<char> cryptoIV;

    public:
    Config(){
        needCryptography = true;
    }

    void setNeedCryptography(bool isNeed){
        needCryptography = isNeed;
    }

    bool isNeedCryptography(){
        return needCryptography;
    }

    std::vector<char> getCryptoKey(){
        return cryptoKey;
    }

    void setCryptoKey(std::vector<char> key){
        cryptoKey = key;
    }

    std::vector<char> getCryptoIV(){
        return cryptoIV;
    }

    void setCryptoIV(std::vector<char> iv){
        cryptoIV = iv;
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