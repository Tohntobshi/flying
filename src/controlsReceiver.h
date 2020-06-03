#pragma once
#include "connection.h"
#include "separateLoop.h"
#include <iostream>
#include "flightController.h"

class ControlsReceiver: public SeparateLoop {
private:
  Connection conn;
  void iterate();
  void onStop();
  FlightController * flightController = nullptr;
public:
  ControlsReceiver(FlightController * flightController);
};
