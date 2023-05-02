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
float envi_humi;
float envi_temp;
int sprinklers;
int sprinklersPrevio;
float illuminan;
unsigned long tpregunta;
char *envJson;
char dato[10]={"soil_humi"};
int evento1=0;
int evento2=0;
int evento3=0;

//Definir variables de recibo de mensajes unicast
int presence_;
float soil_humi;
char *pregunta;

//Funciones de prueba **********************************BORRAR********************************************************
unsigned long timerDiscovery;
unsigned long timerRR;
unsigned long timerRB;

void verDatos(){
  Serial.printf("soil_humi: %f", soil_humi);Serial.println();
  Serial.printf("presence_: %i", presence_);Serial.println();
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
  int len = snprintf(msg, sizeof(msg), "3.envi_temp;envi_humi;illuminan;sprinklers,");
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
    //Serial.printf("pregunta:%s", pregunta); Serial.println();
    if (strcmp("envi_temp", pregunta)==0){
      generarRespuesta("envi_temp", envi_temp);
      //Serial.println("Respuesta enviada");
    }
    else if (strcmp("envi_humi", pregunta)==0){
      generarRespuesta("envi_humi", envi_humi);
      //Serial.println("Respuesta enviada");
    }
    else if (strcmp("illuminan", pregunta)==0){
      generarRespuesta("illuminan", illuminan);
      //Serial.println("Respuesta enviada");
    }
  }
  //memset(pregunta, 0, 15);
}

void addpeers(){
  esp_now_add_peer(broker, ESP_NOW_ROLE_COMBO, 3, NULL,0);
  Serial.println("broker added");
}

//Funciones para la generación de datos
double generarTemperatura(int hora) //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
{
  double media[24]={22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  return numero;
}

double generarHumedad() //Funcion que genera una humedad random siguiendo una distribucion gaussiana
{
  double media[24]={80.0, 81.0, 82.0, 81.0, 80.0, 79.0, 78.0, 76.0, 72.0, 68.0, 65.0, 60.0, 55.0, 50.0, 48.0, 45.0, 48.0, 50.0, 55.0, 60.0, 68.0, 72.0, 75.0, 80.0};
  Gaussian generador = Gaussian(media[hora], 1.0);  
  double numero = generador.random();
  return numero;
}

float generarIlluminan(int hora){
  double media[24]={310.0, 320.0, 340.0, 370.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 900.0, 950.0, 1000.0, 950.0, 900.0, 800.0, 700.0, 600.0, 500.0, 350.0, 330.0, 310.0, 305.0, 300.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  return numero;
}

void generarPregunta(){
  ////timerRR=millis();
  String json;
  StaticJsonDocument<48> doc;
  doc["pregunta"] = dato;
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
  bool tieneP = doc.containsKey("presence_");
  bool tieneSH = doc.containsKey("soil_humi");
  bool tieneS = doc.containsKey("sprinklers");
  
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
  
  else if (tieneP){
    presence_=doc["presence_"];
    return 0;
  }
  
  else if (tieneSH){
    soil_humi=doc["soil_humi"];
    ////unsigned long respuesta = millis()-timerRR;
    ////Serial.printf("R: %lu", respuesta);Serial.println();
    return 0;
  }
  
  else if (tieneS){
    sprinklersPrevio=sprinklers;
    sprinklers=doc["sprinklers"];
    if (sprinklers==1 and sprinklersPrevio==0 and presence_==0){
      Serial.println("Se encendieron los aspersores!!!");
    }
    //else {sprinklers=0;}
    return 0;
  }
}

void generarRespuesta(char *dato, float variable){
  String json;
  StaticJsonDocument<48> doc;
  doc[dato] = variable;
  serializeJson(doc, json);
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
  illuminan=generarIlluminan(hora);
  envi_humi=generarHumedad();
  envi_temp=generarTemperatura(hora);
}

void loop() {
 //Serial.println("aquiLOOP");
 tiempoActual=millis();
 if (digitalRead(BUTTON_PIN) == LOW and broker_anadido==0){
    modoDiscovery();
 }

 if (broker_anadido==1){
  if (tiempoActual-generarDatos>10000){ //Generar datos cada 10 segundos
    //Serial.println("aqui1");
    generarDatos=millis();
    illuminan=generarIlluminan(hora);
    envi_humi=generarHumedad();
    envi_temp=generarTemperatura(hora);
    evento1=0;
    evento2=0;
    evento3=0;
  }
  if (tiempoActual>(tpregunta+15000)){ //Preguntar dato random cada 15 segundos
    //Serial.println("aqui2");
    tpregunta=millis();
    generarPregunta();
    //verDatos();
  }
  if (tiempoActual>(tAvanzarHora+150000)){ //Avanzar hora 150 segundos (cada 2.5 min)
    //Serial.println("aqui3");
    tAvanzarHora=millis();
    hora++;
    if (hora>24){hora=0;}
  }
  if (envi_temp>=34 and evento1==0){
    //Serial.println("aqui4");
    evento1=1;
    ////timerRB=millis();
    generarRespuesta("envi_temp", envi_temp);
  }
  if (envi_humi>=60 and evento2==0){
    //Serial.println("aqui5");
    evento2=1;
    ////timerRB=millis();
    generarRespuesta("envi_humi", envi_humi);
  }
  if (illuminan<=500 and illuminan>0 and evento3==0){
    //Serial.println("aqui6");
    evento3=1;
    ////timerRB=millis();
    generarRespuesta("illuminan", illuminan);
  }
  if ((soil_humi>60 and sprinklers==1) or (presence_==1 and sprinklers==1)){
    //Serial.println("aqui7");
    sprinklers=0;
    Serial.println("Se apagaron los aspersores");
  }
 }
 else{
  if (tiempoActual-generarDatos>10000){
    generarDatos=millis();
    Serial.println("Broker no añadido");
  }
 }
}
