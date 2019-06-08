#ifdef PLACA_COCHE
#pragma "# Compilando para la placa de Coche #"

#include <Arduino.h>
#include <string.h>

#define RECEPTOR_CH3_PIN A1
#define RECEPTOR_CH6_PIN A2
#define DIR_PIN 4
#define STEP_PIN 5

#define minimo_PWM 995
#define maximo_PWM 1989

unsigned long timeout_pwm_micros = 10000;

void setup()
{
  analogReference(INTERNAL);
  Serial.begin(9600);

  pinMode(RECEPTOR_CH3_PIN, INPUT);
  pinMode(RECEPTOR_CH6_PIN, INPUT);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
}

int throttle()
{
  unsigned long value1 = pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  //  unsigned long value2 = pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  //  int media = (value1 + value2) / 2;
  int media = value1;
  int valor = map(media, minimo_PWM, maximo_PWM, 0, 100);
  return max(min(valor, 100), 0);
}

bool switchActivado()
{
  unsigned long value1 = pulseIn(RECEPTOR_CH6_PIN, HIGH, timeout_pwm_micros);
  return !((value1 > 1600));
}

int pasos_por_iteracion = 4000;

void loop()
{
  static int vueltas = 0;
  if (vueltas++ % 20)
  {
    digitalWrite(DIR_PIN, switchActivado());
    vueltas = 1;
  }
  int velocidad = throttle();
  if (velocidad > 5)
  {
    unsigned long retraso = map(velocidad, 0, 100, 500, 10);

    /*
    Serial.print("velocidad ");
    Serial.print(velocidad);
    Serial.print(" retraso ");
    Serial.println(retraso);
*/

    for (int i = 0; i < pasos_por_iteracion; i++)
    {
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(retraso);
      digitalWrite(STEP_PIN, LOW);
      if (i < (pasos_por_iteracion - 1))
      {
        delayMicroseconds(retraso);
      }
    }
  }
}
#endif