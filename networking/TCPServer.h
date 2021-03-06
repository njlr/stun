#pragma once

#include <networking/TCPSocket.h>

namespace networking {

class TCPServer : private TCPSocket {
public:
  using TCPSocket::bind;

  TCPSocket accept();
  event::Condition* canAccept() const;
};
}
