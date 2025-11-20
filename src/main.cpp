#include <Arduino.h>
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <time.h>
#include <PubSubClient.h>

// Wakeup buttons
#define WAKEUP_GPIO_1 GPIO_NUM_4  // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_2 GPIO_NUM_33 // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_3 GPIO_NUM_35 // Only RTC IO are allowed - ESP32 Pin example
#define WAKEUP_GPIO_4 GPIO_NUM_34 // Only RTC IO are allowed - ESP32 Pin example
// Adds all the buttons to the bitmask, so they can be used to wake up the ESP32
#define BUTTON_PIN_BITMASK (1ULL << WAKEUP_GPIO_1) | (1ULL << WAKEUP_GPIO_2) | (1ULL << WAKEUP_GPIO_3) | (1ULL << WAKEUP_GPIO_4) // 2 ^ GPIO_NUMBER in hex
// Sets the wakeup method to 0, which means use EXT1 for wakeup. 1 supports one or more pins
#define USE_EXT0_WAKEUP 0 // 1 = EXT0 wakeup, 0 = EXT1 wakeup

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
// unsigned long sleepTime = 30000; // 30 seconds
unsigned long sleepTime = 100;

//  WIFI
const char *WIFI_SSID = "TEC-IOT";
const char *WIFI_PASS = "42090793";

// mqtt
const char *MQTT_SERVER = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_TOPIC = "tec/iot/buttons";

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
int lastPressedIndex = -1;
String lastPressedName = "";
String lastPressedTimestamp = "";

// put function declarations here:
void ConnectWiFi();
bool InitTime();
void PrintCurrentTimeForButton(int index);
void WakeUpButtonTrigger();
void RememberButtonPress(int index, const struct tm &timeinfo);
void HandleButtons();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // wifi og tid
  ConnectWiFi();
  InitTime();

  // pinMode setup for the buttons and corresponding leds
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

  if (esp_sleep_get_wakeup_cause())
    WakeUpButtonTrigger();
}

void loop()
{
  HandleButtons();
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

  while (!WiFi.isConnected() && (millis() - start < 20000)) // 20 sek timeout
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

// HENT TID VIA NTP
// Print nuværende tid når en bestemt knap (index) er trykket
void PrintCurrentTimeForButton(int index)
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    Serial.printf(
        "trykket: %s  %04d-%02d-%02d %02d:%02d:%02d\n",
        buttonNames[index],
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec);
  }
  else
  {
    Serial.println("Kunne ikke hente lokal tid");
  }
}

void WakeUpButtonTrigger()
{
  int wakeup_reason = log(esp_sleep_get_ext1_wakeup_status()) / log(2);
  Serial.println("Woke up from sleep by " + String(wakeup_reason));

  switch (wakeup_reason)
  {
  case GPIO_NUM_4:
  case GPIO_NUM_33:
  case GPIO_NUM_35:
  case GPIO_NUM_34:
    HandleButtons(wakeup_reason);
    break;
  default:
    Serial.println("Unable to identify wake up cause");
    break;
  }
}

// GEM HUSK OM KNAPTRYK
void RememberButtonPress(int index, const struct tm &timeinfo)
{
  lastPressedIndex = index;
  lastPressedName = buttonNames[index];

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

void HandleButtons()
{
  unsigned long now = millis();

  for (int i = 0; i < count; i++)
  {
    bool reading = digitalRead(buttons[i]);

    // 1: Rå læsning ændret → start debounce-timer
    if (reading != buttonLastReading[i])
    {
      lastDebounceTime[i] = now;
      buttonLastReading[i] = reading;
    }

    // 2: Stabil i DEBOUNCE_MS → opdater stabil tilstand
    if ((now - lastDebounceTime[i]) >= DEBOUNCE_MS)
    {
      if (reading != buttonStableState[i])
      {
        buttonStableState[i] = reading;

        // 3: Tryk registreret (LOW → HIGH)
        if (buttonStableState[i] == HIGH)
        {
          digitalWrite(leds[i], HIGH);
          ledTimer[i] = now;

          struct tm timeinfo;
          if (getLocalTime(&timeinfo))
          {
            RememberButtonPress(i, timeinfo);
          }
          else
          {
            Serial.println("Kunne ikke hente lokal tid");
          }

          lastAction = now; // reset inactivity timer
        }
      }
    }

    // 4: Sluk LED efter timeout
    if (digitalRead(leds[i]) == HIGH && (now - ledTimer[i] >= LED_ON_TIME))
    {
      digitalWrite(leds[i], LOW);
    }
  }

  // Deepsleep hvis inaktiv
  if (now - lastAction >= sleepTime)
  {
    Serial.println("Entering sleep");
    esp_deep_sleep_start();
  }
}