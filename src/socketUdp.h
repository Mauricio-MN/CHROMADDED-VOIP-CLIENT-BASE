#ifndef SOCKETUDP_H // include guard
#define SOCKETUDP_H

#include <SFML/Network.hpp>
#include "data.h"
#include "proto/protocol.pb.h"

#include "cript.h"
#include "protoParse.h"
#include "thread"

class socketUdp{

private:

sf::UdpSocket socket;

sf::IpAddress recipient;
unsigned short port;

bool isValid;

bool needClose;

bool closed;

bool needEncrypt;

data::buffer decrypt(data::buffer &buffer){
  unsigned char* reinterpreted = reinterpret_cast<unsigned char*>(buffer.getData());

  std::vector<unsigned char> encrypted(reinterpreted, reinterpreted + buffer.size());
  std::vector<unsigned char> decrypted = CryptImpl::getInstance().decrypt(&encrypted);

  data::buffer output(reinterpret_cast<char*>(decrypted.data()), decrypted.size());
  return output;
}

data::buffer decrypt(char* buffer, int size){
  data::buffer data(buffer,size);
  return decrypt(data);
}

bool isGoodIntegrity(protocol::Server &server){
    if(server.integritycheck() != 956532){
        return false;
    }
    return true;
}

bool deserialize(protocol::Server &server, sf::Packet &packet){
    bool isParsed = server.ParseFromArray(packet.getData(),packet.getDataSize());
    bool falsePos = !isParsed;
    if(isParsed){
        falsePos = !isGoodIntegrity(server);
    }
    if(falsePos){
        data::buffer buffer = decrypt((char*)packet.getData(), packet.getDataSize());
        bool parse = server.ParseFromArray(buffer.getData(), buffer.size());
        if(parse){
            falsePos = !isGoodIntegrity(server);
        }
    }
    if(falsePos) return false;
    return true;
}

public:

socketUdp(){
    needClose = false;
    isValid = false;
    closed = false;
}

void init(sf::IpAddress &_ip, unsigned short &_port){
    needClose = false;
    isValid = true;
    closed = false;
    recipient = _ip;
    _port = port;
    if (socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
    {
        isValid = false;
    }
}

socketUdp(sf::IpAddress _ip, unsigned short _port){
    init(_ip, _port);
}

bool isConnected(){
    return isValid;
}

void close(){
    needClose = true;
    if(!isValid){
        closed = true;
    }
    while(!closed){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool initialise(){
    if(isValid){
        std::thread(&socketUdp::receive, this).detach();
    }
    return isValid;
}

bool send(protocol::Client& client){
    std::string buffer = client.SerializeAsString();

    if(needEncrypt){
        if(!client.has_secret_id()){
            std::vector<unsigned char> vecBuffer(buffer.data(), buffer.data() + buffer.size());
            std::vector<unsigned char> encrypted = CryptImpl::getInstance().encrypt(&vecBuffer);
            socket.send(encrypted.data(), encrypted.size(), recipient, port);
            return true;
        }
    }
    socket.send(buffer.data(), buffer.size(), recipient, port);
    return true;
}

void receive(){
    while(!needClose){
        protocol::Server server;

        sf::Packet packet;
        socket.receive(packet, recipient, port);

        bool isOk = deserialize(server, packet);

        if(isOk){
            //parse
        }
    }
    closed = true;
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