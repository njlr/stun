#include "UDPServer.h"

#include <Util.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

namespace stun {

UDPServer::UDPServer(int port) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  int err;
  if ((err = getaddrinfo("0.0.0.0", std::to_string(port).c_str(), &hints, &myAddr_)) != 0) {
    throwGetAddrInfoError(err);
  }

  socket_ = socket(myAddr_->ai_family, myAddr_->ai_socktype, myAddr_->ai_protocol);
  if (socket_ < 0) {
    throwUnixError("creating UDPServer's socket");
  }
}

void UDPServer::bind() {
  int ret = ::bind(socket_, myAddr_->ai_addr, myAddr_->ai_addrlen);
  if (ret < 0) {
    throwUnixError("binding to UDPServer's socket");
  }

  fcntl(socket_, F_SETFL, fcntl(socket_, F_GETFL, 0) | O_NONBLOCK);

  io_.set<UDPServer, &UDPServer::doReceive>(this);
  io_.start(socket_, ev::READ);
}

void UDPServer::doReceive(ev::io& watcher, int events) {
  if (events & EV_ERROR) {
    throwUnixError("UDPServer doReceive()");
  }

  UDPPacket packet;
  int ret = recv(socket_, packet.data, kUDPPacketBufferSize, 0);
  if (ret < 0) {
    throwUnixError("receiving a UDP packet");
  }
  packet.size = ret;

  onReceive(packet);
}

UDPServer::~UDPServer() {
  close(socket_);
  freeaddrinfo(myAddr_);
}

}
