#pragma once
#include "connection.h"
#include "separateLoop.h"
#include <iostream>
#include <list>

class Debugger: public SeparateLoop {
private:
  Connection conn;
  std::mutex dataToSendMtx;
  void iterate();
  void onStop();
  std::list<std::string> debugDataToSend;
public:
  Debugger();
  void sendInfo(float pitchError, float rollError, int64_t timestamp);
};
