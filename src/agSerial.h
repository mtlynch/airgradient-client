/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_SERIAL_H
#define AIRGRADIENT_SERIAL_H

#ifndef ESP8266

#include "driver/gpio.h"

#include "Wire.h"

#include "DFRobot_IICSerial.h"

// #define AGSERIAL_DEBUG

class AgSerial {
private:
  const char *const TAG = "AGSERIAL";
  bool _atLineOpened = false;
  TwoWire &_wire; // TODO: remove after DFRobot_IICSerial move to idf
  DFRobot_IICSerial *iicSerial_ = nullptr;
  gpio_num_t _iicResetIO = GPIO_NUM_NC;

public:
  AgSerial(TwoWire &wire);
  ~AgSerial();

  void init(int iicResetIO);
  bool open(int baud = 115200);
  void close();

  bool available();
  void print(const char *str);
  uint8_t read();
};

#endif // ESP8266
#endif // AIRGRADIENT_SERIAL_H
