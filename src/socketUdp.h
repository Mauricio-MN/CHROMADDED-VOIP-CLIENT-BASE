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

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "proto/protocol.pb.h"
#include "proto/timestamp.pb.h"

static bool decodeAudioCallback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    auto* vec = reinterpret_cast<std::vector<uint8_t>*>(*arg);
    size_t len = stream->bytes_left;
    vec->resize(len);
    return pb_read(stream, vec->data(), len);
}

static bool decodeExtraClientCallback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    auto* vec = reinterpret_cast<std::vector<std::vector<uint8_t>>*>(*arg);

    size_t len = stream->bytes_left;

    std::vector<uint8_t> buffer(len);
    if (!pb_read(stream, buffer.data(), len)) {
        return false;
    }

    vec->push_back(std::move(buffer));
    return true;
}

struct ServerPacket {
    protocol_Server server_info;
    std::vector<uint8_t> audioBuffer;
    std::vector<std::vector<uint8_t>> extraClientBuffers;

    ServerPacket() {
        firstStart();
    }

    void firstStart(){
        server_info = protocol_Server_init_zero;
        server_info.audio.funcs.decode = &decodeAudioCallback;
        server_info.audio.arg = &audioBuffer;
        server_info.extraClientMSG.funcs.decode = &decodeExtraClientCallback;
        server_info.extraClientMSG.arg = &extraClientBuffers;
    }

    ServerPacket(const ServerPacket& other) {
        server_info = other.server_info;
        audioBuffer = other.audioBuffer;
        extraClientBuffers = other.extraClientBuffers;

        // dados já estão nos vectors, zera os callbacks
        server_info.audio.funcs.decode = nullptr;
        server_info.audio.arg = nullptr;
        server_info.extraClientMSG.funcs.decode = nullptr;
        server_info.extraClientMSG.arg = nullptr;
    }
};

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

    /*bool deserialize(protocol_Server &server, sf::Packet &packet){
        server
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
    }*/

    bool deserialize(ServerPacket &pkt, sf::Packet &packet) {
        auto tryDecode = [&](const void* data, size_t size) -> bool {
            // Reset completo e callbacks configurados ANTES do decode
            pkt.audioBuffer.clear();
            pkt.extraClientBuffers.clear();
            pkt.server_info = protocol_Server_init_zero;
            pkt.server_info.audio.funcs.decode = &decodeAudioCallback;
            pkt.server_info.audio.arg = &pkt.audioBuffer;
            pkt.server_info.extraClientMSG.funcs.decode = &decodeExtraClientCallback;
            pkt.server_info.extraClientMSG.arg = &pkt.extraClientBuffers;

            pb_istream_t stream = pb_istream_from_buffer(
                reinterpret_cast<const pb_byte_t*>(data), size);
            return pb_decode(&stream, protocol_Server_fields, &pkt.server_info);
        };

        if (ConfigImpl::getInstance().isNeedCryptography()) {
            if (tryDecode(packet.getData(), packet.getDataSize())) {
                if (pkt.server_info.has_handShake || pkt.server_info.has_InvalidSession) {
                    return true;
                }
            }
            auto ucharptr = reinterpret_cast<const unsigned char*>(packet.getData());
            std::vector<unsigned char> bufferToDrcp(ucharptr, ucharptr + packet.getDataSize());
            std::vector<unsigned char> buffer = decrypt(bufferToDrcp);
            return tryDecode(buffer.data(), buffer.size());
        }

        return tryDecode(packet.getData(), packet.getDataSize());
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

    /*bool send(protocol_ClientBase& client){
        bool hasSecretID = false;
        if(client.has_clientext()){
            hasSecretID = client.mutable_clientext()->has_secret_id();
        }
        std::string buffer = client.SerializeAsString();
        return send(buffer, hasSecretID);
    }*/

    bool send(protocol_ClientBase& client){
        bool hasSecretID = false;
        if(client.has_clientExt){
            hasSecretID = client.clientExt.has_secret_id;
        }

        uint8_t buffer[4096];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        if(!pb_encode(&stream, protocol_ClientBase_fields, &client)){
            return false;
        }

        return send(buffer, stream.bytes_written, hasSecretID);
    }

    //thread safe
    bool send(uint8_t* buffer, size_t buffer_size,  bool has_secret_id){

        if(!isConnected()){
            return false;
        }

        std::string strbuffer;

        void* data = buffer;
        int data_size = buffer_size;

        sf::Socket::Status status = sf::Socket::Status::NotReady;
        if(ConfigImpl::getInstance().isNeedCryptography() && !has_secret_id){
            std::vector<unsigned char> vecBuffer(buffer, buffer + buffer_size);
            std::vector<unsigned char> encrypted =  player::SelfImpl::getInstance().encrypt(vecBuffer);
            strbuffer = std::string(encrypted.data(), encrypted.data() + encrypted.size());
            data = strbuffer.data();
            data_size = strbuffer.size();
        }

        sf::IpAddress recipientL = recipient;
        unsigned short portL = port;

        syncSocket.lock_shared();
        if(!isConnected()){
            syncSocket.unlock_shared();
            return false;
        }

        int attempts = 0;
        while(attempts < 3){
            status = socket.send(data, data_size, recipientL, portL);

            if(status == sf::Socket::Status::Done){
                syncSocket.unlock_shared();
                return true;
            }

            if(status != sf::Socket::Status::NotReady){
                syncSocket.unlock_shared();
                return false; // erro real
            }

            std::this_thread::sleep_for(std::chrono::microseconds(200));
            attempts++;
        }

        syncSocket.unlock_shared();

        if(status != sf::Socket::Status::Done && status != sf::Socket::Status::NotReady){
            return false;
        }
        return true;
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

        int attempts = 0;
        while(attempts < 3){
            status = socket.send(data, data_size, recipientL, portL);

            if(status == sf::Socket::Status::Done){
                syncSocket.unlock_shared();
                return true;
            }

            if(status != sf::Socket::Status::NotReady){
                syncSocket.unlock_shared();
                return false; // erro real
            }

            std::this_thread::sleep_for(std::chrono::microseconds(200));
            attempts++;
        }

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
            ServerPacket pkt;
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

            bool isOk = deserialize(pkt, packet);

            if(isOk){
                protoParser->parse(pkt);
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