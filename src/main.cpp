#include <Arduino.h>

int happyButton = 4;
int happyLed = 26;
int satisfiedButton = 33;
int satisfiedLed = 32;
int unsatisfiedButton = 34;
int unsatisfiedLed = 14;
int angryButton = 12;
int angryLed = 25;

// put function declarations here:

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  pinMode(happyButton, INPUT);
  pinMode(happyLed, OUTPUT);
  pinMode(satisfiedButton, INPUT);
  pinMode(satisfiedLed, OUTPUT);
  pinMode(unsatisfiedButton, INPUT);
  pinMode(unsatisfiedLed, OUTPUT);
  pinMode(angryButton, INPUT);
  pinMode(angryLed, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (digitalRead(happyButton) == HIGH)
  {
    Serial.println("happyButton pressed");
    digitalWrite(happyLed, HIGH);
  }
  else
    digitalWrite(happyLed, LOW);

  if (digitalRead(satisfiedButton) == HIGH)
  {
    Serial.println("satisfiedButton pressed");
    digitalWrite(satisfiedLed, HIGH);
  }
  else
    digitalWrite(satisfiedLed, LOW);

  if (digitalRead(unsatisfiedButton) == HIGH)
  {
    Serial.println("unsatisfiedButton pressed");
    digitalWrite(unsatisfiedLed, HIGH);
  }
  else
    digitalWrite(unsatisfiedLed, LOW);

  if (digitalRead(angryButton) == HIGH)
  {
    Serial.println("angryButton pressed");
    digitalWrite(angryLed, HIGH);
  }
  else
    digitalWrite(angryLed, LOW);
}

// put function definitions here:
