#ifndef SOCKETUDP_H // include guard
#define SOCKETUDP_H

#include <SFML/Network.hpp>
#include "data.h"
#include "proto/protocol.pb.h"

#include "crpt.h"
#include "player.h"
#include "protoParse.h"
#include "thread"
#include "config.h"

class socketUdp{

    private:

    sf::UdpSocket socket;

    sf::IpAddress recipient;
    unsigned short port;

    std::atomic<bool> isValid;

    std::atomic<bool> needClose;

    std::atomic<bool> closed;

    std::shared_mutex syncSocket;

    ProtocolParser* protoParser;

    std::vector<unsigned char> decrypt(data::buffer &buffer){
        std::vector<unsigned char> bufferVect(buffer.getData(), buffer.getData() + buffer.size());
        return decrypt(bufferVect);
    }

    std::vector<unsigned char> decrypt(std::vector<unsigned char> &buffer){
        std::vector<unsigned char> decrypted =  player::SelfImpl::getInstance().decrypt(buffer);
        return decrypted;
    }

    std::vector<unsigned char> decrypt(char* buffer, int size){
        std::vector<unsigned char> bufferVect(buffer, buffer + size);
        return decrypt(bufferVect);
    }

    bool deserialize(protocol::Server &server, sf::Packet &packet){
        bool validDecrypted = server.ParseFromArray(packet.getData(),packet.getDataSize());

        if(ConfigImpl::getInstance().isNeedCryptography()){
            if(validDecrypted){
                if(server.has_handshake() || server.has_invalidsession()){
                    return true;
                }
            }

            auto ucharptr = reinterpret_cast<unsigned char*>(const_cast<void*>(packet.getData()));
            std::vector<unsigned char> bufferToDrcp(ucharptr, ucharptr + packet.getDataSize());
            std::vector<unsigned char> buffer = decrypt(bufferToDrcp);
            bool isParsed = server.ParseFromArray(buffer.data(), buffer.size());
            return isParsed;
        }
        
        return validDecrypted;
    }

    public:

    socketUdp(){
        protoParser = nullptr;
        needClose = false;
        isValid = false;
        closed = true;
    }

    //just only one thread to be safe, call close first
    void open(sf::IpAddress &_ip, unsigned short &_port){
        if(isValid){
            close();
        }
        needClose = false;
        isValid = true;
        closed = true;
        recipient = _ip;
        port = _port;
        if(protoParser != nullptr){
            delete protoParser;
        }
        protoParser = new ProtocolParser();
        socket.bind(sf::Socket::AnyPort);
        socket.setBlocking(false);
        player::SelfImpl::getInstance().setNeedCallNewSession(false);
        std::thread(&socketUdp::receive, this).detach();
    }

    socketUdp(sf::IpAddress _ip, unsigned short _port){
        protoParser = nullptr;
        open(_ip, _port);
    }

    ~socketUdp(){
        close();
        if(protoParser != nullptr){
            delete protoParser;
        }
    }

    //thread safe
    bool isConnected(){
        return isValid;
    }

    //thread safe
    void close(){
        //Closing receive and socket
        needClose = true;
        std::cout << "wait receive close" << std::endl;
        while(!closed){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        needClose = false;
    }

    bool send(protocol::ClientBase& client){
        bool hasSecretID = false;
        if(client.has_clientext()){
            hasSecretID = client.mutable_clientext()->has_secret_id();
        }
        std::string buffer = client.SerializeAsString();
        return send(buffer, hasSecretID);
    }

    //thread safe
    bool send(std::string& buffer, bool has_secret_id){

        if(!isConnected()){
            return false;
        }

        void* data = buffer.data();
        int data_size = buffer.size();

        sf::Socket::Status status = sf::Socket::Status::NotReady;
        if(ConfigImpl::getInstance().isNeedCryptography() && !has_secret_id){
            std::vector<unsigned char> vecBuffer(buffer.data(), buffer.data() + buffer.size());
            std::vector<unsigned char> encrypted =  player::SelfImpl::getInstance().encrypt(vecBuffer);
            buffer = std::string(encrypted.data(), encrypted.data() + encrypted.size());
            data = buffer.data();
            data_size = buffer.size();
        }
        sf::IpAddress recipientL = recipient;
        unsigned short portL = port;

        syncSocket.lock_shared();
        if(!isConnected()){
            return false;
        }
        status = socket.send(data, data_size, recipientL, portL);
        syncSocket.unlock_shared();

        if(status != sf::Socket::Status::Done || status != sf::Socket::Status::NotReady){
            return false;
        }
        return true;
    }

    private:

    //Just call on receive
    void closeSocket(){
        std::cout << "locking" << std::endl;
        syncSocket.lock();
        socket.unbind();
        std::cout << "unbind" << std::endl;
        isValid = false;
        closed = true;
        needClose = false;
        std::cout << "instance update" << std::endl;
        player::SelfImpl::getInstance().setConnect(false);
        player::SelfImpl::getInstance().setNeedCallNewSession(true);
        std::cout << "updated" << std::endl;
        syncSocket.unlock();
    }

    void receive(){
        closed = false;
        while(!needClose){
            protocol::Server server;
            sf::Packet packet;
            
            const auto retryCount = 10;

            bool closeThread = false;

            sf::IpAddress recipientL = recipient;
            unsigned short portL = socket.getLocalPort();
            sf::Socket::Status status = socket.receive(packet, recipientL, portL);
            if(status == sf::Socket::Status::NotReady){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            for (auto receiveTries = 1; status != sf::Socket::Done; ++receiveTries)
            {
                recipientL = recipient;
                portL = socket.getLocalPort();

                if(status == sf::Socket::Status::Disconnected){
                    unsigned short lPort = socket.getLocalPort();
                    syncSocket.lock();
                    if(isValid){
                        socket.unbind();
                        socket.bind(lPort);
                    }
                    syncSocket.unlock();
                }
                if(needClose){
                    closeThread = true;
                    break;
                }
                if (receiveTries >= retryCount)
                {
                    std::cerr << "Unable to receive" << "\n";
                    closeThread = true;
                    break;
                }

                status = socket.receive(packet, recipientL, portL);
            }
            if(closeThread){ 
                break; 
            }

            bool isOk = deserialize(server, packet);

            if(isOk){
                protoParser->parse(server);
            }
        }
        closeSocket();
    }

};


class socketUdpImpl{
private:
    static bool initialized;
    static sf::IpAddress ip_;
    static unsigned short port_;
    static socketUdp* instance;

  public:
  static socketUdp& getInstance();

  static void fabric(sf::IpAddress ip, unsigned short port);
  static void refabric(sf::IpAddress ip, unsigned short port);
  static void refabric();

  static void close();
};

#endif