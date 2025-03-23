#include <Arduino.h>

#define USE_CELLULAR

#include "airgradientClient.h"

#ifdef USE_CELLULAR
#include "airgradientCellularClient.h"
#include "cellularModuleA7672xx.h"
#include "agSerial.h"
AgSerial agSerial(Wire);
CellularModule *cell;
#else
#include "WiFi.h"
#include "airgradientWifiClient.h"
#endif

AirgradientClient *agClient;

static const std::string serialNumber = "aabbccddeeff";

#define GPIO_IIC_RESET 3
#define EXPANSION_CARD_POWER 4
#define GPIO_POWER_CELLULAR 5

void cellularCardOn() {
  pinMode(EXPANSION_CARD_POWER, OUTPUT);
  digitalWrite(EXPANSION_CARD_POWER, HIGH);
}

void fetchConfiguration() {
  Serial.println("Try to fetch monitor configuration");
  std::string config = agClient->httpFetchConfig(serialNumber);
  if (agClient->isLastFetchConfigSucceed()) {
    Serial.println("--CONFIG--");
    Serial.println(config.c_str());
  } else {
    Serial.println("--CONFIG FAILED--");
  }
}

void postMeasures() {
  std::string payload = "{}";
  // Try post measures
  Serial.println("Try to post measures");
  bool post = agClient->httpPostMeasures(serialNumber, payload);
  if (agClient->isLastPostMeasureSucceed() && post) { // Both param should have same value
    Serial.println("POST measures success");
  } else {
    Serial.println("POST measures failed");
  }
}

void setup() {
  Serial.begin(115200);
  cellularCardOn();
  delay(3000);

#ifdef USE_CELLULAR
  Wire.begin(7, 6);
  Wire.setClock(1000000);
  agSerial.init(GPIO_IIC_RESET);
  if (!agSerial.open()) {
    Serial.println("AgSerial failed, or cellular module just not found. restarting in 10s");
    delay(10000);
    esp_restart();
  }

  cell = new CellularModuleA7672XX(&agSerial, GPIO_POWER_CELLULAR);
  agClient = new AirgradientCellularClient(cell);

#else // WIFI
  agClient = new AirgradientWifiClient();

  // Connect wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "password");
  Serial.println("\nConnecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#endif

  if (!agClient->begin("serialnumber")) {
    Serial.println("Start Airgradient Client FAILED, restarting in 10s");
    delay(10000);
    esp_restart();
  }
}

void loop() {
  postMeasures();
  delay(60000);
  fetchConfiguration();
  delay(60000);
}
