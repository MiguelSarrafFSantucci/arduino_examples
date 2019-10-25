#include <Ticker.h>
#include <ArduinoJson.h> 
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>


#define REED_PIN       D5
#define WIFI_RETRIES   100 // Number of times to test WiFi connection status before giving up
#define HEALTH_TIME    300


const char *ssid   = "";         // WIFI (SSID)
const char *password = "";   // WIFI password

// Dados da plataforma
const char* USER = "";
const char* PWD = "";
const char* KONKER_SERVER_URL = "data.demo.konkerlabs.net";
const int KONKER_SERVER_PORT = 80;

Ticker send_health;

int connection_errors = 0;
unsigned long ref_open_time = 0;
int health_flag = 0;

int pubHTTP(String channel, const String & message)
{
    WiFiClient client;
    HTTPClient http;
    int httpCode = -1;

    String pub_dir = "/pub/";
    pub_dir += USER;
    pub_dir += "/" + channel;

    // Serial.print("PUB_DIR = ");
    // Serial.println(pub_dir);

    if (http.begin(client, KONKER_SERVER_URL, KONKER_SERVER_PORT, pub_dir))
    {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Accept", "application/json");
        http.setAuthorization(USER, PWD);
        httpCode = http.POST(message);

        if (httpCode == HTTP_CODE_OK)
        {
            (void)http.getString();
            // String payload = http.getString();
            // Serial.print("HTTP resp: ");
            // Serial.println(payload);
        }
        http.end();
    }

    return httpCode;
}

String jsonSensor(int door_state, unsigned long open_time)
{
  String output;
  StaticJsonDocument<200> jsonMSG;

  jsonMSG["state"] = door_state;
  jsonMSG["time"] = open_time;
  serializeJson(jsonMSG, output);

  Serial.print("JSON: ");
  Serial.println(output);

  return output;
}

String jsonHealth(uint32_t conn_er, const String & net, const String & mac, const String & ip, int32_t rssi)
{
  String output;
  StaticJsonDocument<500> jsonMSG;
  JsonObject root = jsonMSG.to<JsonObject>();

  JsonObject info = root.createNestedObject("info");
  info["mac"] = mac;
  info["net"] = net;
  info["ip"] = ip;
  info["rssi"] = rssi;

  JsonObject errors = root.createNestedObject("errors");
  errors["connection"] = conn_er;

  serializeJson(jsonMSG, output);

  Serial.print("JSON: ");
  Serial.println(output);

  return output;
}


void health_msg()
{
    health_flag = 1;
}

void checkDoorState()
{
    static int reedState = LOW;
    static int lastReedState = LOW;
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;
        
    int reading = digitalRead(REED_PIN);

    if (reading != lastReedState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != reedState)
        {
            String sensor_json;
            reedState = reading;

            if (reedState == HIGH)
            {
                sensor_json = jsonSensor(!reedState, millis() - ref_open_time);

                // Serial.println("Fechado");
            }
            else
            {
                ref_open_time = millis();                

                sensor_json = jsonSensor(!reedState, 0);

                // Serial.println("Aberto");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                pubHTTP("DOOR", sensor_json);
            }
            else
            {
                 ++connection_errors;
            }
        }
    }

    lastReedState = reading;
}


void setup()
{
    int conn_tries = 0;

    Serial.begin(115200);
    while (!Serial) delay(100);

    Serial.println();
    Serial.print(F("Core: "));
    Serial.println(ESP.getCoreVersion());
    Serial.print(F("SDK: "));
    Serial.println(ESP.getSdkVersion());

    WiFi.persistent(false);
    WiFi.disconnect();
    delay(10);
    WiFi.setAutoConnect(false);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.setOutputPower(20.5f);
    WiFi.mode(WIFI_STA);
    delay(10);
    WiFi.setAutoReconnect(true);

    Serial.println(F("Connecting Wifi..."));

    WiFi.begin(ssid, password);
    yield();
    while ((WiFi.status() != WL_CONNECTED) && (conn_tries++ < WIFI_RETRIES))
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (conn_tries == WIFI_RETRIES)
    {
        Serial.println(F("No WiFi connection ...restarting."));

        ESP.reset();
    }

    WiFi.printDiag(Serial);
    Serial.println();

    send_health.attach(HEALTH_TIME, health_msg);

    delay(1000);

    ref_open_time = millis();
}

void loop()
{
    checkDoorState();

    if (health_flag)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            pubHTTP("_health", jsonHealth(connection_errors, WiFi.SSID(), WiFi.macAddress(), WiFi.localIP().toString(), WiFi.RSSI()));
        }
        else
        {
            ++connection_errors;
        }
        health_flag = 0;
    }
}
