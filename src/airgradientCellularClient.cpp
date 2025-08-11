/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include <cmath>
#ifndef ESP8266

#include "airgradientCellularClient.h"
#include <sstream>
#include "cellularModule.h"
#include "common.h"
#include "agLogger.h"
#include "config.h"

#define ONE_OPENAIR_POST_MEASURES_ENDPOINT "cts"
#define OPENAIR_MAX_POST_MEASURES_ENDPOINT "cvn"
#ifdef ARDUINO
#define POST_MEASURES_ENDPOINT ONE_OPENAIR_POST_MEASURES_ENDPOINT
#else
#define POST_MEASURES_ENDPOINT OPENAIR_MAX_POST_MEASURES_ENDPOINT
#endif

AirgradientCellularClient::AirgradientCellularClient(CellularModule *cellularModule)
    : cell_(cellularModule) {}

bool AirgradientCellularClient::begin(std::string sn) {
  // Update parent serialNumber variable
  serialNumber = sn;
  clientReady = false;

  if (!cell_->init()) {
    AG_LOGE(TAG, "Cannot initialized cellular client");
    return false;
  }

  // To make sure module ready to use
  if (cell_->isSimReady() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "SIM is not ready, please check if SIM is inserted properly!");
    return false;
  }

  // Printout SIM CCID
  auto result = cell_->retrieveSimCCID();
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed get SIM CCID, please check if SIM is inserted properly!");
    return false;
  }
  _iccid = result.data;
  AG_LOGI(TAG, "SIM CCID: %s", result.data.c_str());

  // Register network
  result =
      cell_->startNetworkRegistration(CellTechnology::Auto, _apn, _networkRegistrationTimeoutMs);
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Cellular client failed, module cannot register to network");
    return false;
  }

  AG_LOGI(TAG, "Cellular client ready, module registered to network");
  clientReady = true;

  return true;
}

void AirgradientCellularClient::setNetworkRegistrationTimeoutMs(int timeoutMs) {
  _networkRegistrationTimeoutMs = timeoutMs;
  ESP_LOGI(TAG, "Timeout set to %d seconds", (_networkRegistrationTimeoutMs / 1000));
}

std::string AirgradientCellularClient::getICCID() { return _iccid; }

bool AirgradientCellularClient::ensureClientConnection(bool reset) {
  AG_LOGE(TAG, "Ensuring client connection, restarting cellular module");
  if (reset) {
    if (cell_->reset() == false) {
      AG_LOGW(TAG, "Reset failed, power cycle module...");
      cell_->powerOff(true);
      DELAY_MS(2000);
      cell_->powerOn();
    }

    AG_LOGI(TAG, "Wait for 10s for module to warming up");
    DELAY_MS(10000);
  }

  if (cell_->reinitialize() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed to reinitialized the cellular module");
    clientReady = false;
    return false;
  }

  // Register network
  auto result =
      cell_->startNetworkRegistration(CellTechnology::Auto, _apn, _networkRegistrationTimeoutMs);
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Cellular client failed, module cannot register to network");
    clientReady = false;
    return false;
  }

  clientReady = true;
  AG_LOGI(TAG, "Cellular client ready, module registered to network");

  return true;
}

std::string AirgradientCellularClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl();
  AG_LOGI(TAG, "Fetch configuration from %s", url.c_str());

  auto result = cell_->httpGet(url); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Module not return OK when call httpGet()");
    lastFetchConfigSucceed = false;
    clientReady = false;
    return {};
  }

  // Reset client ready state
  clientReady = true;

  // Response status check if fetch failed
  if (result.data.statusCode != 200) {
    AG_LOGW(TAG, "Failed fetch configuration from server with return code %d",
            result.data.statusCode);
    // Return code 400 means device not registered on ag server
    if (result.data.statusCode == 400) {
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;

    return {};
  }

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;

  // Sanity check if response body is empty
  if (result.data.bodyLen == 0 || result.data.body == nullptr) {
    // TODO: How to handle this? perhaps cellular module failed to read the buffer
    AG_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    return {};
  }

  AG_LOGI(TAG, "Received configuration: (%d) %s", result.data.bodyLen, result.data.body.get());

  // Move the string from unique_ptr
  std::string body = std::string(result.data.body.get());

  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return body;
}

bool AirgradientCellularClient::httpPostMeasures(const std::string &payload) {
  char url[80] = {0};
  sprintf(url, "http://%s/sensors/%s/%s", httpDomain.c_str(), serialNumber.c_str(),
          POST_MEASURES_ENDPOINT);
  AG_LOGI(TAG, "Post measures to %s", url);
  AG_LOGI(TAG, "Payload: %s", payload.c_str());

  auto result = cell_->httpPost(url, payload); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Module not return OK when call httpPost()");
    lastPostMeasuresSucceed = false;
    clientReady = false;
    return false;
  }

  // Reset client ready state
  clientReady = true;

  // Response status check if post failed
  if ((result.data.statusCode != 200) && (result.data.statusCode != 429) &&
      (result.data.statusCode != 201)) {
    AG_LOGW(TAG, "Failed post measures to server with response code %d", result.data.statusCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  AG_LOGI(TAG, "Success post measures to server with response code %d", result.data.statusCode);

  return true;
}

bool AirgradientCellularClient::httpPostMeasures(int measureInterval,
                                                 const std::vector<OpenAirMaxPayload> &data) {
  // Build payload using oss, easier to manage if there's an invalid value that should not included
  std::ostringstream payload;

  // Add interval at the first position
  payload << measureInterval;

  for (const OpenAirMaxPayload &v : data) {
    // Seperator between measures cycle
    payload << ",";
    // Serialize each measurement
    _serialize(payload, v.rco2, v.particleCount003, v.pm01, v.pm25, v.pm10, v.tvocRaw, v.noxRaw,
               v.atmp, v.rhum, v.signal, v.vBat, v.vPanel, v.o3WorkingElectrode,
               v.o3AuxiliaryElectrode, v.no2WorkingElectrode, v.no2AuxiliaryElectrode, v.afeTemp);
  }

  // Compile it
  std::string toSend = payload.str();

  return httpPostMeasures(toSend);
}

bool AirgradientCellularClient::mqttConnect() {
  auto result = cell_->mqttConnect(serialNumber, mqttDomain, mqttPort);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed connect to airgradient mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success connect to airgardient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttDisconnect() {
  if (cell_->mqttDisconnect() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed disconnect from airgradient mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success disconnect from airgradient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttPublishMeasures(const std::string &payload) {
  // TODO: Ensure mqtt connection
  auto topic = buildMqttTopicPublishMeasures();
  auto result = cell_->mqttPublish(topic, payload);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed publish measures to mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success publish measures to mqtt server");

  return true;
}

void AirgradientCellularClient::_serialize(std::ostringstream &oss, int rco2, int particleCount003,
                                           float pm01, float pm25, float pm10, int tvoc, int nox,
                                           float atmp, float rhum, int signal, float vBat,
                                           float vPanel, float o3WorkingElectrode,
                                           float o3AuxiliaryElectrode, float no2WorkingElectrode,
                                           float no2AuxiliaryElectrode, float afeTemp) {
  // CO2
  if (IS_CO2_VALID(rco2)) {
    oss << std::round(rco2);
  }
  oss << ",";
  // Temperature
  if (IS_TEMPERATURE_VALID(atmp)) {
    oss << std::round(atmp * 10);
  }
  oss << ",";
  // Humidity
  if (IS_HUMIDITY_VALID(rhum)) {
    oss << std::round(rhum * 10);
  }
  oss << ",";
  // PM1.0 atmospheric environment
  if (IS_PM_VALID(pm01)) {
    oss << std::round(pm01 * 10);
  }
  oss << ",";
  // PM2.5 atmospheric environment
  if (IS_PM_VALID(pm25)) {
    oss << std::round(pm25 * 10);
  }
  oss << ",";
  // PM10 atmospheric environment
  if (IS_PM_VALID(pm10)) {
    oss << std::round(pm10 * 10);
  }
  oss << ",";
  // TVOC
  if (IS_TVOC_VALID(tvoc)) {
    oss << tvoc;
  }
  oss << ",";
  // NOx
  if (IS_NOX_VALID(nox)) {
    oss << nox;
  }
  oss << ",";
  // PM 0.3 particle count
  if (IS_PM_VALID(particleCount003)) {
    oss << particleCount003;
  }
  oss << ",";
  // Radio signal
  oss << signal;

#ifndef ARDUINO
  // TODO: Improvement so it is based on product model
  oss << ",";
  // V Battery
  if (IS_VOLT_VALID(vBat)) {
    oss << std::round(vBat * 100);
  }
  oss << ",";
  // V Solar Panel
  if (IS_VOLT_VALID(vPanel)) {
    oss << std::round(vPanel * 100);
  }
  oss << ",";
  // Working Electrode O3
  if (IS_VOLT_VALID(o3WorkingElectrode)) {
    oss << std::round(o3WorkingElectrode * 1000);
  }
  oss << ",";
  // Auxiliary Electrode O3
  if (IS_VOLT_VALID(o3AuxiliaryElectrode)) {
    oss << std::round(o3AuxiliaryElectrode * 1000);
  }
  oss << ",";
  // Working Electrode NO2
  if (IS_VOLT_VALID(no2WorkingElectrode)) {
    oss << std::round(no2WorkingElectrode * 1000);
  }
  oss << ",";
  // Auxiliary Electrode NO2
  if (IS_VOLT_VALID(no2AuxiliaryElectrode)) {
    oss << std::round(no2AuxiliaryElectrode * 1000);
  }
  oss << ",";
  // AFE Temperature
  if (IS_VOLT_VALID(afeTemp)) {
    oss << std::round(afeTemp * 10);
  }
#endif
}

#endif // ESP8266
