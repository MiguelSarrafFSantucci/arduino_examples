#include <Arduino.h>
// #include <ESP8266WiFi.h>
// #include <ArduinoJson.h> 
// #include <ESP8266HTTPClient.h>
#include <konker.h>

// #define AUTO_REGISTRY

// local configuration 
#define FREQUENCY 60000

#ifndef AUTO_REGISTRY
WiFiClient espClient;
HTTPClient http;
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
  
void check_connection_old() 
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

void check_connection() 
{
  int i =0;
  while (!tryConnectClientWifi()) {
    delay(500);
    blink(100);
    Serial.println(">>> trying too connect again");
    connection_error++;
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
    char server_port[255];
    snprintf(server_port, 254, "%d", KONKER_SERVER_PORT);

    pinMode(presence_gpio, INPUT);
    pinMode(led_pin, OUTPUT);
    Serial.begin(115200);
    delay(1*5*1000); 

    Serial.print("Starting SENSOR REVISION ");
    Serial.println(PIO_SRC_REV);

    blink(500);
    mov = -1;

    #ifndef AUTO_REGISTRY
      Serial.println("SAVING CONFIGURATION DATA");
      // WiFi.begin(WIFI_SSID,WIFI_PASS);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept", "application/json");
      http.setAuthorization(USER, PWD);
      Serial.print("WIFI_SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("WIFI_PASS: ");
      Serial.println(WIFI_PASS);
      set_platform_credentials((char*)KONKER_SERVER_URL, server_port, (char*)USER, (char*)PWD, "data");
      setWifiCredentialsNotEncripted((char*)WIFI_SSID, (char*)WIFI_PASS, 
        (char*)WIFI2_SSID, (char*)WIFI2_PASS, 
        (char*)WIFI3_SSID, (char*)WIFI3_PASS);
    #endif 

    // set_factory_wifi("KonkerDevNetwork", "");
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