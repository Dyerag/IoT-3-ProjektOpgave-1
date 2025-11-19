#include <Arduino.h>
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <time.h>

#define WAKEUP_GPIO_1 GPIO_NUM_4                                                                                                 // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_2 GPIO_NUM_33                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_3 GPIO_NUM_34                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_4 GPIO_NUM_12                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define BUTTON_PIN_BITMASK (1ULL << WAKEUP_GPIO_1) | (1ULL << WAKEUP_GPIO_2) | (1ULL << WAKEUP_GPIO_3) | (1ULL << WAKEUP_GPIO_4) // 2 ^ GPIO_NUMBER in hex
#define USE_EXT0_WAKEUP 0                                                                                                        // 1 = EXT0 wakeup, 0 = EXT1 wakeup

const int happyButton = 4;
const int happyLed = 26;
const int satisfiedButton = 33;
const int satisfiedLed = 32;
const int unsatisfiedButton = 34;
const int unsatisfiedLed = 14;
const int angryButton = 12;
const int angryLed = 25;

//  WIFI 
const char* WIFI_SSID = "IoT_H3/4";
const char* WIFI_PASS = "98806829";

// put function declarations here:

// FORBIND TIL WIFI
void connectWiFi()
{
  if (WiFi.isConnected()) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Forbinder til WiFi");

  int tries = 0;
  while (!WiFi.isConnected() && tries < 40) {  // ca. 20 sek
    Serial.print(".");
    delay(500);
    tries++;
  }
  Serial.println();

  if (WiFi.isConnected()) Serial.println("WiFi: FORBUNDET");
  else Serial.println("WiFi: FORBUNDELSE FEJLEDE");
}

// HENT TID VIA NTP 
bool initTime()
{
  connectWiFi();

  if (!WiFi.isConnected()) {
    Serial.println("Ingen WiFi → kan ikke hente NTP-tid");
    return false;
  }

  // 3600 = GMT+1 (DK vintertid), 0 = ingen ekstra sommertid-justering
  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  for (int i = 0; i < 20; i++) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("Tid hentet fra NTP");
      return true;
    }
    Serial.println("Venter på NTP...");
    delay(500);
  }

  Serial.println("Kunne ikke hente tid fra NTP");
  return false;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // wifi og tid
  connectWiFi();
  initTime();

  pinMode(happyButton, INPUT);
  pinMode(happyLed, OUTPUT);
  pinMode(satisfiedButton, INPUT);
  pinMode(satisfiedLed, OUTPUT);
  pinMode(unsatisfiedButton, INPUT);
  pinMode(unsatisfiedLed, OUTPUT);
  pinMode(angryButton, INPUT);
  pinMode(angryLed, OUTPUT);

  digitalWrite(happyLed, HIGH);
  digitalWrite(satisfiedLed, HIGH);
  digitalWrite(unsatisfiedLed, HIGH);
  digitalWrite(angryLed, HIGH);
  delay(2000);
  
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

  rtc_gpio_pulldown_en(WAKEUP_GPIO_1);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_2);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_3);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_4);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_1);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_2);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_3);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_4);

  Serial.println("Entering sleep");
  esp_deep_sleep_start();
}

void loop()
{
  // put your main code here, to run repeatedly:
}

// put function definitions here:
