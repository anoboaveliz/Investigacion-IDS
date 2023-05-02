//Intruso en la red

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

//Definir variables de recibo de mensajes unicast
char *pregunta;

//Definir variables de envío de mensajes unicast "5.soil_temp;illuminan;presence;"
int stat;
int hora=13;
int evento1=0;
char *envJson;
float soil_temp;
float illuminan;
int presence_;
unsigned long tpregunta;

//Funciones de prueba **********************************BORRAR********************************************************
unsigned long timerDiscovery;
unsigned long timerRR;
unsigned long timerRB;

void verDatos(){
  //Serial.printf("envi_humi: %f", envi_humi);Serial.println();
  //Serial.printf("co2_level: %f", co2_level);Serial.println();
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
  int len = snprintf(msg, sizeof(msg), "5.soil_temp;illuminan;presence_;");
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
    if (strcmp("soil_temp", pregunta)==0){
      generarRespuesta("soil_temp", soil_temp);
      //Serial.println("Respuesta enviada");
    }
    else if (strcmp("illuminan", pregunta)==0){
      generarRespuesta("illuminan", illuminan);
      //Serial.println("Respuesta enviada");
    }
    else if (strcmp("presence_", pregunta)==0){
      generarRespuesta("presence_", presence_);
      //Serial.println("Respuesta enviada");
    }
  }
  //memset(pregunta, 0, 15);
}

void addpeers(){
  esp_now_add_peer(broker, ESP_NOW_ROLE_COMBO, 2, NULL,0);
  Serial.println("broker added");
}

//Funciones para la generación de datos
double generarTemperatura(int hora) //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
{
  double media[24]={22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  if (hora>=20 or hora<6){return numero*0.001;}
  if (hora>7 or hora<=19){return numero*10;}
}

float generarIlluminan(int hora){
  double media[24]={310.0, 320.0, 340.0, 370.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 900.0, 950.0, 1000.0, 950.0, 900.0, 800.0, 700.0, 600.0, 500.0, 350.0, 330.0, 310.0, 305.0, 300.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  return numero*10;
}

int generarPresencia(int hora){
  return 1;
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
  soil_temp=generarTemperatura(hora);
  illuminan=generarIlluminan(hora);
}

void loop() {
 tiempoActual=millis();
 if (digitalRead(BUTTON_PIN) == LOW and broker_anadido==0) {
    modoDiscovery();
 }

 if (broker_anadido==1){
  if (tiempoActual-generarDatos>10000){ //Generar datos cada 10 segundos
    generarDatos=millis();
    soil_temp=generarTemperatura(hora);
    illuminan=generarIlluminan(hora);
    evento1=0;
  }
  if (tiempoActual>(tAvanzarHora+150000)){ //Avanzar hora 150 segundos (cada 2.5 min)
    tAvanzarHora=millis();
    hora++;
    if (hora>24){hora=0;}
    presence_=generarPresencia(hora);
  }
 }
 else{
  if (tiempoActual-generarDatos>10000){
    generarDatos=millis();
    Serial.println("Broker no añadido");
  }
 }
}
