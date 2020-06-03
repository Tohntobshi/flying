#pragma once
#include "connection.h"
#include "separateLoop.h"
#include "frameCapturer.h"
#include <iostream>
#include <list>

class FrameSender: public SeparateLoop {
private:
  Connection conn;
  void beforeLoop();
  void iterate();
  void afterLoop();
  void onStop();
  bool sendError = false;
  FrameCapturer * frameCapturer = nullptr;
public:
};
