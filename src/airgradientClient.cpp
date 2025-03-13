/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientClient.h"

bool AirgradientClient::begin() { return true; }

std::string AirgradientClient::httpFetchConfig(const std::string &sn) { return std::string(); }

bool AirgradientClient::httpPostMeasures(const std::string &sn, const std::string &payload) {
  return false;
}

void AirgradientClient::resetFetchConfigurationStatus() { lastFetchConfigSucceed = true; }

void AirgradientClient::resetPostMeasuresStatus() { lastPostMeasuresSucceed = true; }

bool AirgradientClient::isLastFetchConfigSucceed() { return lastFetchConfigSucceed; }

bool AirgradientClient::isLastPostMeasureSucceed() { return lastPostMeasuresSucceed; }

bool AirgradientClient::isRegisteredOnAgServer() { return registeredOnAgServer; }

std::string AirgradientClient::buildFetchConfigUrl(const std::string &sn, bool useHttps) {
  // http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/one/config
  char url[80] = {0};
  if (useHttps) {
    sprintf(url, "https://%s/sensors/airgradient:%s/one/config", domain, sn.c_str());
  } else {
    sprintf(url, "http://%s/sensors/airgradient:%s/one/config", domain, sn.c_str());
  }

  return std::string(url);
}

std::string AirgradientClient::buildPostMeasuresUrl(const std::string &sn, bool useHttps) {
  // http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/measures
  char url[80] = {0};
  if (useHttps) {
    sprintf(url, "https://%s/sensors/airgradient:%s/measures", domain, sn.c_str());
  } else {
    sprintf(url, "http://%s/sensors/airgradient:%s/measures", domain, sn.c_str());
  }

  return std::string(url);
}
