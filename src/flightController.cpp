#include <pigpio.h>
#include "flightController.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <math.h>


using namespace glm;

#define MPU9250_ADDRESS 0x68
#define AK8963_ADDRESS  0x0C

#define ACCEL_XOUT_H 0x3B    // Accel data first register
#define GYRO_XOUT_H 0x43     // Gyro data first register
#define USER_CTRL 0x6A       // MPU9250 config
#define INT_PIN_CFG 0x37     // MPU9250 config

#define AK8963_CNTL 0x0A  // Mag config
#define AK8963_XOUT_L 0x03  // Mag data first register
#define AK8963_ASAX 0x10 // Mag adj vals first register

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

FlightController * FlightController::instance = nullptr;

FlightController * FlightController::Init(DebugSender * deb)
{
  if (instance != nullptr)
  {
    return instance;
  }
  instance = new FlightController(deb);
  return instance;
}

void FlightController::Destroy()
{
  delete instance;
  instance = nullptr;
}

FlightController::FlightController(DebugSender * deb): debugger(deb)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  if (gpioInitialise() < 0)
  {
    std::cout << "cant init gpio\n";
  }


  accGyroFD = i2cOpen(1, MPU9250_ADDRESS, 0);
  if (accGyroFD < 0)
  {
    std::cout << "cant open accel/gyro sensor\n";
  }
  uint8_t intPinCfg = readByte(accGyroFD, INT_PIN_CFG);


  writeByte(accGyroFD, INT_PIN_CFG, intPinCfg | 0b00000010);  // set pass though mode for mpu9250 in order to have direct access to magnetometer
  magFD = i2cOpen(1, AK8963_ADDRESS, 0);
  if (magFD < 0)
  {
    std::cout << "cant open mag sensor\n";
  }
  writeByte(magFD, AK8963_CNTL, 0b00001111);  // set rom mode for magnetometer
  uint8_t rawMagAdjVals[3];
  readBytes(magFD, AK8963_ASAX, rawMagAdjVals, 3);
  magAdjustment.x = (float)(rawMagAdjVals[0] - 128) / 128.f + 1.f;
  magAdjustment.y = (float)(rawMagAdjVals[1] - 128) / 128.f + 1.f;
  magAdjustment.z = (float)(rawMagAdjVals[2] - 128) / 128.f + 1.f;
  writeByte(magFD, AK8963_CNTL, 0b00010110);  // set 16bit continious measurement mode for magnetometer
  vec3 magVals = getMagData();
  magMinVals = magVals;
  magMaxVals = magVals;


  barFD = i2cOpen(1, BMP280_I2C_ADDR_PRIM, 0);
  if (barFD < 0)
  {
    std::cout << "cant open bar sensor\n";
  }
  bmp.delay_ms = [](uint32_t period_ms) { std::this_thread::sleep_for(std::chrono::milliseconds(period_ms)); };
  bmp.dev_id = barFD;
  bmp.read = readBytes;
  bmp.write = writeBytes;
  bmp.intf = BMP280_I2C_INTF;
  bmp280_init(&bmp);
  bmp280_config bmpConf;
  bmp280_get_config(&bmpConf, &bmp);
  bmpConf.filter = BMP280_FILTER_COEFF_2;
  bmpConf.os_pres = BMP280_OS_4X;
  bmpConf.odr = BMP280_ODR_62_5_MS;
  bmp280_set_config(&bmpConf, &bmp);
  bmp280_set_power_mode(BMP280_NORMAL_MODE, &bmp);



  gpioSetMode(MOTOR_FL_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_FR_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_BL_PIN, PI_OUTPUT);
  gpioSetMode(MOTOR_BR_PIN, PI_OUTPUT);
  controlAll(0);
}

FlightController::~FlightController()
{
  gpioTerminate();
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
  baseVal = MIN_VAL;
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
  baseVal = 0;
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

int8_t FlightController::readBytes(uint8_t device, uint8_t reg, uint8_t *bytes, uint16_t count)
{
  int res = i2cReadI2CBlockData(device, reg, (char *)bytes, count);
  return res > 0 ? 0 : -1;
}

void FlightController::writeByte(uint8_t device, uint8_t reg, uint8_t byte)
{
  i2cWriteByteData(device, reg, byte);
}


int8_t FlightController::writeBytes(uint8_t device, uint8_t reg, uint8_t *bytes, uint16_t count)
{
  int res = i2cWriteI2CBlockData(device, reg, (char *)bytes, count);
  return res == 0 ? 0 : -1;
}

vec3 FlightController::getAccData()
{
  uint8_t rawData[6];
  readBytes(accGyroFD, ACCEL_XOUT_H, rawData, 6);
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
  readBytes(accGyroFD, GYRO_XOUT_H, rawData, 6);
  int16_t rawX = ((int16_t)rawData[0] << 8) | rawData[1];
  int16_t rawY = ((int16_t)rawData[2] << 8) | rawData[3];
  int16_t rawZ = ((int16_t)rawData[4] << 8) | rawData[5];
  vec3 result;

  result.x = (float)rawX*gRes;
  result.y = (float)rawY*gRes;
  result.z = (float)rawZ*gRes;
  return result;
}

vec3 FlightController::getMagData()
{
  uint8_t rawData[7];
  readBytes(magFD, AK8963_XOUT_L, rawData, 7);
  if((rawData[6] & 0x08))
  {
    // data corrupted, do something
  }
  int16_t rawX = ((int16_t)rawData[1] << 8) | rawData[0];
  int16_t rawY  = ((int16_t)rawData[3] << 8) | rawData[2];
  int16_t rawZ = ((int16_t)rawData[5] << 8) | rawData[4];
  vec3 result;
  result.x = (float)rawX * mRes * magAdjustment.x;
  result.y = (float)rawY * mRes * magAdjustment.y;
  result.z = (float)rawZ * mRes * magAdjustment.z;
  return result;
}

vec3 FlightController::getMagNormalizedData()
{
  vec3 result = getMagData();
  if (result.x < magMinVals.x) magMinVals.x = result.x;
  if (result.x > magMaxVals.x) magMaxVals.x = result.x;
  if (result.y < magMinVals.y) magMinVals.y = result.y;
  if (result.y > magMaxVals.y) magMaxVals.y = result.y;
  if (result.z < magMinVals.z) magMinVals.z = result.z;
  if (result.z > magMaxVals.z) magMaxVals.z = result.z;
  magMidVals.x = (magMaxVals.x + magMinVals.x) / 2.f;
  magMidVals.y = (magMaxVals.y + magMinVals.y) / 2.f;
  magMidVals.z = (magMaxVals.z + magMinVals.z) / 2.f;
  float xRange = abs(magMaxVals.x - magMinVals.x) / 2.f;
  float yRange = abs(magMaxVals.y - magMinVals.y) / 2.f;
  float zRange = abs(magMaxVals.z - magMinVals.z) / 2.f;
  vec3 noralizedSeparately({ (result.x - magMidVals.x) / xRange, (result.y - magMidVals.y) / yRange, (result.z - magMidVals.z) / zRange });
  return normalize(noralizedSeparately);
}

float FlightController::getBarData()
{
  bmp280_uncomp_data ucomp_data;
  double pres;
  bmp280_get_uncomp_data(&ucomp_data, &bmp);
  bmp280_get_comp_pres_double(&pres, ucomp_data.uncomp_press, &bmp);
  return pres;
}

void FlightController::iterate()
{
  std::unique_lock<std::mutex> lck(commandMutex);
  vec3 acc = normalize(getAccData());
  vec3 gyro = getGyroData();
  vec3 mag = getMagNormalizedData();
  float pressure = getBarData();
  
  float accPitch = degrees(atan2(acc.x, acc.z)) + pitchBias;
  float accRoll = -degrees(atan2(acc.y, acc.z)) + rollBias;
  float magYaw = degrees(atan2(mag.y, mag.x));

  // std::cout << "yaw " << magYaw << " mag x " << mag.x << " mag y " << mag.y << " mag z " << mag.z << "\n";

  int64_t currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  float secondsElapsed = (double)(currentTimestamp - prevTimeStamp) / 1000.0;

  float pitchChangeRate = - gyro.y;
  float rollChangeRate = - gyro.x;

  float currentYawSpeed = (gyro.z + yawSpeedBias) * (1.f - yawSpeedFilteringCoef) + prevYawSpeed * yawSpeedFilteringCoef;

  float currentPitch = accPitch * accTrust + (1.f - accTrust) * (prevPitch + pitchChangeRate * secondsElapsed);
  float currentRoll = accRoll * accTrust + (1.f - accTrust) * (prevRoll + rollChangeRate * secondsElapsed);

  float currentPitchError = desiredPitch - currentPitch;
  float currentRollError = desiredRoll - currentRoll;
  float currentYawSpeedError = desiredYawSpeed - currentYawSpeed;
  float prevPitchError = desiredPitch - prevPitch;
  float prevRollError = desiredRoll - prevRoll;
  float prevYawSpeedError = desiredYawSpeed - prevYawSpeed;

  float pitchErrorChangeRate = -pitchChangeRate * (1.f - inclineChangeRateFilteringCoef) + prevPitchErrChangeRate * inclineChangeRateFilteringCoef;
  float rollErrorChangeRate = -rollChangeRate * (1.f - inclineChangeRateFilteringCoef) + prevRollErrChangeRate * inclineChangeRateFilteringCoef;
  float yawSpeedErrorChangeRate = ((currentYawSpeedError - prevYawSpeedError) / secondsElapsed) * (1.f - yawSpeedChangeRateFilteringCoef) + prevYawSpeedErrChangeRate * yawSpeedChangeRateFilteringCoef;
  
  pitchErrInt += currentPitchError * secondsElapsed;
  rollErrInt += currentRollError * secondsElapsed;
  yawSpeedErrInt += currentYawSpeedError * secondsElapsed;

  int pitchAdjust = currentPitchError * pitchPropCoef + pitchErrorChangeRate * pitchDerCoef + pitchErrInt * pitchIntCoef;
  int rollAdjust = currentRollError * rollPropCoef + rollErrorChangeRate * rollDerCoef + rollErrInt * rollIntCoef;
  int yawAdjust = currentYawSpeedError * yawSpPropCoef + yawSpeedErrorChangeRate * yawSpDerCoef + yawSpeedErrInt * yawSpIntCoef;

  if (baseVal < MIN_VAL)
  {
    controlAll(0);
    pitchErrInt = 0.f;
    rollErrInt = 0.f;
    yawSpeedErrInt = 0.f;
  }
  else if (baseVal < MIN_VAL + 300)
  {
    controlAll(baseVal);
    pitchErrInt = 0.f;
    rollErrInt = 0.f;
    yawSpeedErrInt = 0.f;
  }
  else
  {
    if (onlyPositiveAdjustMode)
    {
      int fronLeft = baseVal + (pitchAdjust > 0 ? pitchAdjust : 0) - (rollAdjust < 0 ? rollAdjust : 0) + (yawAdjust > 0 ? yawAdjust : 0);
      int frontRight =  baseVal + (pitchAdjust > 0 ? pitchAdjust : 0) + (rollAdjust > 0 ? rollAdjust : 0) - (yawAdjust < 0 ? yawAdjust : 0);
      int backLeft = baseVal - (pitchAdjust < 0 ? pitchAdjust : 0) - (rollAdjust < 0 ? rollAdjust : 0) - (yawAdjust < 0 ? yawAdjust : 0);
      int backRight = baseVal - (pitchAdjust < 0 ? pitchAdjust : 0) + (rollAdjust > 0 ? rollAdjust : 0) + (yawAdjust > 0 ? yawAdjust : 0);
      controlAll(fronLeft, frontRight, backLeft, backRight);
    }
    else
    {
      int fronLeft = baseVal + pitchAdjust - rollAdjust + yawAdjust;
      int frontRight =  baseVal + pitchAdjust + rollAdjust - yawAdjust;
      int backLeft = baseVal - pitchAdjust - rollAdjust - yawAdjust;
      int backRight = baseVal - pitchAdjust + rollAdjust + yawAdjust;
      controlAll(fronLeft, frontRight, backLeft, backRight);
    }
    
  }
  prevPitch = currentPitch;
  prevRoll = currentRoll;
  prevYawSpeed = currentYawSpeed;
  prevTimeStamp = currentTimestamp;
  prevPitchErrChangeRate = pitchErrorChangeRate;
  prevRollErrChangeRate = rollErrorChangeRate;
  prevYawSpeedErrChangeRate = yawSpeedErrorChangeRate;
  lck.unlock();
  if (debugger != nullptr)
  {
    debugger->sendInfo({ false, currentPitchError, currentRollError, pitchErrorChangeRate, rollErrorChangeRate, currentYawSpeedError, yawSpeedErrorChangeRate });
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void FlightController::command(uint8_t command)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  switch (command)
  {
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
  case ARM:
    arm();
    break;
  case CALIBRATE:
    calibrate();
    break;
  }
}


void FlightController::setPitchPropCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  pitchPropCoef = value;
}

void FlightController::setPitchDerCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  pitchDerCoef = value;
}

void FlightController::setPitchIntCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  pitchIntCoef = value;
  pitchErrInt = 0.f;
}

void FlightController::setRollPropCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  rollPropCoef = value;
}

void FlightController::setRollDerCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  rollDerCoef = value;
}

void FlightController::setRollIntCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  rollIntCoef = value;
  rollErrInt = 0.f;
}


void FlightController::setYawSpPropCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  yawSpPropCoef = value;
}

void FlightController::setYawSpDerCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  yawSpDerCoef = value;
}

void FlightController::setYawSpIntCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  yawSpIntCoef = value;
  yawSpeedErrInt = 0.f;
}

void FlightController::setPitchBias(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  pitchBias = value;
}

void FlightController::setRollBias(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  rollBias = value;
}

void FlightController::setYawSpeedBias(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  rollBias = value;
}

void FlightController::setBaseVal(int value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  baseVal = value;
}

void FlightController::setAccTrust(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  accTrust = value;
}

void FlightController::setIncChangeRateFilteringCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  inclineChangeRateFilteringCoef = value;
}

void FlightController::setYawSpFilteringCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  yawSpeedFilteringCoef = value;
}

void FlightController::setYawSpChangeRateFilteringCoef(float value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  yawSpeedChangeRateFilteringCoef = value;
}

void FlightController::setOnlyPositiveAdjustMode(bool value)
{
  std::unique_lock<std::mutex> lck(commandMutex);
  onlyPositiveAdjustMode = value;
}