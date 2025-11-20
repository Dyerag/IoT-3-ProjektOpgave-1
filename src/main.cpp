#include <Arduino.h>
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <time.h>

// Wakeup buttons
#define WAKEUP_GPIO_1 GPIO_NUM_4                                                                                                 // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_2 GPIO_NUM_33                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_3 GPIO_NUM_35                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_4 GPIO_NUM_34                                                                                                // Only RTC IO are allowed - ESP32 Pin example
#define BUTTON_PIN_BITMASK (1ULL << WAKEUP_GPIO_1) | (1ULL << WAKEUP_GPIO_2) | (1ULL << WAKEUP_GPIO_3) | (1ULL << WAKEUP_GPIO_4) // 2 ^ GPIO_NUMBER in hex
#define USE_EXT0_WAKEUP 0                                                                                                        // 1 = EXT0 wakeup, 0 = EXT1 wakeup

// Button and LED pins
const int happyButton = 4;
const int happyLed = 26;
const int satisfiedButton = 33;
const int satisfiedLed = 32;
const int unsatisfiedButton = 35;
const int unsatisfiedLed = 14;
const int angryButton = 34;
const int angryLed = 25;

// Deepsleep time
unsigned long lastAction = 0;
unsigned long sleepTime = 30000; // 30 seconds

//  WIFI
const char *WIFI_SSID = "TEC-IOT";
const char *WIFI_PASS = "42090793";

// Knap/LED-arrays
int buttons[] = {happyButton, satisfiedButton, unsatisfiedButton, angryButton};
int leds[] = {happyLed, satisfiedLed, unsatisfiedLed, angryLed};
const int count = 4;

// Navn til print
const char *buttonNames[] = {
    "HAPPY",
    "SATISFIED",
    "UNSATISFIED",
    "ANGRY"};

// Debounce
const unsigned long DEBOUNCE_MS = 30;
bool buttonStableState[4];
bool buttonLastReading[4];
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};

// LED-timer (4 sek.)
unsigned long ledTimer[4] = {0, 0, 0, 0};
const unsigned long LED_ON_TIME = 1000; // sekunder

// Hukommelse til sidste knaptryk 
int    lastPressedIndex      = -1; 
String lastPressedName       = "";
String lastPressedTimestamp  = "";

// put function declarations here:
void ConnectWiFi();
bool InitTime();
void PrintCurrentTimeForButton(int index);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // wifi og tid
  ConnectWiFi();
  InitTime();

  pinMode(happyButton, INPUT);
  pinMode(happyLed, OUTPUT);
  pinMode(satisfiedButton, INPUT);
  pinMode(satisfiedLed, OUTPUT);
  pinMode(unsatisfiedButton, INPUT);
  pinMode(unsatisfiedLed, OUTPUT);
  pinMode(angryButton, INPUT);
  pinMode(angryLed, OUTPUT);

  // led slukket
  for (int i = 0; i < count; i++)
  {
    digitalWrite(leds[i], LOW);
  }

  // Init debounce-tilstand til nuværende knapniveau
  for (int i = 0; i < count; i++)
  {
    bool reading = digitalRead(buttons[i]);
    buttonStableState[i] = reading;
    buttonLastReading[i] = reading;
  }

  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

  rtc_gpio_pulldown_en(WAKEUP_GPIO_1);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_2);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_3);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_4);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_1);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_2);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_3);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_4);
}

void loop()
{
  unsigned long now = millis();

  for (int i = 0; i < count; i++)
  {
    bool reading = digitalRead(buttons[i]);

    // 1) Rå læsning ændret -> start debounce-timer
    if (reading != buttonLastReading[i])
    {
      lastDebounceTime[i] = now;
      buttonLastReading[i] = reading;
    }

    // 2) Hvis uændret i DEBOUNCE_MS -> opdatér stabil tilstand
    if ((now - lastDebounceTime[i]) >= DEBOUNCE_MS)
    {
      // er der en reel ændring i stabil tilstand
      if (reading != buttonStableState[i])
      {
        // stabil tilstand skifter
        buttonStableState[i] = reading;

        // 3) Registrér et "tryk" ved LOW -> HIGH
        if (buttonStableState[i] == HIGH)
        {
          // Tænd LED og start timer
          digitalWrite(leds[i], HIGH);
          ledTimer[i] = now;

          // Hent tid og gem knaptryk
          struct tm timeinfo;
          if (getLocalTime(&timeinfo))
          {
            RememberButtonPress(i, timeinfo);
          }
          else
          {
            Serial.println("Kunne ikke hente lokal tid");
          }

          // reset inactivity timer on button press
          lastAction = now;
        }
      }
    }

    // 4) Sluk LED efter LED_ON_TIME ms
    if (digitalRead(leds[i]) == HIGH && (now - ledTimer[i] >= LED_ON_TIME))
    {
      digitalWrite(leds[i], LOW);
    }

    // Start deep sleep if inactivity exceeds sleepTime
    if (now - lastAction >= sleepTime)
    {
      Serial.println("Entering sleep");
      esp_deep_sleep_start();
    }
  }
}

// put function definitions here:
bool InitTime()
{
  if (!WiFi.isConnected())
  {
    Serial.println("Ingen WiFi → kan ikke hente NTP-tid");
    return false;
  }

  // 3600 = GMT+1 (DK vintertid), 0 = ingen ekstra sommertid-justering
  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  unsigned long start = millis();

  while (millis() - start < 10000) 
  {
    if (getLocalTime(&timeinfo))
    {
      Serial.println("Tid hentet fra NTP");
      return true;
    }

    Serial.println("Venter på NTP...");
    vTaskDelay(100);   
  }

  Serial.println("Kunne ikke hente tid fra NTP");
  return false;
}

// FORBIND TIL WIFI
void ConnectWiFi()
{
  if (WiFi.isConnected())
    return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Forbinder til WiFi");

  unsigned long start = millis();

  while (!WiFi.isConnected() && (millis() - start < 20000))  // 20 sek timeout
  {
    Serial.print(".");
    vTaskDelay(1);   
  }

  Serial.println();

  if (WiFi.isConnected())
    Serial.println("WiFi: FORBUNDET");
  else
    Serial.println("WiFi: FORBUNDELSE FEJLEDE");
}

// GEM HUSK OM KNAPTRYK
void RememberButtonPress(int index, const struct tm &timeinfo)
{
  lastPressedIndex = index;
  lastPressedName  = buttonNames[index];

  char buf[32];
  snprintf(
      buf,
      sizeof(buf),
      "%04d-%02d-%02d %02d:%02d:%02d",
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec);

  lastPressedTimestamp = String(buf);

  Serial.print("gemt: ");
  Serial.print(lastPressedName);
  Serial.print("  tid: ");
  Serial.println(lastPressedTimestamp);
}