#include <Arduino.h>
#include "voltaje.h"

#define PIN_VOLTAJE A0
double VREF = 1.054; // valor de referencia interno del Atmega 328p, 1.1V teóricos
double R1 = 330000;  // 1M
double R2 = 6800;    // 100K

float voltajeMaximo()
{
  return VREF / (R2 / (R1 + R2));
}

float leerVoltaje()
{
  int valor = analogRead(PIN_VOLTAJE);
  Serial.println(valor);
  double v = (valor * VREF) / 1024.0;
  return (v / (R2 / (R1 + R2)));
}