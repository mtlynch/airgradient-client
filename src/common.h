/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#ifndef AG_COMMON_H
#define AG_COMMON_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define MILLIS() ((uint32_t)(esp_timer_get_time() / 1000))

class Common {
public:
  static void splitByDelimiter(const std::string &data, int *v1, int *v2, char delimiter = ',') {
    size_t pos = data.find(delimiter);
    if (pos != std::string::npos) {
      *v1 = std::stoi(data.substr(0, pos));
      *v2 = std::stoi(data.substr(pos + 1, data.length()));
    }
  }
  static void splitByDelimiter(const std::string &data, std::string &v1, std::string &v2,
                               char delimiter = ',') {
    size_t pos = data.find(delimiter);
    if (pos != std::string::npos) {
      v1 = data.substr(0, pos);
      v2 = data.substr(pos + 1, data.length());
    }
  }

  static void parseUri(const std::string &uri, std::string &protocol, std::string &username,
                       std::string &password, std::string &host, int &port) {
    protocol.clear();
    username.clear();
    password.clear();
    host.clear();
    port = -1;

    size_t pos = uri.find("://");
    if (pos != std::string::npos) {
      protocol = uri.substr(0, pos);
      pos += 3; // skip "://"
    } else {
      pos = 0;
    }

    // Check for userinfo before @
    size_t atPos = uri.find('@', pos);
    if (atPos != std::string::npos) {
      std::string userinfo = uri.substr(pos, atPos - pos);
      pos = atPos + 1;

      size_t colonPos = userinfo.find(':');
      if (colonPos != std::string::npos) {
        username = userinfo.substr(0, colonPos);
        password = userinfo.substr(colonPos + 1);
      } else {
        username = userinfo;
      }
    }

    // Host and optional port
    size_t colonPos = uri.find(':', pos);
    if (colonPos != std::string::npos) {
      host = uri.substr(pos, colonPos - pos);
      port = std::stoi(uri.substr(colonPos + 1));
    } else {
      host = uri.substr(pos);
    }
  }
};

#endif // AG_COMMON_H
#endif // ESP8266
