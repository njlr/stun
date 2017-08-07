#include "stun/CommandCenter.h"

#include <stun/SessionHandler.h>

#include <common/Configerator.h>
#include <common/Util.h>
#include <event/Trigger.h>
#include <networking/IPTables.h>
#include <networking/Messenger.h>

#include <algorithm>

namespace stun {

using namespace networking;

CommandCenter::CommandCenter() : didDisconnect_(new event::BaseCondition()) {}

event::Condition* CommandCenter::didDisconnect() const {
  return didDisconnect_.get();
}

void CommandCenter::serve(ServerConfig config) {
  server_ = std::make_unique<class Server>(config);
}

void CommandCenter::connect(ClientConfig config) {
  TCPSocket client;
  client.connect(config.serverAddr);

  auto sessionConfig = SessionConfig{config.serverAddr, config.secret,
                                     config.encryption, config.paddingTo, 0};

  didDisconnect_->arm();
  std::unique_ptr<SessionHandler> handler{
      new SessionHandler(nullptr, ClientSession, sessionConfig,
                         std::make_unique<TCPSocket>(std::move(client)))};

  event::Trigger::arm({handler->didEnd()}, [this]() {
    LOG_I("Command") << "We are disconnected." << std::endl;
    didDisconnect_->fire();
    clientHandler_.reset();
  });

  clientHandler_ = std::move(handler);
}
}
