/** 
 * SCALE 
 * ------------------------------------------------------------------------------------------
 * implements a program to communicate with scale (michaletti A-100) 
 * and send the current weight information to KONKER platform
 * 
 * version | developer | comment
 * ------------------------------------------------------------------------------------------
 * 1.0       alexjunq    first implementation and usage of KONKER virtual device abstraction
 * ------------------------------------------------------------------------------------------
 * copyright(c) by Konker 2018
 */

// uncomment the following line to generated debug information on Serial line 
// to be monitored using a serial terminal 

// #define DEBUG  

#define PIO_SRC_REV 123

#include "Arduino.h"
#include "scale.h"
#include "measurement.h"
#include <konker.h>


// local configuration 
#define FREQUENCY 60000

WiFiClient espClient;
HTTPClient http;
//Dados de Wifi
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";
const char* USER = "";
const char* PWD = ""; 

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
  

#define __APP_VERSION__ "1.6"

// create a scale device (physical) and inform digital PINS used to create a software serial 
// communication 
Scale scale(D7,D6);
// create a logical konker device to link to the physical device 
#define SERIAL_BAUD 115200

void check_connection() 
{
  int i =0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    i++;
    Serial.print(".");
    if (i > 100) { 
      Serial.println("waiting to connect (1 minute)");
      Serial.print("WIFI: ");
      Serial.println(WiFi.SSID());
      Serial.print("PASSWD: '");
      Serial.print(WIFI_PASS);
      Serial.println("'");
      delay(1*60*1000); 
      i = 0;
      connection_error++;
    }
  }
}

/** 
 * initialize the program, creating a new logical device, and linking it to the physical device
 */
void setup() {

    Serial.begin(SERIAL_BAUD);


  char server_port[255];
  snprintf(server_port, 254, "%d", KONKER_SERVER_PORT);

//  resetALL();
      mov = -1;

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

  konkerConfig((char*)"data.demo.konkerlabs.net:80",(char*)"S0X0X",false);

  check_connection();

  Serial.println("Setup finished");
 

}


/** 
 * main loop of the app 
 * just check connection, and if available, make a reading on the physical connected device 
 * this read triggers internally the logic to verify if the reading need to be replicated
 * to the logical device ... if so, maintain it synchronized automatically 
 * 
 */
void loop() {

    konkerLoop();

    Measurement *m; 

    if (scale.getState()) {
 
        scale.read();
        m = scale.getCurrent();
        if (m) {
          m->dump();
        }

        if (m->weight() > 10) {
            int ret_code;
            Serial.println("-------------------> SENDING TO PLATFORM ---------");
            Serial.println(m->weight());
            pubHttp("weight", jsonMQTTmsgDATAint((char*)USER, "Number", m->weight()), ret_code);
            Serial.print("HTTP ret-code: ");
            Serial.println(ret_code);
        }
    }
    else  {
        Serial.println("setting up scale...");
        scale.begin();
    }
    yield();

}
