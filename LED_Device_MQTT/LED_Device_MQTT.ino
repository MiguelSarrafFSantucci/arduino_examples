/*
  Exemplo básico de conexão a Konker Plataform via MQTT, 
  baseado no https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_auth/mqtt_auth.ino. 
  Este exemplo se utiliza das bibliotecas do ESP8266 programado via Arduino IDE 
  (https://github.com/esp8266/Arduino) e a biblioteca PubSubClient que pode ser 
  obtida em: https://github.com/knolleary/pubsubclient/
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

#define PIN01 D1
#define PIN02 D2
#define PIN03 D3
 
// Vamos primeiramente conectar o ESP8266 com a rede Wireless (mude os parâmetros abaixo para sua rede).

// Dados da rede WiFi
const char* ssid = "";
const char* password = "";

// Dados do servidor MQTT
const char* mqtt_server = "";

const char* USER = "";
const char* PWD = "";

const char* PUB = "";
const char* SUB = "";

//Variaveis de controle de mensagem
char msgBufferIN[2048];
char msgTopic[32];
int received_msg=0;

float temperature=0;

//Criando os objetos de conexão com a rede e com o servidor MQTT.
WiFiClient espClient;
PubSubClient client(espClient);

float jsonMQTT_temperature_msg(const char msg[])
{
   const int capacity = 4*JSON_OBJECT_SIZE(3);
   float temperatura;
   StaticJsonDocument<capacity> jsonMSG;
   DeserializationError err = deserializeJson(jsonMSG, msg);
   if (err)
   {
     Serial.println("ERRO.");
     return 0;
   }
   if (jsonMSG.containsKey("value")) 
   { 
     temperatura = jsonMSG["value"].as<float>();
   }   
   return temperatura;
}



//Criando a função de callback
//Essa funcao eh rodada quando uma mensagem eh recebida via MQTT.
//Nesse caso ela eh muito simples: imprima via serial o que voce recebeu
void callback(char* topic, byte* payload, unsigned int length) 
{
  int i;
  for (i = 0; i < length; i++) msgBufferIN[i] = payload[i];
  msgBufferIN[i] = '\0';
  strcpy(msgTopic, topic);
  received_msg = 1;
  Serial.print("Mensagem Recebida: ");
  Serial.println(msgBufferIN);
  Serial.print("topico: ");
  Serial.println(topic);
}

void reconnect() {
  // Entra no Loop ate estar conectado
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Tentando conectar
    if (client.connect(USER, USER, PWD)) 
    {
      Serial.println("connected");
      // Subscrevendo no topico esperado
      client.subscribe(SUB);
    } 
    else 
    {
      Serial.print("Falhou! Codigo rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      // Esperando 5 segundos para tentar novamente
      delay(5000);
    }
  }
}

void setup_wifi() 
{
  delay(10);
  // Agora vamos nos conectar em uma rede Wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    //Imprimindo pontos na tela ate a conexao ser estabelecida!
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereco de IP: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  //Configurando a porta Serial, os pinos de IO e escolhendo o servidor MQTT
  Serial.begin(115200);
  pinMode(PIN01, OUTPUT);
  pinMode(PIN02, OUTPUT);
  pinMode(PIN03, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop()
{
  //O programa em si eh muito simples: 
  //se nao estiver conectado no Broker MQTT, se conecte!
  if (!client.connected()) 
    {
      reconnect();
    }
    
  //Se receber uma mensagem, analisar o conteudo e ajustar os LEDs  
  if (received_msg==1)
    {
      received_msg=0;
      temperature = jsonMQTT_temperature_msg(msgBufferIN);
      if (temperature>10.0) digitalWrite(PIN01, HIGH);
      else digitalWrite(PIN01, LOW);
      if (temperature>20.0) digitalWrite(PIN02, HIGH);
      else digitalWrite(PIN02, LOW);
      if (temperature>30.0) digitalWrite(PIN03, HIGH);
      else digitalWrite(PIN03, LOW);
    }
  client.loop();
  delay(100);
}

