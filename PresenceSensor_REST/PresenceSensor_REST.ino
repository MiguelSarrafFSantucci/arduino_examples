#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h> 
#include <ESP8266HTTPClient.h>

WiFiClient espClient;
HTTPClient http;

//Dados de Wifi
const char* WIFI_SSID = "Guest";
const char* WIFI_PASS = "";

// Dados do servidor
const char* USER = "";
const char* PWD = ""; 
const char* PUB_pir = "";

//GPIO usado para o sensor de presenca (D1)
int presence_gpio = D1;
int led_pin = D3;

//Variaveis gloabais desse codigo
char bufferJ[256];
int httpCode = 0;
int mov=0;

char *jsonMQTTmsgDATAint(const char *device_id, const char *metric, int value)
  {
      StaticJsonBuffer<200> jsonMQTT;
      JsonObject& jsonMSG = jsonMQTT.createObject();
        jsonMSG["name"] = device_id;
        jsonMSG["metric"] = metric;
        jsonMSG["value"] = value;
        jsonMSG.printTo(bufferJ, sizeof(bufferJ));
        return bufferJ;
  }
  
void handleInterrupt4() {
  //Serial.println("Interrupt GPIO4 !!!");
  mov++;
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
    if (i > 100) ESP.deepSleep(300*1000000, WAKE_RF_DEFAULT); //Tentar conectar por 50 segundos, caso nao consiga, dormir por 5 minutos
  }
}

void setup()
{
  pinMode(presence_gpio, INPUT);
  pinMode(led_pin, OUTPUT);
  Serial.begin(115200);
  blink(500);
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  check_connection();
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.setAuthorization(USER, PWD);
}



void loop()
{
  WiFi.forceSleepBegin(60000000);
  delay(1);
  attachInterrupt(digitalPinToInterrupt(presence_gpio), handleInterrupt4, RISING);
  delay(60000);
  detachInterrupt(digitalPinToInterrupt(presence_gpio));
  WiFi.forceSleepWake();
  delay(1);
  Serial.println("Sending data!");
  check_connection();
  http.begin("data.demo.konkerlabs.net",80,PUB_pir);
  httpCode=http.POST(jsonMQTTmsgDATAint("movement_detector", "Number", mov));
  Serial.println(httpCode);
  http.end();
  blink(200);
  mov=0;
}

