/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#include "airgradientWifiClient.h"
#include "agLogger.h"

#ifdef ARDUINO
#include <HTTPClient.h>
#else
#include "esp_http_client.h"
#endif

bool AirgradientWifiClient::begin(std::string sn) {
  serialNumber = sn;
  return true;
}

std::string AirgradientWifiClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl(true);
  AG_LOGI(TAG, "Fetch configuration from %s", url.c_str());

  // Perform HTTP GET
  int responseCode;
  std::string responseBody;
  if (_httpGet(url, responseCode, responseBody) == false) {
    lastFetchConfigSucceed = false;
    return {};
  }

  // Define result by response code
  if (responseCode != 200) {
    // client.end();
    AG_LOGE(TAG, "Failed fetch configuration from server with return code %d", responseCode);
    // Return code 400 means device not registered on ag server
    if (responseCode == 400) {
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;
    return {};
  }

  // Sanity check if response body is empty
  if (responseBody.empty()) {
    AG_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    lastFetchConfigSucceed = false;
    return responseBody;
  }

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;
  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return responseBody;
}
bool AirgradientWifiClient::httpPostMeasures(const std::string &payload) {
  std::string url = buildPostMeasuresUrl(true);
  AG_LOGI(TAG, "Post measures to %s", url.c_str());

  // Perform HTTP POST
  int responseCode;
  if (_httpPost(url, payload, responseCode) == false) {
    lastPostMeasuresSucceed = false;
    return false;
  }

  if ((responseCode != 200) && (responseCode != 429)) {
    AG_LOGE(TAG, "Failed post measures to server with response code %d", responseCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  AG_LOGI(TAG, "Success post measures to server with response code %d", responseCode);

  return true;
}

bool AirgradientWifiClient::_httpGet(const std::string &url, int &responseCode,
                                     std::string &responseBody) {
#ifdef ARDUINO
  // Init http client
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    return false;
  }

  responseCode = client.GET();
  responseBody = client.getString().c_str();
  client.end();
  return true;
#else
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_GET;
  config.cert_pem = AG_SERVER_ROOT_CA;

  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (esp_http_client_open(client, 0) != ESP_OK) {
    AG_LOGE(TAG, "Failed perform HTTP GET");
    esp_http_client_cleanup(client);
    return false;
  }
  esp_http_client_fetch_headers(client);
  responseCode = esp_http_client_get_status_code(client);

  int totalRead = 0;
  int readLen;
  while ((readLen = esp_http_client_read(client, responseBuffer + totalRead,
                                         MAX_RESPONSE_BUFFER - totalRead)) > 0) {
    totalRead += readLen;
  }


  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  responseBuffer[totalRead] = '\0';
  responseBody = std::string(responseBuffer);

  return true;
#endif
}

bool AirgradientWifiClient::_httpPost(const std::string &url, const std::string &payload,
                                      int &responseCode) {
#ifdef ARDUINO
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    return false;
  }

  client.addHeader("content-type", "application/json");
  responseCode = client.POST(String(payload.c_str()));
  client.end();
  return true;
#else
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_POST;
  config.cert_pem = AG_SERVER_ROOT_CA;
  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, payload.c_str(), payload.length());

  if (esp_http_client_perform(client) != ESP_OK) {
    AG_LOGE(TAG, "Failed perform HTTP POST");
    esp_http_client_cleanup(client);
    return false;
  }
  responseCode = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  return true;
#endif
}

#endif // ESP8266
