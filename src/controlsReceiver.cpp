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

    if (type == SET_PITCH_PROP_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setPitchPropCoef(val);
      return;
    }
    if (type == SET_PITCH_DER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setPitchDerCoef(val);
      return;
    }
    if (type == SET_PITCH_INT_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setPitchIntCoef(val);
      return;
    }
    if (type == SET_ROLL_PROP_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setRollPropCoef(val);
      return;
    }
    if (type == SET_ROLL_DER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setRollDerCoef(val);
      return;
    }
    if (type == SET_ROLL_INT_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setRollIntCoef(val);
      return;
    }
    if (type == SET_YAW_SP_PROP_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpPropCoef(val);
      return;
    }
    if (type == SET_YAW_SP_DER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpDerCoef(val);
      return;
    }
    if (type == SET_YAW_SP_INT_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpIntCoef(val);
      return;
    }
    if (type == SET_PITCH_BIAS)
    {
      float val = d["v"].GetFloat();
      flightController->setPitchBias(val);
      return;
    }
    if (type == SET_ROLL_BIAS)
    {
      float val = d["v"].GetFloat();
      flightController->setRollBias(val);
      return;
    }
    if (type == SET_YAW_SPEED_BIAS)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpeedBias(val);
      return;
    }
    if (type == SET_BASE_VAL)
    {
      int val = d["v"].GetInt();
      flightController->setBaseVal(val);
      return;
    }
    if (type == SET_ACC_TRUST)
    {
      float val = d["v"].GetFloat();
      flightController->setAccTrust(val);
      return;
    }
    if (type == SET_INC_CH_RATE_FILTER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setIncChangeRateFilteringCoef(val);
      return;
    }
    if (type == SET_YAW_SP_FILTER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpFilteringCoef(val);
      return;
    }
    if (type == SET_YAW_SP_CH_RATE_FILTER_COEF)
    {
      float val = d["v"].GetFloat();
      flightController->setYawSpChangeRateFilteringCoef(val);
      return;
    }
    if (type == SET_ONLY_POSITIVE_ADJUST_MODE)
    {
      int val = d["v"].GetInt();
      flightController->setOnlyPositiveAdjustMode((bool)val);
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