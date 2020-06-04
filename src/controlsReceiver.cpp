#include "controlsReceiver.h"


void ControlsReceiver::iterate()
{
  if (!conn.listenIP("8081"))
  {
    std::cout << "controls receiver: not listening, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  std::cout << "controls receiver: accepted connection...\n";
  while (!shouldStop())
  {
    Packet pk = conn.receivePacket();
    if (pk.size > 0)
    {
      int a = pk.data[0];
      std::cout << "received " << a <<"\n";
      if (flightController != nullptr)
      {
        flightController->command(pk.data, pk.size);
      }
      delete [] pk.data;
    }
    else
    {
      break;
    }
  }
}

void ControlsReceiver::onStop()
{
  conn.abortListenAndReceive();
}

ControlsReceiver::ControlsReceiver(FlightController * fc): flightController(fc)
{

}