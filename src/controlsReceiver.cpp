#include "controlsReceiver.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

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
      parseCommand(pk.data);
      delete [] pk.data;
    }
    else
    {
      break;
    }
  }
}

void ControlsReceiver::parseCommand(uint8_t * data)
{
  Document d;
  d.Parse((char *)data);
  uint8_t type = d["t"].GetUint();
  if (flightController != nullptr)
  {
    if (type == INCLINE_FORW || type == INCLINE_LEFT || type == INCLINE_RIGHT || type == INCLINE_BACKW || type == INCLINE_DEFAULT || type == CALIBRATE || type == ARM)
    {
      flightController->command(type);
      return;
    }
    if (type == SET_RPM)
    {
      int val = d["v"].GetInt();
      flightController->setRPM(val);
      return;
    }
    if (type == SET_ROLL_BIAS)
    {
      float val = d["v"].GetFloat();
      flightController->setRollBias(val);
      return;
    }
    if (type == SET_PITCH_BIAS)
    {
      float val = d["v"].GetFloat();
      flightController->setPitchBias(val);
      return;
    }
    if (type == SET_DER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setDerCoef(val);
      return;
    }
    if (type == SET_INT_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setIntCoef(val);
      return;
    }
    if (type == SET_PROP_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setPropCoef(val);
      return;
    }
    if (type == SET_ACC_TRUST)
    {
      float val = d["v"].GetFloat();
      flightController->setAccTrust(val);
      return;
    }
    if (type == SET_PREV_VAL_INF)
    {
      float val = d["v"].GetFloat();
      flightController->setPrevValInfluence(val);
      return;
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