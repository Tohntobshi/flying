#pragma once
#include <stdint.h>
#include "glm/glm.hpp"
#include "separateLoop.h"
#include "debugSender.h"

#define STOP_MOVING 0U
#define INCREASE 1U
#define DECREASE 2U
#define INCREASE_PROP_COEF 3U
#define DECREASE_PROP_COEF 4U
#define INCLINE_FORW 5U
#define INCLINE_BACKW 6U
#define INCLINE_LEFT 7U
#define INCLINE_RIGHT 8U
#define INCLINE_DEFAULT 9U
#define INCREASE_DER_COEF 10U
#define DECREASE_DER_COEF 11U
#define INCREASE_INT_COEF 12U
#define DECREASE_INT_COEF 13U
#define INCREASE_PITCH_BIAS 14U
#define DECREASE_PITCH_BIAS 15U
#define INCREASE_ROLL_BIAS 16U
#define DECREASE_ROLL_BIAS 17U

class FlightController: public SeparateLoop
{
private:
  static FlightController * instance;  
  FlightController(bool calibrate, DebugSender * debugger);
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
  void iterate();
  DebugSender * debugger = nullptr;
public:
  static FlightController * Init(bool calibrate, DebugSender * debugger);
  static void Destroy();
  void command(uint8_t* command, uint8_t size); // may be called from separate thread
};
