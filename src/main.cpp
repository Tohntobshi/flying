#include "flightController.h"
#include "debugSender.h"
#include "controlsReceiver.h"
#include "signal.h"
#include "frameSender.h"
#include <iostream>

int main()
{
  signal(SIGPIPE, SIG_IGN);

  // FrameSender frameSender;
  // frameSender.start();

  DebugSender debugger("8082");
  debugger.start();

  FlightController * flightControllerPtr = FlightController::Init(&debugger);
  flightControllerPtr->start();
  
  ControlsReceiver controlsReceiver(flightControllerPtr);
  controlsReceiver.start();


  debugger.join();

  flightControllerPtr->join();
  flightControllerPtr->Destroy();

  controlsReceiver.join();

  // frameSender.join();

  return 0;
}
