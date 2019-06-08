#ifdef PLACA_VOLTAGE
#pragma "# Compilando para la placa de Voltaje #"

#include "voltaje.h"
#include "pantalla.h"

#include <Arduino.h>
#include <string.h>
#include <Servo.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define RECEPTOR_CH3_PIN A1
#define RECEPTOR_CH5_PIN A2
#define TEST_PIN 2
#define SALIDA_ESC_PIN 10

#define minimo_PWM 995
#define maximo_PWM 1989

SoftwareSerial softSer(7, 8);
char serialBuffer[6];

const unsigned long TIEMPO_PASO_TEST = 1000;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

volatile int test = 0;
float iteracion_paso_test = 0;

Servo ESC;
int velocidadReal = 0;
int vAhora_real = 0;

const char *texto_version = "Power Module v4A";

float voltage_final = 0;
volatile int velocidadPWM = 0;
int vAhora = 0;
int velmin = 0;
int incremento = 20;
int voltage_max = 12.0;

int iteraciones_escritura = 10;
int escribe = 0;

const int lecturas = 5;   // Total lecturas V
int index = 0;            // El indice de la lectura actual
float readings[lecturas]; // Lecturas de la entrada analogica
float Vtotal = 0.0;       // Total
float Vmedia = 0.0;       // Promedio

unsigned long timeout_pwm_micros = 50000;

volatile unsigned long siguientePaso = 0; // millis en los que tiene que saltar al siguiente paso

void test_funcion();

void setup()
{
  analogReference(INTERNAL);
  Serial.begin(9600);
  softSer.begin(9600);

  u8g2.begin();
  pantallaBienvenida(u8g2, texto_version);

  pinMode(TEST_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TEST_PIN), test_funcion, FALLING);

  pinMode(RECEPTOR_CH3_PIN, INPUT);
  pinMode(RECEPTOR_CH5_PIN, INPUT);

  Serial.print("Voltaje Máximo: ");
  Serial.print(voltajeMaximo());
  Serial.println("V");
  Serial.println("");

  ESC.attach(SALIDA_ESC_PIN); // salida al ESC a D10
}

bool switchActivado()
{
  unsigned long value1 = pulseIn(RECEPTOR_CH5_PIN, HIGH, timeout_pwm_micros);
  unsigned long value2 = pulseIn(RECEPTOR_CH5_PIN, HIGH, timeout_pwm_micros);
  if ((value1 > 1600) && (value2 > 1600))
  {
    return false;
  }
  return true;
}

void test_funcion()
{
  static unsigned long ultima_interrupcion = 0;
  unsigned long tiempo_actual = micros() / 1000;
  if (tiempo_actual - ultima_interrupcion > 100) // debounce
  {
    test = !test;
    velocidadPWM = 0;
    siguientePaso = 0;
  }
  ultima_interrupcion = tiempo_actual;
}

void printSerial()
{
  escribe++;
  if (escribe == iteraciones_escritura)
  {
    escribe = 0;

    Serial.print("Limitador activado:");
    Serial.println(switchActivado());
    Serial.print("V: ");
    Serial.println(voltage_final);
    Serial.print("Vel: ");
    Serial.println(velocidadPWM);
    Serial.print("vAhora: ");

    Serial.println(vAhora);
  }
}

void loop()
{
  voltage_final = leerVoltaje();

  unsigned long value = pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  value += pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  value += pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  value += pulseIn(RECEPTOR_CH3_PIN, HIGH, timeout_pwm_micros);
  velocidadPWM = value / 4;

  if (test)
  {
    if (siguientePaso == 0)
    {
      softSer.println("<s-t>");
      delay(500);
      vAhora_real = 0;
      siguientePaso = millis() + TIEMPO_PASO_TEST;
    }
    if (millis() > siguientePaso)
    {
      siguientePaso = millis() + TIEMPO_PASO_TEST;
      vAhora_real++;
      if (vAhora_real <= 100)
      {
        sprintf(serialBuffer, "<v%03d", vAhora_real);
        softSer.println(serialBuffer);
      }
    }
    if (vAhora_real > 100)
    {
      test = 0;
      vAhora_real = 0;
      softSer.println("<e-t>");
    }
    velocidadPWM = map(vAhora_real, 0, 100, minimo_PWM, maximo_PWM);
  }

  if (voltage_final < voltage_max && vAhora >= velmin)
  {
    vAhora = vAhora - incremento;
  }
  else if (vAhora < velocidadPWM)
  {
    vAhora = vAhora + incremento;
  }
  else
  {
    vAhora = velocidadPWM;
  }

  if (!test)
  {
    velocidadReal = map(velocidadPWM, minimo_PWM, maximo_PWM, 0, 100);
    vAhora_real = map(vAhora, minimo_PWM, maximo_PWM, 0, 100);
  }
  if (vAhora_real < 0)
  {
    vAhora_real = 0;
  }
  if (velocidadReal < 0)
  {
    velocidadReal = 0;
  }

  readings[index] = voltage_final;
  index = index + 1;

  if (index >= lecturas)
  {
    index = 0; // ...volvemos al inicio:
  }
  Vtotal = 0;

  for (int i = 0; i < lecturas; i++)
  {
    Vtotal = Vtotal + readings[i];
  }

  Vmedia = Vtotal / lecturas;

  char *automatico;

  if (test)
  {
    automatico = (char *)"T";
    ESC.writeMicroseconds(velocidadPWM);
  }
  else
  {
    if (switchActivado())
    {
      ESC.writeMicroseconds(vAhora);
      automatico = (char *)"A";
    }
    else
    {
      ESC.writeMicroseconds(velocidadPWM);
      automatico = (char *)"M";

      vAhora_real = velocidadReal;
    }
  }

  char vA[10];
  dtostrf(vAhora_real, 5, 0, vA);
  char vR[10];
  dtostrf(velocidadReal, 5, 0, vR);
  char vo[10];
  dtostrf(voltage_final, 5, 2, vo);
  char voM[10];
  dtostrf(Vmedia, 5, 2, voM);
  //char Vis[10];

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_helvR14_tr);
    u8g2.drawStr(14, 14, vA);
    u8g2.drawStr(60, 14, vR);

    u8g2.drawStr(58, 12, "/");
    u8g2.drawStr(80, 64, voM);
    u8g2.setFont(u8g2_font_logisoso24_tr);
    u8g2.drawStr(50, 44, vo);

    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(0, 12, "Vel:");
    u8g2.drawStr(0, 34, "Voltaje:");
    u8g2.drawStr(0, 62, "Voltaje Medio:");

    u8g2.setFont(u8g2_font_tenstamps_mf);

    u8g2.drawStr(110, 12, automatico);

  } while (u8g2.nextPage());

  printSerial();
}

#endif
