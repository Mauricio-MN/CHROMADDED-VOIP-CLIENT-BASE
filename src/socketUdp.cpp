#include "socketUdp.h"


  bool socketUdpImpl::initialized = false;
  sf::IpAddress socketUdpImpl::ip_ = nullptr;
  unsigned short socketUdpImpl::port_ = 0;
  socketUdp* socketUdpImpl::instance = new socketUdp();

  socketUdp& socketUdpImpl::getInstance(){
    if(initialized){
      return *instance;
    } else {
      perror("fabric Connection!");
    }
  }

  void socketUdpImpl::fabric(sf::IpAddress ip, unsigned short port){
    if(initialized){
        instance->close();
    }
    ip_ = ip;
    port_ = port;
    initialized = true;
    delete instance;
    instance = new socketUdp(ip, port);
  }

  void socketUdpImpl::refabric(sf::IpAddress ip, unsigned short port){
    fabric(ip, port);
  }

  void socketUdpImpl::refabric(){
    if(initialized){
        instance->close();
    }
    initialized = true;
    delete instance;
    instance = new socketUdp(ip_, port_);
  }