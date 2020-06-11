#pragma once
#include <stdint.h>
#include "glm/glm.hpp"
#include "separateLoop.h"
#include "debugSender.h"
#include "actions.h"
#include "bmp280.h"

class FlightController: public SeparateLoop
{
private:
  static FlightController * instance;  
  FlightController(DebugSender * debugger);
  ~FlightController();
  std::mutex commandMutex;
  void controlAll(uint32_t val);
  void controlAll(uint32_t fl, uint32_t fr, uint32_t bl, uint32_t br);
  int accGyroFD = -1;
  int magFD = -1;
  int barFD = -1;
  struct bmp280_dev bmp;
  float aRes = 2.0 / 32768.0;
  float gRes = 250.0 / 32768.0;
  float mRes = 4912.0 / 32768.0;
  glm::vec3 magAdjustment;
  glm::vec3 magMinVals;
  glm::vec3 magMaxVals;
  glm::vec3 magMidVals;
  static uint8_t readByte(uint8_t device, uint8_t reg);
  static int8_t readBytes(uint8_t device, uint8_t reg, uint8_t * bytes, uint16_t count);
  static void writeByte(uint8_t device, uint8_t reg, uint8_t byte);
  static int8_t writeBytes(uint8_t device, uint8_t reg, uint8_t * bytes, uint16_t count);
  glm::vec3 getAccData();
  glm::vec3 getGyroData();
  glm::vec3 getMagData();
  glm::vec3 getMagNormalizedData();
  float getBarData();
  void calibrate();
  void arm();

  int baseVal = 0;

  // desired values
  float desiredPitch = 0.f;
  float desiredRoll = 0.f;
  float desiredYawSpeed = 0.f;

  // previous iteration values
  int64_t prevTimeStamp = 0;
  float prevRoll = 0.f;
  float prevPitch = 0.f;
  float prevYawSpeed = 0.f;

  // PID coefficients for pitch
  float pitchPropCoef = 0.f;
  float pitchDerCoef = 0.f;
  float pitchIntCoef = 0.f;

  // PID coefficients for roll
  float rollPropCoef = 0.f;
  float rollDerCoef = 0.f;
  float rollIntCoef = 0.f;

  // PID coefficients for yaw speed
  float yawSpPropCoef = 0.f;
  float yawSpDerCoef = 0.f;
  float yawSpIntCoef = 0.f;

  float pitchErrInt = 0.f;
  float rollErrInt = 0.f;
  float yawSpeedErrInt = 0.f;

  float pitchBias = 0.f;
  float rollBias = 0.f;
  float yawSpeedBias = 0.f;

  float accTrust = 0.1f;
  float inclineChangeRateFilteringCoef = 0.f;
  float yawSpeedFilteringCoef = 0.f;
  float yawSpeedChangeRateFilteringCoef = 0.f;

  float prevPitchErrChangeRate = 0.f;
  float prevRollErrChangeRate = 0.f;
  float prevYawSpeedErrChangeRate = 0.f;

  bool onlyPositiveAdjustMode = true;

  void iterate();
  DebugSender * debugger = nullptr;
public:
  static FlightController * Init(DebugSender * debugger);
  static void Destroy();

  // controls below may be called from separate threads

  void command(uint8_t command);

  // void setIncPropCoef(float val);
  // void setIncDerCoef(float val);
  // void setIncIntCoef(float val);
  void setPitchPropCoef(float val);
  void setPitchDerCoef(float val);
  void setPitchIntCoef(float val);

  void setRollPropCoef(float val);
  void setRollDerCoef(float val);
  void setRollIntCoef(float val);

  void setYawSpPropCoef(float val);
  void setYawSpDerCoef(float val);
  void setYawSpIntCoef(float val);

  void setPitchBias(float val);
  void setRollBias(float val);
  void setYawSpeedBias(float val);

  void setBaseVal(int val);

  void setAccTrust(float val);
  void setIncChangeRateFilteringCoef(float val);
  void setYawSpFilteringCoef(float val);
  void setYawSpChangeRateFilteringCoef(float val);

  void setOnlyPositiveAdjustMode(bool val);
};
