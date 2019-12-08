/*
  Exemplo básico de conexão a Konker Plataform via HTTP, 
  Este exemplo se utiliza das bibliotecas do ESP8266 programado via Arduino IDE 
  (https://github.com/esp8266/Arduino).
*/
#define ARDUINOJSON_USE_LONG_LONG 1

//Inserindo os dados do termistor usado

// Resistencia nominal a 25C (Estamos utilizando um MF52 com resistencia nominal de 1kOhm)
#define TERMISTORNOMINAL 1000      
// Temperatura na qual eh feita a medida nominal (25C)
#define TEMPERATURANOMINAL 25   
//Quantas amostras usaremos para calcular a tensao media (um numero entre 4 e 10 eh apropriado)
#define AMOSTRAS 4
// Coeficiente Beta (da equacao de Steinhart-Hart) do termistor (segundo o datasheet eh 3100)
#define BETA 3100
// Valor da resistencia utilizada no divisor de tensao (para temperatura ambiente, qualquer resistencia entre 470 e 2k2 pode ser usada)
#define RESISTOR 470   

#define PIN01 D1
#define PIN02 D2
#define PIN03 D3

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h> 
#include <ESP8266HTTPClient.h>

// Vamos primeiramente conectar o ESP8266 com a rede Wireless (mude os parâmetros abaixo para sua rede).

// Dados da rede WiFi
const char* ssid = "";
const char* password = "";

// Dados do dispositivo e servidor HTTP/REST
const char* USER = "";
const char* PWD = "";

const char* http_publication_url = "";
const char* http_subscription_url = "";


//Variaveis gloabais desse codigo
char bufferJ[256];
char *mensagem;
String payload;
int httpCode = 0;
uint64_t timestamp = 0;
uint64_t last_timestamp = 0;

//Variaveis do termometro
float temperature;
float received_temperature;
float tensao;
float resistencia_termistor;
int i = 0;


//Vamos criar uma funcao para formatar os dados no formato JSON
char *jsonMQTTmsgDATA(const char *device_id, const char *metric, float value) {
  const int capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> jsonMSG;
  jsonMSG["deviceId"] = device_id;
  jsonMSG["metric"] = metric;
  jsonMSG["value"] = value;
  serializeJson(jsonMSG, bufferJ);
  return bufferJ;
}
//E outra funcao para ler os dados
float jsonMQTT_temperature_msg(String msg)
{
   const int capacity = JSON_ARRAY_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + 190;
   float temperatura;
   //msg = msg.substring(1,msg.length()-1);
   StaticJsonDocument<capacity> jsonMSG;
   DeserializationError err = deserializeJson(jsonMSG, msg);
   if (err)
   {
     Serial.print("ERRO: ");
     Serial.println(err.c_str());
     return 0;
   }
   JsonObject root_0_data = jsonMSG[0]["data"]; 
   temperatura = root_0_data["value"].as<float>(); 
   JsonObject root_0_meta = jsonMSG[0]["meta"];
   timestamp = root_0_meta["timestamp"].as<unsigned long long>();
   
   return temperatura;
}
//Criando os objetos de conexão com a rede e com o servidor HTTP.
WiFiClient espClient;
HTTPClient http;

void setup_wifi() {
  delay(10);
  // Agora vamos nos conectar em uma rede Wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Imprimindo pontos na tela ate a conexao ser estabelecida!
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereco de IP: ");
  Serial.println(WiFi.localIP());
  http.begin("data.demo.konkerlabs.net",80,http_subscription_url);
  httpCode = http.GET();
  if (httpCode>0) {
    payload = http.getString();
    received_temperature = jsonMQTT_temperature_msg(payload);
    last_timestamp = timestamp;
  }
}

void setup()
{
  //Configurando a porta Serial e escolhendo o servidor MQTT
  Serial.begin(115200);
  setup_wifi();
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.setAuthorization(USER, PWD);
  
}

void loop()
{
  //O programa em si eh muito simples: 
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  
  tensao = 0;
  
  //Tirando a media do valor lido no ADC
  for (i=0; i< AMOSTRAS; i++) {
   tensao += analogRead(0)/AMOSTRAS;
   delay(10);
  }
  //Calculando a resistencia do Termistor
  resistencia_termistor = RESISTOR*tensao/(1023-tensao);
  //Equacao de Steinhart-Hart
  temperature = (1 / (log(resistencia_termistor/TERMISTORNOMINAL) * 1/BETA + 1/(TEMPERATURANOMINAL + 273.15))) - 273.15;
  //Vamos imprimir via Serial o resultado para ajudar na verificacao
  Serial.print("Resistencia do Termistor: "); 
  Serial.println(resistencia_termistor);
  Serial.print("Temperatura: "); 
  Serial.println(temperature);
  
  mensagem = jsonMQTTmsgDATA("My_favorite_thermometer", "Celsius", temperature);
  //Enviando via MQTT o resultado calculado da temperatura
  http.begin("data.demo.konkerlabs.net",80,http_publication_url);
  httpCode=http.POST(mensagem);
  Serial.print("Codigo de resposta: ");
  Serial.println(httpCode);
  http.end();
  http.begin("data.demo.konkerlabs.net",80,http_subscription_url);
  httpCode = http.GET();
  if (httpCode>0){
    payload = http.getString();
    received_temperature = jsonMQTT_temperature_msg(payload);
    if (timestamp > last_timestamp){
      Serial.print("Mensagem recebida da plataforma: ");
      Serial.println(payload); 
      if (received_temperature>10.0) digitalWrite(PIN01, HIGH);
        else digitalWrite(PIN01, LOW);
      if (received_temperature>20.0) digitalWrite(PIN02, HIGH);
        else digitalWrite(PIN02, LOW);
      if (received_temperature>30.0) digitalWrite(PIN03, HIGH);
        else digitalWrite(PIN03, LOW);
    last_timestamp = timestamp;
    }
   Serial.println("");
  }
  
  //Gerando um delay de 2 segundos antes do loop recomecar
  delay(2000);
}
