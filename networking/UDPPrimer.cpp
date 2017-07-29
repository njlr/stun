#include "networking/UDPPrimer.h"

namespace networking {

UDPPrimer::UDPPrimer(UDPPipe& pipe) : outboundQ_(pipe.outboundQ.get()) {}

void UDPPrimer::start() {
  timer_.reset(new event::Timer(0));
  action_.reset(new event::Action({timer_->didFire()}));
  action_->callback = [this]() {
    UDPPacket packet;
    packet.pack(kUDPPrimerContent);
    outboundQ_->push(packet);
    timer_->extend(kUDPPrimerInterval);
  };
}

UDPPrimerAcceptor::UDPPrimerAcceptor(UDPPipe& pipe)
    : inboundQ_(pipe.inboundQ.get()) {}

void UDPPrimerAcceptor::start() {
  listener_.reset(new event::Action({inboundQ_->canPop()}));
  listener_->callback = [this]() {
    while (*inboundQ_->canPop()) {
      UDPPacket packet = inboundQ_->pop();
      if (packet.unpack<uint64_t>() == kUDPPrimerContent) {
        didFinish_.fire();
      }
    }
  };
}

event::Condition* UDPPrimerAcceptor::didFinish() { return &didFinish_; }
}