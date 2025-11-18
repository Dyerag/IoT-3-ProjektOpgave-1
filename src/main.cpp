#include <Arduino.h>

int button = 2;
int led = 25;

// put function declarations here:

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  pinMode(button, INPUT);
  pinMode(led, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (digitalRead(button) == HIGH)
    // digitalWrite(led, HIGH);
    Serial.println("Button pressed");
  // else
    // digitalWrite(led, LOW);
}

// put function definitions here:
