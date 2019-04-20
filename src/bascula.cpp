
#include "HX711.h"
#include "bascula.h"

HX711 scale;
#define LOADCELL_DOUT_PIN 5
#define LOADCELL_SCK_PIN 4

void setupBascula()
{
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  //long zero_factor = scale.read_average(); //Get a baseline reading
}

void calibrarBasculaSetup()
{
  pinMode(13, OUTPUT);
  while (true)
  {
    delay(2000);
    Serial.println("Place object");
    digitalWrite(13, HIGH);
    scale.set_scale();
    scale.tare();
    delay(4000);
    digitalWrite(13, LOW);
    Serial.println(scale.get_units(10));
  }
}

void taraBascula()
{
  scale.tare(); //Reset the scale to 0
}

void calibracionBascula(float factorCalibracionBascula)
{
  scale.set_scale(factorCalibracionBascula); //Adjust to this calibration factor
}

float pesoBascula()
{
  return scale.get_units();
}