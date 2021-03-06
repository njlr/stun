#pragma once

#include <stun/ServerSessionHandler.h>

#include <event/Timer.h>
#include <networking/IPAddressPool.h>
#include <networking/TCPServer.h>

namespace stun {

using networking::SubnetAddress;
using networking::TCPServer;
using networking::IPAddressPool;

struct ServerConfig {
public:
  int port;
  SubnetAddress addressPool;
  bool encryption;
  std::string secret;
  size_t paddingTo;
  bool compression;
  event::Duration dataPipeRotationInterval;
  bool authentication;
  std::map<std::string, size_t> quotaTable;
  std::map<std::string, IPAddress> staticHosts;
};

class Server {
public:
  Server(ServerConfig config);

  std::unique_ptr<IPAddressPool> addrPool;

private:
  ServerConfig config_;

  std::unique_ptr<TCPServer> server_;
  std::unique_ptr<event::Action> listener_;
  std::vector<std::unique_ptr<ServerSessionHandler>> sessionHandlers_;

  void doAccept();

  // FIXME: This should really be a inner class instead;
  friend ServerSessionHandler;
};
}
