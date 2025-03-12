/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_CELLULAR_CLIENT_H
#define AIRGRADIENT_CELLULAR_CLIENT_H

#include <string>

#include "esp_log.h"

#include "airgradientClient.h"
#include "cellularModule.h"

class AirgradientCellularClient : public AirgradientClient {
private:
  const char *const TAG = "AgCellClient";
  std::string _apn = "iot.1nce.net";
  CellularModule *cell_ = nullptr;

public:
  AirgradientCellularClient(CellularModule *cellularModule);
  ~AirgradientCellularClient() {};

  bool begin();

  std::string httpFetchConfig(const std::string &sn);
  bool httpPostMeasures(const std::string &sn, const std::string &payload);
};

#endif // AIRGRADIENT_CELLULAR_CLIENT_H
