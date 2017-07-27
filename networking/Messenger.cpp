#include "networking/Messenger.h"

namespace networking {

typedef uint32_t MessengerLengthHeaderType;

Messenger::Messenger(TCPPipe& client)
    : bufferUsed_(0), inboundQ_(client.inboundQ.get()),
      outboundQ_(client.outboundQ.get()), client_(client) {}

void Messenger::start() {
  receiver_.reset(
      new event::Action({inboundQ_->canPop(), outboundQ_->canPush()}));
  receiver_->callback.setMethod<Messenger, &Messenger::doReceive>(this);
}

void Messenger::doReceive() {
  while (inboundQ_->canPop()->value) {
    TCPPacket in = inboundQ_->pop();

    // Append the new content to our buffer.
    assertTrue(bufferUsed_ + in.size <= kMessengerReceiveBufferSize,
               "Messenger receive buffer overflow.");
    memcpy(buffer_ + bufferUsed_, in.buffer, in.size);
    bufferUsed_ += in.size;

    // Deliver complete messages
    while (bufferUsed_ >= sizeof(MessengerLengthHeaderType)) {
      int messageLen = *((MessengerLengthHeaderType*)buffer_);
      int totalLen = sizeof(MessengerLengthHeaderType) + messageLen;
      if (bufferUsed_ < totalLen) {
        break;
      }

      // We have a complete message
      Message message;
      message.size = messageLen;
      memcpy(message.buffer, buffer_ + sizeof(MessengerLengthHeaderType),
             messageLen);

      if (bufferUsed_ > totalLen) {
        // Move the left-over to the front
        memmove(buffer_, buffer_ + totalLen, bufferUsed_ - (totalLen));
      }

      bufferUsed_ -= (totalLen);
      LOG() << "Messenger received: " << message.getType() << " - "
            << message.getBody() << std::endl;

      Message response = handler(message);
      if (response.size > 0) {
        send(response);
      }
    }
  }
}

event::Condition* Messenger::canSend() {
  return outboundQ_->canPush();
}

void Messenger::send(Message const& message) {
  // TODO: Think about large messages later
  assertTrue(message.size + sizeof(MessengerLengthHeaderType) <=
                 kPipePacketBufferSize,
             "Message too large");

  TCPPacket packet;
  packet.size = sizeof(MessengerLengthHeaderType) + message.size;
  *((MessengerLengthHeaderType*)packet.buffer) = message.size;
  memcpy(packet.buffer + sizeof(MessengerLengthHeaderType), message.buffer,
         message.size);

  client_.outboundQ->push(packet);
  LOG() << "Messenger sent: " << message.getType() << " - "
        << message.getBody() << std::endl;
}
}
