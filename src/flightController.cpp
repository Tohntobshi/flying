//#include <wiringPi.h>
//#include <softPwm.h>
#include <pigpio.h>
#include "flightController.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <math.h>

using namespace glm;

#define MPU9250_ADDRESS 0x68

#define ACCEL_XOUT_H 0x3B    // Accel data first register
#define GYRO_XOUT_H 0x43     // Gyro data first register

static unsigned int MIN_VAL = 1000;
static unsigned int MAX_VAL = 2000;

static unsigned int MOTOR_FL_PIN = 21;
static unsigned int MOTOR_FR_PIN = 20;
static unsigned int MOTOR_BL_PIN = 19;
static unsigned int MOTOR_BR_PIN = 26;

static vec3 flVec = normalize(vec3(-1.0, 1.0, 0.0));
static vec3 frVec = normalize(vec3(1.0, 1.0, 0.0));
static vec3 blVec = normalize(vec3(-1.0, -1.0, 0.0));
static vec3 brVec = normalize(vec3(1.0, -1.0, 0.0));

static float ACC_TRUST = 0.1f;

FlightController * FlightController::instance = nullptr;

FlightController * FlightController::Init(bool cal, DebugSender * deb)
{
  if (instance != nullptr)
  {
    return instance;
  }
  instance = new FlightController(cal, deb);
  return instance;
}

void FlightController::Destroy()
{
  delete instance;
  instance = nullptr;
}

FlightController::FlightController(bool cal, DebugSender * deb): debugger(deb)
{
  if (gpioInitialise() < 0)
  {
    std::cout << "cant init gpio\n";
  }
  accGyroFD = i2cOpen(1, MPU9250_ADDRESS, 0);
  if (accGyroFD < 0)
  {
    std::cout << "cant open accel/gyro sensor\n";
  }
  gpioSetMode(MOTOR_FL_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_FR_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_BL_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_BR_PIN, PI_OUTPUT);
  controlAll(0);
  if (cal) calibrate();
  arm();
}

FlightController::~FlightController()
{
  gpioTerminate();
}
void FlightController::command(uint8_t command)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  switch (command)
  {
  case INCREASE:
    val = (val >= MAX_VAL) ? MAX_VAL : val + 50;
    std::cout << "val " << val << "\n";
    break;
  case DECREASE:
    val = (val <= MIN_VAL) ? MIN_VAL : val - 50;
    std::cout << "val " << val << "\n";
    break;
  case INCREASE_PROP_COEF:
    proportionalCoef += 0.05f;
    std::cout << "proportionalCoef " << proportionalCoef << "\n";
    break;
  case DECREASE_PROP_COEF:
    proportionalCoef -= 0.05f;
    std::cout << "proportionalCoef " << proportionalCoef << "\n";
    break;
  case INCREASE_DER_COEF:
    derivativeCoef += 0.05f;
    std::cout << "derivativeCoef " << derivativeCoef << "\n";
    break;
  case DECREASE_DER_COEF:
    derivativeCoef -= 0.05f;
    std::cout << "derivativeCoef " << derivativeCoef << "\n";
    break;
  case STOP_MOVING:
    val = MIN_VAL;
    std::cout << "val " << val << "\n";
    break;
  case INCLINE_FORW:
    desiredPitch = -20.f;
    desiredRoll = 0.f;
    std::cout << "incline forw\n";
    break;
  case INCLINE_BACKW:
    desiredPitch = 20.f;
    desiredRoll = 0.f;
    std::cout << "incline backw\n";
    break;
  case INCLINE_LEFT:
    desiredPitch = 0.f;
    desiredRoll = 20.f;
    std::cout << "incline left\n";
    break;
  case INCLINE_RIGHT:
    desiredPitch = 0.f;
    desiredRoll = -20.f;
    std::cout << "incline right\n";
    break;
  case INCLINE_DEFAULT:
    desiredPitch = 0.f;
    desiredRoll = 0.f;
    std::cout << "incline default\n";
    break;
  }
}

void FlightController::arm()
{
  std::cout << "arm\n";
  controlAll(0);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  controlAll(MAX_VAL);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  controlAll(MIN_VAL);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "arm finished\n";
  val = MIN_VAL;
}

void FlightController::calibrate()
{
  std::cout << "calibrate\n";
  controlAll(0);
  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "calibrate max\n";
  controlAll(MAX_VAL);
  std::this_thread::sleep_for(std::chrono::seconds(6));
  std::cout << "calibrate min\n";
  controlAll(MIN_VAL);
  std::this_thread::sleep_for(std::chrono::seconds(6));
  std::cout << "calibrate finished\n";
  controlAll(0);
}

void FlightController::controlAll(uint32_t val)
{
  gpioServo(MOTOR_FL_PIN, val);
  gpioServo(MOTOR_FR_PIN, val);
  gpioServo(MOTOR_BL_PIN, val);
  gpioServo(MOTOR_BR_PIN, val);
}

void FlightController::controlAll(uint32_t fl, uint32_t fr, uint32_t bl, uint32_t br)
{
  gpioServo(MOTOR_FL_PIN, fl);
  gpioServo(MOTOR_FR_PIN, fr);
  gpioServo(MOTOR_BL_PIN, bl);
  gpioServo(MOTOR_BR_PIN, br);
}

uint8_t FlightController::readByte(uint8_t device, uint8_t reg)
{
  return i2cReadByteData(device, reg);
}

void FlightController::readBytes(uint8_t device, uint8_t reg, uint8_t count, uint8_t *bytes)
{
  for (uint8_t i = 0; i < count; i++)
  {
    bytes[i] = i2cReadByteData(device, reg + i);
  }
}

void FlightController::writeByte(uint8_t device, uint8_t reg, uint8_t byte)
{
  i2cWriteByteData(device, reg, byte);
}

vec3 FlightController::getAccData()
{
  uint8_t rawData[6];
  readBytes(accGyroFD, ACCEL_XOUT_H, 6, rawData);
  int16_t rawX = ((int16_t)rawData[0] << 8) | rawData[1];
  int16_t rawY = ((int16_t)rawData[2] << 8) | rawData[3];
  int16_t rawZ = ((int16_t)rawData[4] << 8) | rawData[5];
  vec3 result;

  result.x = (float)rawX *aRes;
  result.y = (float)rawY *aRes;
  result.z = (float)rawZ *aRes;
  return result;
}

vec3 FlightController::getGyroData()
{
  uint8_t rawData[6];
  readBytes(accGyroFD, GYRO_XOUT_H, 6, rawData);
  int16_t rawX = ((int16_t)rawData[0] << 8) | rawData[1];
  int16_t rawY = ((int16_t)rawData[2] << 8) | rawData[3];
  int16_t rawZ = ((int16_t)rawData[4] << 8) | rawData[5];
  vec3 result;

  result.x = (float)rawX*gRes;
  result.y = (float)rawY*gRes;
  result.z = (float)rawZ*gRes;
  return result;
}

void FlightController::iterate()
{
  vec3 acc = normalize(getAccData());
  vec3 gyro = getGyroData();
  float accPitch = degrees(atan2(acc.x, acc.z)) + pitchBias;
  float accRoll = -degrees(atan2(acc.y, acc.z)) + rollBias;
  int64_t currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  float secondsElapsed = (double)(currentTimestamp - prevTimeStamp) / 1000.0;
  float pitchChangeRate = - gyro.y;
  float rollChangeRate = - gyro.x;
  float currentPitch = accPitch * ACC_TRUST + (1.f - ACC_TRUST) * (prevPitch + pitchChangeRate * secondsElapsed);
  float currentRoll = accRoll * ACC_TRUST + (1.f - ACC_TRUST) * (prevRoll + rollChangeRate * secondsElapsed);
  
  // std::cout << "pitch " << currentPitch << " roll " << currentRoll << " seconds elapsed " << secondsElapsed << "\n";
  std::unique_lock<std::mutex> lck(commandMutex);
  // // one implementation
  // float joyPitch = degrees(atan2(joystickPos.y, joystickPos.z));
  // float joyRoll = degrees(atan2(joystickPos.x, joystickPos.z));
  // // std::cout << "pitch " << currentPitch << " roll " << currentRoll << " joyPitch " << joyPitch << " joyRoll " << joyRoll << "\n";
  // vec3 top;
  // float pitchTan = tan(radians(currentPitch + joyPitch));
  // float rollTan = tan(radians(currentRoll + joyRoll));
  // // this will not work upside down but it is not needed now
  // top.z = sqrt( 1.0 / ( pow(pitchTan, 2) + pow(rollTan, 2) + 1 ) );
  // top.x = rollTan * top.z;
  // top.y = pitchTan * top.z;
  // // std::cout << "top x " << top.x << " y " << top.y << " z " << top.z << "\n";
  
  // int flAdjust = -dot(flVec, top) * (val - MIN_VAL) * (sensorSensitivity / 100.0);
  // int frAdjust = -dot(frVec, top) * (val - MIN_VAL) * (sensorSensitivity / 100.0);
  // int blAdjust = -dot(blVec, top) * (val - MIN_VAL) * (sensorSensitivity / 100.0);
  // int brAdjust = -dot(brVec, top) * (val - MIN_VAL) * (sensorSensitivity / 100.0);
  // // std::cout << "fl " << flAdjust << " fr " << frAdjust << " bl " << blAdjust << " br " << brAdjust << "\n";
  
  // controlAll(val + flAdjust, val + frAdjust, val + blAdjust, val + brAdjust);
  float currentPitchError = desiredPitch - currentPitch;
  float currentRollError = desiredRoll - currentRoll;
  float prevPitchError = desiredPitch - prevPitch;
  float prevRollError = desiredRoll - prevRoll;
  float pitchErrorChangeRate = (currentPitchError - prevPitchError) / secondsElapsed;
  float rollErrorChangeRate = (currentRollError - prevRollError) / secondsElapsed;
  pitchErrInt += currentPitchError * secondsElapsed;
  rollErrInt += currentRollError * secondsElapsed; 

  int pitchAdjust = currentPitchError * proportionalCoef + pitchErrorChangeRate * derivativeCoef + pitchErrInt * integralCoef;
  int rollAdjust = currentRollError * proportionalCoef + rollErrorChangeRate * derivativeCoef + rollErrInt * integralCoef;
  if (val <= MIN_VAL)
  {
    controlAll(MIN_VAL);
    // std::cout << "pitchAdjust " << pitchAdjust << " rollAdjust " << rollAdjust << "\n";
  }
  else
  {
    controlAll(val + pitchAdjust - rollAdjust, val + pitchAdjust + rollAdjust, val - pitchAdjust - rollAdjust, val - pitchAdjust + rollAdjust);
  }
  prevPitch = currentPitch;
  prevRoll = currentRoll;
  prevTimeStamp = currentTimestamp;
  lck.unlock();
  if (debugger != nullptr)
  {
    debugger->sendInfo(currentPitchError, currentRollError, currentTimestamp);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
