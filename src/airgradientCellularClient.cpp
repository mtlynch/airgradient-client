/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientCellularClient.h"
#include "cellularModule.h"

AirgradientCellularClient::AirgradientCellularClient(CellularModule *cellularModule)
    : cell_(cellularModule) {}

bool AirgradientCellularClient::begin(std::string sn) {
  // Update parent serialNumber variable
  serialNumber = sn;
  clientReady = false;

  if (!cell_->init()) {
    ESP_LOGE(TAG, "Cannot initialized cellular client");
    return false;
  }

  // To make sure module ready to use
  if (cell_->isSimReady() != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "SIM is not ready, please check if SIM is inserted properly!");
    return false;
  }

  // Printout SIM CCID
  auto result = cell_->retrieveSimCCID();
  if (result.status != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Failed get SIM CCID, please check if SIM is inserted properly!");
    return false;
  }

  ESP_LOGI(TAG, "SIM CCID: %s", result.data.c_str());

  // Register network
  result = cell_->startNetworkRegistration(CellTechnology::LTE, _apn, 30000);
  if (result.status != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Cellular client failed, module cannot register to network");
    return false;
  }

  ESP_LOGI(TAG, "Cellular client ready, module registered to network");
  clientReady = true;

  return true;
}

bool AirgradientCellularClient::ensureClientConnection() {
  if (cell_->isNetworkRegistered(CellTechnology::LTE) == CellReturnStatus::Ok) {
    ESP_LOGI(TAG, "Client connection is OK");
    clientReady = true;
    return true;
  }

  ESP_LOGE(TAG, "Network not registered! Power cycle module and restart network registration");
  if (cell_->reinitialize() != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Failed reinitialized cellular module");
    clientReady = false;
    return false;
  }

  // To make sure module ready to use
  if (cell_->isSimReady() != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "SIM is not ready, please check if SIM is inserted properly!");
    clientReady = false;
    return false;
  }

  // Register network
  auto result = cell_->startNetworkRegistration(CellTechnology::LTE, _apn, 30000);
  if (result.status != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Cellular client failed, module cannot register to network");
    clientReady = false;
    return false;
  }

  clientReady = true;
  ESP_LOGI(TAG, "Cellular client ready, module registered to network");

  return true;
}

std::string AirgradientCellularClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl();

  ESP_LOGI(TAG, "Fetch configuration from server");
  auto result = cell_->httpGet(url); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    // TODO: This can be timeout from module or error, how to handle this?
    ESP_LOGE(TAG, "Module not return OK when call httpGet()");
    lastFetchConfigSucceed = false;
    return {};
  }

  // Response status check if fetch failed
  if (result.data.statusCode != 200) {
    ESP_LOGW(TAG, "Failed fetch configuration from server with return code %d",
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
    ESP_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    return {};
  }

  ESP_LOGD(TAG, "Received configuration: (%d) %s", result.data.bodyLen, result.data.body.get());

  // Move the string from unique_ptr
  std::string body = std::string(result.data.body.get());

  ESP_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return body;
}

bool AirgradientCellularClient::httpPostMeasures(const std::string &payload) {
  // std::string url = buildPostMeasuresUrl();
  char url[80] = {0};
  sprintf(url, "http://%s/sensors/%s/c-c", httpDomain, serialNumber.c_str());

  ESP_LOGI(TAG, "Post measures to server");
  auto result = cell_->httpPost(url, payload); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    // TODO: This can be timeout from module or error, how to handle this?
    ESP_LOGE(TAG, "Module not return OK when call httpPost()");
    lastPostMeasuresSucceed = false;
    return false;
  }

  // Response status check if post failed
  if ((result.data.statusCode != 200) && (result.data.statusCode != 429) &&
      (result.data.statusCode != 201)) {
    ESP_LOGW(TAG, "Failed post measures to server with response code %d", result.data.statusCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  ESP_LOGI(TAG, "Success post measures to server with response code %d", result.data.statusCode);

  return true;
}

bool AirgradientCellularClient::mqttConnect() {
  auto result = cell_->mqttConnect(serialNumber, mqttDomain, mqttPort);
  if (result != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Failed connect to airgradient mqtt server");
    return false;
  }
  ESP_LOGI(TAG, "Success connect to airgardient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttDisconnect() {
  if (cell_->mqttDisconnect() != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Failed disconnect from airgradient mqtt server");
    return false;
  }
  ESP_LOGI(TAG, "Success disconnect from airgradient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttPublishMeasures(const std::string &payload) {
  // TODO: Ensure mqtt connection
  auto topic = buildMqttTopicPublishMeasures();
  auto result = cell_->mqttPublish(topic, payload);
  if (result != CellReturnStatus::Ok) {
    ESP_LOGE(TAG, "Failed publish measures to mqtt server");
    return false;
  }
  ESP_LOGI(TAG, "Success publish measures to mqtt server");

  return true;
}
