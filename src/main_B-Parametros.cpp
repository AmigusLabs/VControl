#ifdef PLACA_PARAMETROS
#pragma "# Compilando para la placa de Parámetros #"

#include "voltaje.h"
#include "bascula.h"
#include "pantalla.h"

#include <Arduino.h>
#include <Servo.h>
//#include <EEPROM.h>
#include <Wire.h>
#include "Adafruit_SI1145.h"
#include <SoftwareSerial.h>

#define RESET_PIN 3
#define TARA_PIN 2
#define RECEPTOR_CH3_PIN A1

#define minimo_PWM 995
#define maximo_PWM 1989

int testIndice;
char serialBuffer[6];
void ejecutaBuffer();
int test = 0;
double testSuma = 0;
int testSumacontador = 0;

SoftwareSerial softSer(7, 8);

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset */ U8X8_PIN_NONE);
Adafruit_SI1145 uv = Adafruit_SI1145();

float factorCalibracionBascula = 74325.00 / (723.2 - 15);

int incomingByte = 0;

volatile int taremoi = 1;

const char *texto_version = "Power Module v4B";

float velMax;
float pesoMax;

float Visindex;
float IRindex;
float UVindex;

Servo ESC;

int vAhora_real = 0;

float peso = 0;

float voltage_final = 0;
int velocidadPWM = 1000;
int vAhora = 1000;
int velmin = 1000;
int incremento = 20;
int voltage_max = 12.0;

int iteraciones_escritura = 10;
int escribe = 0;

const int lecturas = 5; // Total lecturas V
int index = 0;          // El indice de la lectura actual
float Vtotal = 0.0;     // Total

float Ptotal;
float peso_media[lecturas]; // Lecturas de peso

void tare()
{
  taremoi = 1;
}

void reset_data()
{
  pesoMax = 0;
  velMax = 0;
}

void setup()
{
  analogReference(INTERNAL);
  Serial.begin(9600);
  softSer.begin(9600);

  //u8g2_SetI2CAddress(u8g2.getU8g2(), 0x78);
  u8g2.begin();
  pantallaBienvenida(u8g2, texto_version);

  if (!uv.begin())
  {
    Serial.println("Didn't find Si1145");
  }

  setupBascula();
  pinMode(TARA_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TARA_PIN), tare, FALLING);

  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), reset_data, FALLING);

  pinMode(RECEPTOR_CH3_PIN, INPUT);

  Serial.print("Voltaje Máximo: ");
  Serial.print(voltajeMaximo());
  Serial.println("V");
  Serial.println("");

  ESC.attach(10); // salida al ESC a D10

  serialBuffer[5] = '\0';
  //delay(2000);
}

void printSerial()
{
  escribe++;
  if (escribe == iteraciones_escritura)
  {
    escribe = 0;

    Serial.println();
    Serial.print("V: ");
    Serial.println(voltage_final);
    Serial.print("Vel: ");
    Serial.println(velocidadPWM);
    Serial.print("vAhora: ");
    Serial.println(vAhora);
    Serial.print("Fuerza: ");
    Serial.print(peso);
    Serial.println(" g");
    Serial.print("Vis: ");
    Serial.println(Visindex);
    Serial.print("IR: ");
    Serial.println(IRindex);
    Serial.print("UV: ");
    Serial.println(UVindex);
  }
}
void loop()
{

  voltage_final = leerVoltaje();

  velocidadPWM = pulseIn(RECEPTOR_CH3_PIN, HIGH);

  vAhora_real = map(velocidadPWM, minimo_PWM, maximo_PWM, 0, 100);
  if (vAhora_real < 0)
  {
    vAhora_real = 0;
  }
  if (taremoi != 0)
  {
    Serial.println("Calibrando Báscula");
    u8g2.firstPage();
    do
    {
      u8g2.setFont(u8g2_font_logisoso16_tr);
      u8g2.drawStr(5, 38, "@migus Labs");
      u8g2.setFont(u8g2_font_helvR08_tr);
      u8g2.drawStr(18, 54, texto_version);
      u8g2.setFont(u8g2_font_helvR08_tr);
      u8g2.drawStr(32, 64, "Calibrando...");
    } while (u8g2.nextPage());
    delay(200);
    taraBascula();
    delay(200);
    taremoi = 0;
  }

  calibracionBascula(factorCalibracionBascula);

  float peso_ahora = pesoBascula();
  peso_media[index] = peso_ahora;

  index = index + 1;
  if (index >= lecturas)
  {
    index = 0; // ...volvemos al inicio:
  }
  Ptotal = 0;
  for (int i = 0; i < lecturas; i++)
  {
    Ptotal = Ptotal + peso_media[i];
  }

  peso = Ptotal / lecturas; //

  if (peso < 0.09)
  {
    if (peso > -0.09)
    {
      peso = 0;
    }
  }

  if (peso > pesoMax)
  {
    pesoMax = peso;
    velMax = vAhora_real;
  }

  Visindex = uv.readVisible();
  IRindex = uv.readIR();
  UVindex = uv.readUV();
  UVindex /= 100.0;

  char g[10];
  dtostrf(peso, 5, 0, g);
  char vA[10];
  dtostrf(vAhora_real, 5, 0, vA);
  char vo[10];
  dtostrf(voltage_final, 5, 2, vo);
  char Vis[10];
  dtostrf(Visindex, 5, 0, Vis);
  char IR[10];
  dtostrf(IRindex, 5, 0, IR);
  char UV[10];
  dtostrf(UVindex, 5, 2, UV);

  char VM[10];
  dtostrf(velMax, 5, 0, VM);

  char PM[10];
  dtostrf(pesoMax, 5, 0, PM);

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_helvR14_tr);
    u8g2.drawStr(14, 14, vA);

    u8g2.drawStr(80, 14, vo);

    u8g2.drawStr(14, 30, VM);

    u8g2.drawStr(80, 30, PM);

    u8g2.setFont(u8g2_font_helvB24_tr);
    u8g2.drawStr(40, 64, g);
    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(122, 62, "g");
    u8g2.drawStr(0, 12, "Vel:");
    u8g2.drawStr(0, 28, "Max:");
    u8g2.drawStr(50, 28, " ==");
    u8g2.drawStr(60, 12, "V:");
    u8g2.drawStr(0, 44, "Vis:");
    u8g2.drawStr(0, 54, "IR:");
    u8g2.drawStr(0, 64, "UV:");
    u8g2.drawStr(20, 44, Vis); //Vis
    u8g2.drawStr(20, 54, IR);  //IR
    u8g2.drawStr(20, 64, UV);  //UV
    u8g2.setFont(u8g2_font_tenstamps_mf);
  } while (u8g2.nextPage());

  /* calibración báscula desactivada temporalmente
  if (Serial.available())
  {
    char temp = Serial.read();
    if (temp == '+' || temp == 'a')
    {
      factorCalibracionBascula += 10;
      EEPROM.put(0, factorCalibracionBascula);
    }
    else if (temp == '-' || temp == 'z')
    {
      factorCalibracionBascula -= 10;
      EEPROM.put(0, factorCalibracionBascula);
    }
  }
*/
  while (softSer.available())
  {
    char chr = softSer.read();
    // Serial.print(chr);
    //    return;
    if (chr == '\r')
    {
    }
    else if (chr == '\n')
    {
      ejecutaBuffer();
    }
    else
    {
      serialBuffer[0] = serialBuffer[1];
      serialBuffer[1] = serialBuffer[2];
      serialBuffer[2] = serialBuffer[3];
      serialBuffer[3] = serialBuffer[4];
      serialBuffer[4] = chr;
    }
  }
  if (test)
  {
    testSuma += peso;
    testSumacontador++;
  }
  //printSerial();
}

void ejecutaBuffer()
{
  if (strcmp(serialBuffer, "<s-t>") == 0)
  {
    // inicio del test
    test = 1;
    pesoMax = 0;
    testIndice = 0;
    testSuma = 0;
    testSumacontador = 0;
  }
  else if (strcmp(serialBuffer, "<e-t>") == 0)
  {
    // fin del test
    test = 0;
    testIndice = 0;
  }
  else
  {
    if (strncmp(serialBuffer, "<v", 2) == 0)
    {
      int testAnterior = testIndice;
      testIndice = atol(&serialBuffer[2]);
      if (testIndice > 0)
      {
        //Serial.print("X=");
        //Serial.print(testAnterior);
        //Serial.print(",Y=");
        Serial.print(testAnterior);
        Serial.print(",");
        Serial.println(testSumacontador == 0 ? 0 : testSuma / testSumacontador);
        testSuma = 0;
        testSumacontador = 0;
      }
    }
    else
    {
      Serial.print("Unknown command ");
      Serial.println(serialBuffer);
    }
  }
}

#endif
