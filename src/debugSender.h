#pragma once
#include "packetsSender.h"

class DebugSender: public PacketsSender {
public:
  using PacketsSender::PacketsSender;
  void sendInfo(float pitchError, float rollError, int64_t timestamp);
};
