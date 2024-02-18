#ifndef CONFIG_H // include guard
#define CONFIG_H

#include "vector"
#include "atomic"
#include "mutex"

class Config{
    private:

    std::atomic_bool needCryptography;

    std::atomic_bool has_Key;
    std::atomic_bool has_Iv;

    std::vector<char> cryptoKey;
    std::vector<char> cryptoIV;

    std::mutex cryptoKeyMTX;
    std::mutex cryptoIVMTX;

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
        return needCryptography.load();
    }

    std::vector<char> getCryptoKey(){
        if(!has_Key.load()) perror("Crypto Key not found!");
        std::vector<char> result;
        cryptoKeyMTX.lock();
        result = cryptoKey;
        cryptoKeyMTX.unlock();
        return result;
    }

    void setCryptoKey(std::vector<char> key){
        cryptoKeyMTX.lock();
        cryptoKey = key;
        has_Key = true;
        cryptoKeyMTX.unlock();
    }

    std::vector<char> getCryptoIV(){
        if(!has_Iv.load()) perror("Crypto Iv not found!");
        std::vector<char> result;
        cryptoIVMTX.lock();
        result = cryptoIV;
        cryptoIVMTX.unlock();
        return result;
    }

    void setCryptoIV(std::vector<char> iv){
        cryptoIVMTX.lock();
        cryptoIV = iv;
        has_Iv = true;
        cryptoIVMTX.unlock();
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