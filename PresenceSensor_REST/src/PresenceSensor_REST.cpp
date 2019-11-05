#include <Arduino.h>
#include "konker.h"

// #define AUTO_REGISTRY

// local configuration 
#define FREQUENCY 60000
#define WIFI_RETRIES  40
// #define DEBUG_MSG
// #define RESETALL
#ifndef AUTO_REGISTRY

//Dados de Wifi
#ifdef _WS1 
const char* WIFI_SSID = _WS1;
#else 
const char* WIFI_SSID = "";
#endif
#ifdef _WP1 
const char* WIFI_PASS = _WP1;
#else
const char* WIFI_PASS = "";
#endif
#ifdef _WS2
const char* WIFI2_SSID = _WS2;
#else
const char* WIFI2_SSID = "";
#endif
#ifdef _WP2
const char* WIFI2_PASS = _WP2;
#else
const char* WIFI2_PASS = "";
#endif
#ifdef _WS3
const char* WIFI3_SSID = _WS3;
#else
const char* WIFI3_SSID = "";
#endif
#ifdef _WP3
const char* WIFI3_PASS = _WP3;
#else
const char* WIFI3_PASS = "";
#endif
#ifdef _KONKER_USER
const char* USER = _KONKER_USER;
#else
const char* USER = "";
#endif
#ifdef _KONKER_PASSWD
const char* PWD = _KONKER_PASSWD; 
#else
const char* PWD = "";
#endif
#endif 

// Dados do servidor
char* KONKER_SERVER_URL = "data.demo.konkerlabs.net";
char* KONKER_SERVER_PORT = "80";
char* SENSOR_TYPE = "S2V04";

//GPIO usado para o sensor de presenca (D1)
const int presence_gpio = D1;
const int led_pin = D3;

//Variaveis globais desse codigo
char _bufferJ[256];
int httpCode = 0;
int mov = 0;
int sensorValue = 0;
int freq_cnt = 0;
int i = 0;
int connection_error = 0;


char * jsonMQTTmsgDATAint(const char *device_id, const char *metric, int value)
{
  String output;
  StaticJsonDocument<200> jsonMSG;

  jsonMSG["name"] = device_id;
  jsonMSG["metric"] = metric;
  jsonMSG["value"] = value;
  jsonMSG["conn_err"] = connection_error;
  serializeJson(jsonMSG, output);
  strcpy(_bufferJ, output.c_str());

#ifdef DEBUG_MSG
  Serial.print("JSON: ");
  Serial.println(output);
#endif
  return _bufferJ;
}

void blink(int interval)
{
#ifdef DEBUG_MSG
  Serial.print("blink ");
  Serial.println(interval);
#endif
  
  digitalWrite(led_pin, HIGH);
  delay(interval);
  digitalWrite(led_pin, LOW);
  delay(interval);
}
  
void check_connection() 
{
  while (!tryConnectClientWifi()) {
    delay(500);
    blink(100);
#ifdef DEBUG_MSG
    Serial.println(">>> trying too connect again");
#endif
    connection_error++;
  }
}

bool WiFi_On()
{
  WiFi.forceSleepWake();
  delay(10);

  bool con_st = tryConnectClientWifi();

  if (! con_st)
  {
    connection_error++;
  }
#ifdef DEBUG_PY
  if (con_st)
  {
    Serial.println("[WF_ST] ON");
  }
  else
  {
    Serial.println("[WF_ER] ON");
  }
#endif
  return con_st;
}

bool WiFi_Off()
{
  int conn_tries = 0; 
  
  WiFi.mode(WIFI_OFF);
  delay(10);
  WiFi.forceSleepBegin();
  delay(10);

  while ((WiFi.status() == WL_CONNECTED) && (conn_tries++ < WIFI_RETRIES))
  {
    delay(100);

#ifdef DEBUG_MSG
    Serial.print(".");
#endif
  }

  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef DEBUG_PY
    Serial.println("[WF_ST] OFF");
#endif
    return true;
  }
  else
  {
#ifdef DEBUG_PY
    Serial.println("[WF_ER] OFF");
#endif
    return false;
  }
}

void setup()
{
#ifdef RESETALL
  Serial.begin(115200);
  delay(1*5*1000); 
  Serial.println("JUST TO RESET CONFIGURATION");
  resetALL();
#else
  pinMode(presence_gpio, INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.begin(115200);
  while (!Serial) delay(100);
#ifdef DEBUG_MSG
  Serial.println(F("\n"));    // <CR> past boot messages.
  Serial.print("Starting SENSOR REVISION ");
  Serial.println(PIO_SRC_REV);
#endif

  blink(500);
  mov = -1;

#ifndef AUTO_REGISTRY
#ifdef DEBUG_MSG
  Serial.println("SAVING CONFIGURATION DATA");
  Serial.print("WIFI_SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("WIFI_PASS: ");
  Serial.println(WIFI_PASS);
#endif
  set_platform_credentials((char*)KONKER_SERVER_URL, (char*)KONKER_SERVER_PORT, (char*)USER, (char*)PWD, "data");
  setWifiCredentialsNotEncripted((char*)WIFI_SSID, (char*)WIFI_PASS, 
    (char*)WIFI2_SSID, (char*)WIFI2_PASS, 
    (char*)WIFI3_SSID, (char*)WIFI3_PASS);
#else
    Serial.println("USING AUTO_REGISTRY");
#endif

  // set_factory_wifi("KonkerDevNetwork", "");
  // set_factory_gateway_addr("0.0.0.0:8081");
  
  WiFi.persistent(false);
  WiFi.disconnect();
  delay(10);
  WiFi.setAutoConnect(false);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(20.5f);

  konkerConfig(KONKER_SERVER_URL, SENSOR_TYPE, false);

  WiFi.setAutoReconnect(false);

#ifdef DEBUG_MSG
  Serial.println("Setup finished");
#endif
  (void)WiFi_Off();

#ifdef DEBUG_PY
  Serial.println("[GENE_] FIRST CONNECTION");
#endif
#endif
}

void loop()
{
#ifdef RESETALL
  delay(1000);
  Serial.println("WRITE NEW FIRMWARE !!!! THIS IS JUST TO RESET OLD FILES");
#else
  if (freq_cnt++ < FREQUENCY)
  {
    sensorValue += digitalRead(presence_gpio);
    delay(1);
  }
  else
  {
    if (WiFi_On())
    {
      konkerLoop();
#ifdef DEBUG_MSG
      Serial.print("Sending data! Count: ");
      Serial.println(sensorValue);
#endif
      int ret_code = 0;

      pubHttp("pir", jsonMQTTmsgDATAint("movement_detector", "Number", sensorValue), ret_code);
#ifdef DEBUG_MSG
      Serial.print("HTTP ret-code: ");
      Serial.println(ret_code);
#endif
#ifdef DEBUG_PY
      Serial.print("[HTTP_] ");
      Serial.println(ret_code);
#endif

      blink(200);
      mov=0;
      
#ifdef DEBUG_MSG
      Serial.println("WiFi Off");
      Stat_WiFi();
#endif
    }
    
    (void)WiFi_Off();

    sensorValue = 0;
    freq_cnt = 0;
  }
#endif
}
