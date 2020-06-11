#pragma once
#include "packetsSender.h"

struct DebugInfo {
  bool noMoreInfo;
  float pitchError;
  float rollError;
  float pitchErrorChangeRate;
  float rollErrorChangeRate;
  float yawSpeedError;
  float yawSpeedErrorChangeRate;
};

class DebugSender: public PacketsSender {
public:
  using PacketsSender::PacketsSender;
  void sendInfo(DebugInfo info);
};
