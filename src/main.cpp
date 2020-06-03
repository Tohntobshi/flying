#include <thread>
#include <chrono>
#include "connection.h"
#include "flightController.h"
#include <iostream>
#include "debugger.h"
#include "controlsReceiver.h"
#include "signal.h"
#include "frameSender.h"

int main()
{
  signal(SIGPIPE, SIG_IGN);

  FrameSender frameSender;
  frameSender.start();

  // Debugger debugger;
  // debugger.start();

  FlightController * flightControllerPtr = FlightController::Init(false, nullptr);
  flightControllerPtr->start();
  
  ControlsReceiver controlsReceiver(flightControllerPtr);
  controlsReceiver.start();


  // debugger.join();

  flightControllerPtr->join();
  flightControllerPtr->Destroy();

  controlsReceiver.join();

  frameSender.join();

  return 0;
}
