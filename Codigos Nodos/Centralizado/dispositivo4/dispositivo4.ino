//Script para NodeMCU

//Librerias
#include <stdio.h>
#include <string.h>
#include <espnow.h>
#include <Gaussian.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WifiEspNowBroadcast.h>

//Variables de tiempo
unsigned long tiempoActual;
unsigned long tAvanzarHora;
unsigned long generarDatos;

//Información del broker
uint8_t broker[6]; //Arreglo donde se guarda la información del broker
int broker_anadido=0;

//Modo DISCOVERY
const unsigned long eventInterval=25000; //Tiempo en milisegundos que se permanecerá en modo DISCOVERY

//Definir el pin del botón "flash" para activar el modo DISCOVERY 
static const int BUTTON_PIN = 0;
//Definir el pin del led azul de la tarjeta 
static const int LED_PIN = 2;
int ledState = HIGH;

//Definir variables de envío de mensajes unicast
int stat;
int hora=13;
int awning;
int awningPrevio;
int lights;
int lightsPrevio;
int presence_;
char *envJson;
int evento1=0;
int evento2=0;
float co2_level;
unsigned long tpregunta;
char dato[2][10]={"illuminan", "envi_temp"};

//Definir variables de recibo de mensajes unicast
float illuminan;
float envi_temp;
char *pregunta;

//Funciones de prueba **********************************BORRAR********************************************************
unsigned long timerDiscovery;
unsigned long timerRR;
unsigned long timerRB;

void verDatos(){
  Serial.printf("illuminan: %f", illuminan);Serial.println();
}
//Funciones de prueba **********************************BORRAR********************************************************

//Funciones de envío de mensajes de DISCOVERY con el uso de ESP-NOW
void modoDiscovery(){
  Serial.println();
  Serial.println("Iniciando modo DISCOVERY");
  esp_now_deinit();
  ledState = 1 - ledState;
  digitalWrite(LED_PIN, ledState);
  WiFi.persistent(false);
  bool ok = WifiEspNowBroadcast.begin("ESPNOW", 1);
  if (!ok) {
    Serial.println("WifiEspNowBroadcast.begin() failed");
    ESP.restart();
  }
  unsigned long currentTime = millis();
  while (currentTime + eventInterval >= millis()) {
    sendMessage();
    WifiEspNowBroadcast.loop();
    delay(10);
  }
  ledState = 1 - ledState;
  digitalWrite(LED_PIN, ledState);
  WifiEspNowBroadcast.end();
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA); //Set device as wi-fi station
  WiFi.disconnect();
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb (OnDataSent);
  esp_now_register_recv_cb (OnDataRecv);
  if (broker_anadido>0){addpeers();}
  Serial.println("Finalizando modo DISCOVERY");
  Serial.println();
}

void sendMessage(){
  char msg[60];
  int len = snprintf(msg, sizeof(msg), "4.co2_level;presence;awning,lights,");
  WifiEspNowBroadcast.send(reinterpret_cast<const uint8_t*>(msg), len);
}

void processRx(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Condición para ver si la dirección ya se encuentra registrada
  if (broker_anadido==1){return;} 
  uint8_t* dispositivo = const_cast<uint8_t*>(mac); //Cast de la dirección MAC de const a no const
  dispositivo[0]=dispositivo[0]-2;
  //Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", dispositivo[0], dispositivo[1], dispositivo[2], dispositivo[3], dispositivo[4], dispositivo[5]); //Imprimir dirección mac del dispositivo que envía el mensaje
  char* informacion = reinterpret_cast<char*>(const_cast<uint8_t*>(buf));
  char tipo[7];
  strncpy(tipo, informacion, 6);
  
  if (strcmp("broker",tipo)==0){
    for (size_t j=0; j<6; j++){
      broker[j]=dispositivo[j];
    }
    broker_anadido=1;
    timerDiscovery=millis();
    Serial.printf("tiempo: %lu",timerDiscovery);Serial.println();
  }
}

//Funciones para el envío y recepción de datos mediante el protocolo ESP-NOW
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  //Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    //Serial.println("Delivery success");
    stat=1;
  }
  else{
    //Serial.println("Delivery fail");
    stat=0;
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  char *buff = (char*) incomingData;
  //Serial.println(buff);
  if (strcmp(buff, "status")==0){return;}
  int resp = deserializarJson(buff);
  if (resp==1){
    //Serial.printf("pregunta:%s", pregunta);Serial.println();
    if (strcmp("presence_", pregunta)==0){
      generarRespuesta("presence_", presence_);
      //Serial.println("Respuesta enviada");
    }
    else if (strcmp("co2_level", pregunta)==0){
      generarRespuesta("co2_level", co2_level);
      //Serial.println("Respuesta enviada");
    }
  }
  //memset(pregunta, 0, 15);
}

void addpeers(){
  esp_now_add_peer(broker, ESP_NOW_ROLE_COMBO, 4, NULL,0);
  Serial.println("broker added");
}

//Funciones para la generación de datos
int generarPresencia(int hora){
  int media[24]={50,50,50,50,50,53,65,70,75,80,65,60,55,50,50,50,50,60,70,60,50,50,50,50};
  int numero=random(0,media[hora])+1;
  if (numero>50){return 1;}
  else {return 0;}
}

double generarCO2(int hora){
  double media[24]={1000.0, 980.0, 930.0, 900.0, 800.0, 700.0, 700.0, 700.0, 800.0, 700.0, 650.0, 650.0, 600.0, 550.0, 500.0, 550.0, 600.0, 650.0, 750.0, 800.0, 800.0, 850.0, 900.0, 950.0};
  Gaussian generador = Gaussian(media[hora], 1.2); 
  double numero = generador.random();
  return numero;
}

void generarPregunta(){
  ////timerRR=millis();
  String json;
  long randomNumber = random(1,3)-1;
  StaticJsonDocument<48> doc;
  doc["pregunta"] = dato[randomNumber];
  serializeJson(doc, json);
  uint8_t *buffer = (uint8_t*) json.c_str();
  size_t sizeBuff = sizeof(buffer) * json.length();
  ////Serial.println(json);
  esp_now_send(broker, buffer, sizeBuff);
}

int deserializarJson(char* input){
  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return -1;
  }
  bool tienePregunta = doc.containsKey("pregunta");
  bool tieneError = doc.containsKey("error");
  bool tieneI = doc.containsKey("illuminan");
  bool tieneET = doc.containsKey("envi_temp");
  bool tieneA = doc.containsKey("awning");
  bool tieneL = doc.containsKey("lights");
  
  if (tieneError){
    //Serial.println("Se recibio un error");
    ////unsigned long tempo=millis()-timerRB;
    ////Serial.printf("T evento:%lu", tempo);Serial.println();
    return -1;
  }

  else if (tienePregunta){
    const char *cast = doc["pregunta"];
    pregunta=(char*)cast;
    //Serial.printf("Se ha recibido una pregunta:%s", pregunta);Serial.println();
    return 1; 
  }
  
  else if (tieneI){
    illuminan=doc["illuminan"];
    ////unsigned long respuesta = millis()-timerRR;
    ////Serial.printf("R: %lu", respuesta);Serial.println();
    return 0;
  }

  else if (tieneET){
    envi_temp=doc["envi_temp"];
    ////unsigned long respuesta = millis()-timerRR;
    ////Serial.printf("R: %lu", respuesta);Serial.println();
    return 0;  
  }

  else if (tieneA){
    awningPrevio=awning;
    awning=doc["awning"];
    if(awning==1 and awningPrevio==0){
      Serial.println("Se bajo el toldo!!!");
    }
    return 0;  
  }

  else if (tieneL){
    lightsPrevio=lights;
    lights=doc["lights"];
    if(lights==1 and lightsPrevio==0){
      Serial.println("Se encendieron las luces!!!");
    }
    return 0;  
  }
}

void generarRespuesta(char *dato, float variable){
  String json;
  StaticJsonDocument<48> doc;
  doc[dato] = variable;
  serializeJson(doc, json);
  //Serial.println(json);
  uint8_t *buffer = (uint8_t*) json.c_str();
  size_t sizeBuff = sizeof(buffer) * json.length();
  esp_now_send(broker, buffer, sizeBuff);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  randomSeed(analogRead(A0));
  
  WifiEspNowBroadcast.onReceive(processRx, nullptr);

  modoDiscovery();

  tpregunta=millis();
  tAvanzarHora=millis();
  generarDatos=millis();
  co2_level=generarCO2(hora);
}

void loop() {
 tiempoActual=millis();
 if (digitalRead(BUTTON_PIN) == LOW and broker_anadido==0) {
    modoDiscovery();
 }

 if (broker_anadido==1){
  if (tiempoActual-generarDatos>10000){ //Generar datos cada 10 segundos
    generarDatos=millis();
    co2_level=generarCO2(hora);
    evento1=0;
    evento2=0;
  }
  if (tiempoActual>(tpregunta+15000)){ //Preguntar dato random cada 15 segundos
    tpregunta=millis();
    generarPregunta();
    //verDatos();
  }
  if (tiempoActual>(tAvanzarHora+150000)){ //Avanzar hora 150 segundos (cada 2.5 min)
    tAvanzarHora=millis();
    hora++;
    if (hora>24){hora=0;}
    presence_=generarPresencia(hora);
  }
  if (presence_==1 and evento1==0){
    evento1=1;
    ////timerRB=millis();
    generarRespuesta("presence_", presence_);
  }
  if ((co2_level<=300 or co2_level>=1000)and evento2==0){
    evento2=1;
    ////timerRB=millis();
    generarRespuesta("co2_level", co2_level);
  }
  if (envi_temp<34 and awning==1){
    awning=0;
    Serial.println("Se subio el toldo");
  }
  if (illuminan>500 and lights==1){
    lights=0;
    Serial.println("Se apago el sistema de iluminacion");
  }
 }
 else{
  if (tiempoActual-generarDatos>10000){
    generarDatos=millis();
    Serial.println("Broker no añadido");
  }
 }
}
