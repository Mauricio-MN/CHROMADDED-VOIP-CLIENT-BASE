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
        if(ConfigImpl::getInstance().isNeedCryptography()){
            auto ucharptr = reinterpret_cast<unsigned char*>(const_cast<void*>(packet.getData()));
            std::vector<unsigned char> bufferToDrcp(ucharptr, ucharptr + packet.getDataSize());
            std::vector<unsigned char> buffer = decrypt(bufferToDrcp);
            bool isParsed = server.ParseFromArray(buffer.data(), buffer.size());
            return isParsed;
        } else {
            bool isParsed = server.ParseFromArray(packet.getData(),packet.getDataSize());
            return isParsed;
        }
    }

    public:

    socketUdp(){
        needClose = false;
        isValid = false;
        closed = true;
    }

    //just only one thread to be safe, call close first
    void open(sf::IpAddress &_ip, unsigned short &_port){
        needClose = false;
        isValid = true;
        closed = true;
        recipient = _ip;
        port = _port;
        socket.bind(sf::Socket::AnyPort);
        std::thread(&socketUdp::receive, this).detach();
    }

    socketUdp(sf::IpAddress _ip, unsigned short _port){
        open(_ip, _port);
    }

    bool isConnected(){
        return isValid;
    }

    void close(){
        //Closing receive
        needClose = true;
        while(!closed){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        needClose = false;
    }

    bool send(protocol::Client& client){

        if(!isConnected()){
            return false;
        }

        std::string buffer = client.SerializeAsString();
        //client.SerializeToString();

        const void* data = buffer.data();
        int data_size = buffer.size();

        sf::Socket::Status status = sf::Socket::Status::NotReady;
        if(ConfigImpl::getInstance().isNeedCryptography() && !client.has_secret_id()){
            std::vector<unsigned char> vecBuffer(buffer.data(), buffer.data() + buffer.size());
            std::vector<unsigned char> encrypted =  player::SelfImpl::getInstance().encrypt(vecBuffer);
            data = encrypted.data();
        }
        sf::IpAddress recipientL = recipient;
        unsigned short portL = port;

        syncSocket.lock_shared();
        if(!isConnected()){
            return false;
        }
        status = socket.send(data, data_size, recipientL, portL);
        syncSocket.unlock_shared();

        if(status != sf::Socket::Status::Done){
            return false;
        }
        return true;
    }

    private:

    //Just call on receive
    void closeSocket(){
        syncSocket.lock();
        socket.unbind();
        isValid = false;
        closed = true;
        needClose = false;
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
            unsigned short portL = port;
            sf::Socket::Status status = socket.receive(packet, recipientL, portL);
            for (auto receiveTries = 1; status != sf::Socket::Done; ++receiveTries)
            {
                if(status == sf::Socket::Status::Disconnected){
                    unsigned short lPort = socket.getLocalPort();
                    syncSocket.lock();
                    socket.unbind();
                    socket.bind(lPort);
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
            if(closeThread) break;

            bool isOk = deserialize(server, packet);

            if(isOk){
                //parse
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
};

#endif