#pragma once
#include <stdint.h>
#include "glm/glm.hpp"
#include "separateLoop.h"
#include "debugSender.h"
#include "actions.h"

class FlightController: public SeparateLoop
{
private:
  static FlightController * instance;  
  FlightController(DebugSender * debugger);
  ~FlightController();
  int val = 0;
  
  void controlAll(uint32_t val);
  void controlAll(uint32_t fl, uint32_t fr, uint32_t bl, uint32_t br);
  int accGyroFD = -1;
  float aRes = 2.0 / 32768.0;
  float gRes = 250.0 / 32768.0;
  uint8_t readByte(uint8_t device, uint8_t reg);
  void readBytes(uint8_t device, uint8_t reg, uint8_t count, uint8_t * bytes);
  void writeByte(uint8_t device, uint8_t reg, uint8_t byte);
  glm::vec3 getAccData();
  glm::vec3 getGyroData();
  float prevRoll = 0;
  float prevPitch = 0;
  int64_t prevTimeStamp = 0;
  std::mutex commandMutex;
  void calibrate();
  void arm();
  float desiredPitch = 0.f;
  float desiredRoll = 0.f;
  float proportionalCoef = 0.5f;
  float derivativeCoef = 0.f;
  float integralCoef = 0.f;
  float pitchErrInt = 0.f;
  float rollErrInt = 0.f;
  float pitchBias = 0.f;
  float rollBias = 0.f;
  float accTrust = 0.1f;
  float prevValInfluence = 0.f;
  void iterate();
  DebugSender * debugger = nullptr;
public:
  static FlightController * Init(DebugSender * debugger);
  static void Destroy();
  void command(uint8_t command); // may be called from separate thread
  void setPropCoef(float val);
  void setDerCoef(float val);
  void setIntCoef(float val);
  void setPitchBias(float val);
  void setRollBias(float val);
  void setRPM(int val);
  void setAccTrust(float val);
  void setPrevValInfluence(float val);
};
