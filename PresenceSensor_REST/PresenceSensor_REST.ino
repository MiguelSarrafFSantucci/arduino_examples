#include <Arduino.h>
// #include <ESP8266WiFi.h>
// #include <ArduinoJson.h> 
// #include <ESP8266HTTPClient.h>
#include <konker.h>

#define AUTO_REGISTRY

// local configuration 
#define FREQUENCY 60000

#ifndef AUTO_REGISTRY
WiFiClient espClient;
HTTPClient http;
//Dados de Wifi
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";
const char* USER = "";
const char* PWD = ""; 
#endif 

// Dados do servidor
char* KONKER_SERVER_URL="data.demo.konkerlabs.net";
int KONKER_SERVER_PORT=80;
char* SENSOR_TYPE = "S2V02";

//GPIO usado para o sensor de presenca (D1)
int presence_gpio = D1;
int led_pin = D3;

//Variaveis gloabais desse codigo
char _bufferJ[256];
int httpCode = 0;
int mov=0;
int sensorValue=0;
int i = 0;
int connection_error = 0;

char *jsonMQTTmsgDATAint(const char *device_id, const char *metric, int value)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["name"] = device_id;
        jsonMSG["metric"] = metric;
        jsonMSG["value"] = value;
        jsonMSG["conn_err"] = connection_error;
        jsonMSG.printTo(_bufferJ, sizeof(_bufferJ));
        return _bufferJ;
  }
  
void blink(int interval)
{
  Serial.print("blink ");
  Serial.println(interval);
  
  digitalWrite(led_pin, HIGH);
  delay(interval);
  digitalWrite(led_pin, LOW);
  delay(interval);
}
  
void check_connection() 
{
  int i =0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    blink(100);
    i++;
    Serial.print(".");
    if (i > 100) { 
      Serial.println("waiting to connect (1 minute)");
      Serial.print("WIFI: ");
      Serial.println(WiFi.SSID());
      #ifndef AUTO_REGISTRY
      Serial.print("PASSWD: '");
      Serial.print(WIFI_PASS);
      #endif
      Serial.println("'");
      delay(1*60*1000); 
      i = 0;
      connection_error++;
    }
  }
}

void setup()
{

  #ifdef RESETALL
  resetALL();
  Serial.println("JUST TO RESET CONFIGURATION");
  #else
  char server_port[255];
  snprintf(server_port, 254, "%d", KONKER_SERVER_PORT);

  pinMode(presence_gpio, INPUT);
  pinMode(led_pin, OUTPUT);
  Serial.begin(115200);

  Serial.print("Starting SENSOR REVISION ");
  Serial.println(PIO_SRC_REV);

  blink(500);
  mov = -1;

#ifndef AUTO_REGISTRY
  // WiFi.begin(WIFI_SSID,WIFI_PASS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.setAuthorization(USER, PWD);
  Serial.print("WIFI_SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("WIFI_PASS: ");
  Serial.println(WIFI_PASS);
  set_platform_credentials((char*)KONKER_SERVER_URL, server_port, (char*)USER, (char*)PWD, "data");
  setWifiCredentialsNotEncripted((char*)WIFI_SSID, (char*)WIFI_PASS);
#endif 

  set_factory_wifi("KonkerDevNetwork", "");
 // set_factory_gateway_addr("0.0.0.0:8081");
  
  konkerConfig(KONKER_SERVER_URL,SENSOR_TYPE,false);

  check_connection();

  Serial.println("Setup finished");
  #endif
}



void loop()
{
  konkerLoop();
  #ifdef RESETALL
  delay(1000);
  Serial.println("WRITE NEW FIRMWARE !!!! THIS IS JUST TO RESET OLD FILES");
  #else
  WiFi.forceSleepBegin(60000000);
  delay(1);
  sensorValue=0;
  // load data from sensor with predefined FREQUENCY
  for (i=0; i<FREQUENCY; i++) {
    sensorValue+= digitalRead(presence_gpio);
    delay(1);
  }
  WiFi.forceSleepWake();
  delay(1);
  Serial.print("Sending data! Count: ");
  Serial.println(sensorValue);

  check_connection();

  int ret_code;

  pubHttp("pir", jsonMQTTmsgDATAint("movement_detector", "Number", sensorValue), ret_code);
  
  Serial.print("HTTP ret-code: ");
  Serial.println(ret_code);

  blink(200);
  mov=0;
  #endif
}