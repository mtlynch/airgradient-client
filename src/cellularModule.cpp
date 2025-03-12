/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "cellularModule.h"

CellularModule::CellularModule() {}

CellularModule::~CellularModule() {}

bool CellularModule::init() { return false; }

void CellularModule::powerOn() {}

void CellularModule::powerOff() {}

bool CellularModule::reset() { return true; }

void CellularModule::sleep() {}

CellResult<std::string> CellularModule::getModuleInfo() { return CellResult<std::string>(); }

CellResult<std::string> CellularModule::retrieveSimCCID() { return CellResult<std::string>(); }

CellReturnStatus CellularModule::isSimReady() { return CellReturnStatus(); }

CellResult<int> CellularModule::retrieveSignal() { return CellResult<int>(); }

CellResult<std::string> CellularModule::retrieveIPAddr() { return CellResult<std::string>(); }

CellResult<std::string> CellularModule::startNetworkRegistration(CellTechnology ct,
                                                                 const std::string &apn,
                                                                 uint32_t operationTimeoutMs) {
  return CellResult<std::string>();
}

CellResult<CellularModule::HttpResponse>
CellularModule::httpGet(const std::string &url, int connectionTimeout, int responseTimeout) {
  return CellResult<HttpResponse>();
}

CellResult<CellularModule::HttpResponse> CellularModule::httpPost(const std::string &url,
                                                                  const std::string &body,
                                                                  int connectionTimeout,
                                                                  int responseTimeout) {
  return CellResult<HttpResponse>();
}
