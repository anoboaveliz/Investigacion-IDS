//Script para NodeMCU broker e IDS

//Librerias
#include <stdio.h>
#include <string.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WifiEspNowBroadcast.h>
#include <IDS_MITM_inferencing.h>

//Arreglos y contadores para guardar direcciones MAC e información
int stat; //Status de los nodos
int dispositivos=0; //Contador de dispostitivos
char datos[50][15]; //Arreglo que guarda los tipos de datos que sensa cada dispositivo
int datos_escritos=0; //Contador de cuantas filas se han escrito en el arreglo datos
char actuadores[50][15]; //Arreglo que guarda los tipos de datos que recibe cada dispositivo
int actuadores_escritos=0; //Contador de cuantas filas se han escrito en el arreglo actuadores
int canales_estado[20][2]; //Arreglo que guarda los canales de transmision ESP-NOW y el estado del nodo
const int tamano_datos=50; //Número constante de filas en el arreglo datos
uint8_t direcciones[20][6]; //Aregglo de direcciones MAC de dispostitivos
const int tamano_actuadores=50; //Número constante de filas en el arreglo datos
const unsigned long presionarBoton=25000; //Tiempo en milisegundos que se permanecerá en modo DISCOVERY
const unsigned long inicioDiscovery=40000; //Tiempo en milisegundos que se permanecerá en modo DISCOVERY


//Definir el pin del botón "flash" para activar el modo DISCOVERY 
static const int BUTTON_PIN = 0;
//Definir el pin del led azul de la tarjeta 
static const int LED_PIN = 2;
int ledState = HIGH;

//Variables de tiempo
int hora=13;
unsigned long tAvanzarHora;
unsigned long tiempoActual;
unsigned long verificarActivos;
unsigned long tiempoRandom;
unsigned long tActualizarDatos;
unsigned long tPreguntar=10000;

//Variables para trabajar con mensajes json
char *pregunta;
int presence_;
float soil_temp;
float soil_humi;
float envi_temp;
float envi_humi;
float co2_level;
float illuminan;
int primeraActualizacion=0;

//Variable para trabajar con IDS
float features[7];
float resultados[2]; //[anormal, normal]

//Funciones y variables de prueba **********************************BORRAR********************************************************
float rand_number; //Número random a enviar
float recv_number; //Número random que se recibe
uint8_t pruebaDir[] = {0xAC, 0x0B, 0xFB, 0xD6, 0x53, 0x06}; //3
unsigned long timerDiscovery;
unsigned long timersRR[6];

void leerdatos(){
  for (size_t x=0; x<datos_escritos; x++){
   for (size_t y=0; y<15; y++){
    Serial.print(datos[x][y]); 
   }
   Serial.println();
  }
 Serial.printf("Actuadores: %i", actuadores_escritos);Serial.println();
  for (size_t x=0; x<actuadores_escritos; x++){
   for (size_t y=0; y<15; y++){
    Serial.print(actuadores[x][y]); 
   }
   Serial.println();
  }
  Serial.println();
}

void imprimiractivos(){
  Serial.println("Dispositivos activos:");
  for (size_t x=0; x<dispositivos; x++){
    if(canales_estado[x][1]==1){
      Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", direcciones[x][0], direcciones[x][1], direcciones[x][2], direcciones[x][3], direcciones[x][4], direcciones[x][5]);
      Serial.println();
    }
  }
}

void verDatos(){
  Serial.printf("La hora es: %i:00", hora);Serial.println();
  Serial.printf("soil_temp: %f", soil_temp);Serial.println();
  Serial.printf("soil_humi: %f", soil_humi);Serial.println();
  Serial.printf("envi_temp: %f", envi_temp);Serial.println();
  Serial.printf("envi_humi: %f", envi_humi);Serial.println();
  Serial.printf("presence_: %i", presence_);Serial.println();
  Serial.printf("co2_level: %f", co2_level);Serial.println();
  Serial.printf("illuminan: %f", illuminan);Serial.println();
}
//Funciones de prueba **********************************BORRAR********************************************************

//Funciones de envío de mensajes de DISCOVERY con el uso de ESP-NOW
void modoDiscovery(unsigned long tiempo){
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
  while (currentTime + tiempo >= millis()) {
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
  if (dispositivos>0){addpeers();}
  Serial.println("Finalizando modo DISCOVERY");
  Serial.println();
}

void processRx(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Loop para ver si la dirección ya se encuentra registrada
  uint8_t* dispositivo = const_cast<uint8_t*>(mac); //Cast de la dirección MAC de const a no const
  dispositivo[0]=dispositivo[0]-2;
  int igualdad;
  for (size_t a=0; a<10; a++){
    igualdad=0;
    for(size_t b=0; b<6; b++){
      if (memcmp(&direcciones[a][b], &dispositivo[b], sizeof(uint8_t))==0){
        igualdad++;
      }
    }
    if (igualdad==6){
      return;
    }
  }
  //Procesar mensaje
  char* informacion = reinterpret_cast<char*>(const_cast<uint8_t*>(buf));
  if(strlen(informacion)==0 or informacion[1]!='.'){
    return;
  }
  Serial.printf("Mensaje de %02X:%02X:%02X:%02X:%02X:%02X", dispositivo[0], dispositivo[1], dispositivo[2], dispositivo[3], dispositivo[4], dispositivo[5]); //Imprimir dirección mac del dispositivo que envía el mensaje  //Procesar informacion del mensaje
  int canal = String(informacion[0]).toInt();
  canales_estado[dispositivos][0] = canal;
  canales_estado[dispositivos][1] = 1;

  //Añadir la información al arreglo correspondiente
  char palabra[15];
  memset(palabra, 0, 15);
  int size_palabra = 0;
  for (size_t j=0; j<6; j++){
    direcciones[dispositivos][j]=dispositivo[j];
  }
  for (size_t x=2; x<strlen(informacion);x++){
    if (informacion[x]!=';' and informacion[x]!=','){
      palabra[size_palabra]=informacion[x];
      size_palabra++;
    }
    else if (informacion[x]==';'){
      if (datos_escritos<tamano_datos){
        for (size_t q=0; q<strlen(palabra)+1; q++){
          if (q==0){
            char pos = dispositivos+'0';
            datos[datos_escritos][q]=pos;
          }
          else{
            datos[datos_escritos][q]=palabra[q-1]; 
          }
        }
        datos_escritos++;
      }
      else if(datos_escritos==tamano_datos){
        Serial.println("Cantidad de sensores maxima alcanzada");
      }
      memset(palabra, 0, 15);
      size_palabra=0;
    }
    else if (informacion[x]==','){
      if (actuadores_escritos<tamano_actuadores){
        for (size_t q=0; q<strlen(palabra)+1; q++){
          if (q==0){
            char pos = dispositivos+'0';
            actuadores[actuadores_escritos][q]=pos;
          }
          else{
            actuadores[actuadores_escritos][q]=palabra[q-1]; 
          }
        }
        actuadores_escritos++;
      }
      else if(actuadores_escritos==tamano_actuadores){
        Serial.println("Cantidad de actuadores maxima alcanzada");
      }
      memset(palabra, 0, 15);
      size_palabra=0;
    }
  }
  dispositivos++;
  ////timerDiscovery=millis();
  ////Serial.printf("%i: %lu", dispositivos, timerDiscovery);Serial.println();
  Serial.println(" => Dispositivo añadido");
  leerdatos();
}

void sendMessage(){
  char msg[60];
  int len = snprintf(msg, sizeof(msg), "broker;");
  WifiEspNowBroadcast.send(reinterpret_cast<const uint8_t*>(msg), len);
}

//Funciones para el envío y recepción de datos mediante el protocolo ESP-NOW
void addpeers(){
  for (size_t j=0; j<dispositivos; j++){
    esp_now_add_peer(direcciones[j], ESP_NOW_ROLE_COMBO, canales_estado[j][0], NULL,0);
  }
  Serial.println("Peers added");
}

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  char *buff = (char*) incomingData;
  //Serial.print("buffer: ");Serial.println(buff);
  int resp = deserializarJson(buff, mac);
  if (resp==1 and primeraActualizacion==1){
    //Serial.println("Aqui10");
    char pedido[15];
    //Serial.println("Aqui11");
    memset(pedido, 0, 15);
    for (size_t x=0; x<datos_escritos; x++){
      for (size_t y=1; y<15; y++){
        //Serial.println("Aqui12");
        pedido[y-1]=datos[x][y];
      }
      if (strcmp(pedido, pregunta)==0){
        //Serial.println("Aqui13");
        generarRespuesta(pregunta, mac);
        return;
      }
      memset(pedido, 0, 15);
    }
    generarError(mac);
  }
  else if(primeraActualizacion==0){
    generarError(mac);
  }
  //memset(pregunta, 0, 15);
}

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

void verificarestado(){
  char *estado="status";
  for (size_t i=0; i<dispositivos; i++){
    esp_now_send(direcciones[i], (uint8_t *)estado, sizeof(char)*strlen(estado));
    delay(10);
    if (stat==1){
      canales_estado[i][1]=1;
    }
    else{
      canales_estado[i][1]=0;
    }
  }
  Serial.println("Se verifico estado");
}

//Funciones para trabajar con formato json
int deserializarJson(char* input, uint8_t *mac){ //TODO BIEN:0 ERROR=-1 PREGUNTA=1
  //Serial.println("Aqui1");
  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return -1;
  }
  bool tienePregunta = doc.containsKey("pregunta");
  bool tieneST = doc.containsKey("soil_temp");
  bool tieneSH = doc.containsKey("soil_humi");
  bool tieneET = doc.containsKey("envi_temp");
  bool tieneEH = doc.containsKey("envi_humi");
  bool tieneP = doc.containsKey("presence_");
  bool tieneCO2 = doc.containsKey("co2_level");
  bool tieneI = doc.containsKey("illuminan");
  //Serial.println("Aqui2");
  if (tienePregunta){
    //Serial.println("Aqui3");
    const char *cast = doc["pregunta"];
    pregunta=(char*)cast;
    //Serial.printf("Se ha recibido una pregunta:%s", pregunta);Serial.println();
    return 1;  
  }

  else if (tieneST){
    //Serial.println("Aqui4");
    soil_temp=doc["soil_temp"];
    //Serial.println("Aqui5");
    ////unsigned long respuesta = millis()-timersRR[0];
    ////Serial.printf("ST: %lu", respuesta);Serial.println();
    return 0;  
  }
  
  else if (tieneSH){
    //Serial.println("Aqui5");
    soil_humi=doc["soil_humi"];
    ////unsigned long respuesta = millis()-timersRR[1];
    ////Serial.printf("SH: %lu", respuesta);Serial.println();
    if (soil_humi<=60){
      Serial.printf("Se genero un evento del nivel de humedad del suelo!: %f", soil_humi);Serial.println();
      buscarConsumidor("soil_humi");
      ////generarError(mac);
    }
    return 0;  
  }
  
  else if (tieneET){
    //Serial.println("Aqui6");
    envi_temp=doc["envi_temp"];
    ////unsigned long respuesta = millis()-timersRR[2];
    ////Serial.printf("ET: %lu", respuesta);Serial.println();
    if (envi_temp>=34){
      Serial.printf("Se genero un evento del nivel de temperatura del ambiente!: %f", envi_temp);Serial.println();
      buscarConsumidor("envi_temp");
      ////generarError(mac);
      
    }
    return 0;  
  }
  
  else if (tieneEH){
    //Serial.println("Aqui7");
    envi_humi=doc["envi_humi"];
    ////unsigned long respuesta = millis()-timersRR[3];
    ////Serial.printf("EH: %lu", respuesta);Serial.println();
    if (envi_humi>=60){
      Serial.printf("Se genero un evento del nivel de humedad del ambiente!: %f", envi_humi);Serial.println();
      buscarConsumidor("envi_humi");
      ////generarError(mac);
    }
    return 0;  
  }
  
  else if (tieneP){
    //Serial.println("Aqui8");
    presence_=doc["presence_"];
    if (presence_==1){
      Serial.printf("Se genero un evento de presencia!: %i", presence_);Serial.println();
      buscarConsumidor("presence_");
      ////generarError(mac);
    }
    return 0;  
  }
  
  else if (tieneCO2){
    //Serial.println("Aqui9");
    co2_level=doc["co2_level"];
    ////unsigned long respuesta = millis()-timersRR[5];
    ////Serial.printf("CO: %lu", respuesta);Serial.println();
    if (co2_level<=300 or co2_level>=1000){
      Serial.printf("Se genero un evento del nivel de CO2!: %f", co2_level);Serial.println();
      buscarConsumidor("co2_level");
      ////generarError(mac);
    }
    return 0;  
  }

  else if (tieneI){
    //Serial.println("Aqui10");
    illuminan=doc["illuminan"];
    ////unsigned long respuesta = millis()-timersRR[4];
    ////Serial.printf("I: %lu", respuesta);Serial.println();
    if (illuminan<=500){
      Serial.printf("Se genero un evento de iluminancia!: %f", illuminan);Serial.println();
      buscarConsumidor("illuminan");
      ////generarError(mac);
    }
    //Serial.println("Aqui11");
    return 0;  
  }
  //Serial.println("Aqui11");
}

void generarRespuesta(char *pregunta, uint8_t *emisor){
  if (strcmp("soil_temp", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["soil_temp"] = soil_temp;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("soil_humi", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["soil_humi"] = soil_humi;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("envi_temp", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["envi_temp"] = envi_temp;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("envi_humi", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["envi_humi"] = envi_humi;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("presence_", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["presence_"] = presence_;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("co2_level", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["co2_level"] = co2_level;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
  else if (strcmp("illuminan", pregunta)==0){
    //Serial.println("Se envio un dato");
    String json;
    StaticJsonDocument<48> doc;
    doc["illuminan"] = illuminan;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    esp_now_send(emisor, buffer, sizeBuff);
  }
}

void generarPreguntas(){
  char peticion[15];
  for (size_t x=0; x<datos_escritos; x++){
    for (size_t y=1; y<15; y++){
      peticion[y-1]=datos[x][y];
    }
    if (strcmp("presence_", peticion)==0){continue;}
    timersRR[x]=millis();
    String json;
    StaticJsonDocument<48> doc;
    doc["pregunta"] = peticion;
    serializeJson(doc, json);
    uint8_t *buffer = (uint8_t*) json.c_str();
    size_t sizeBuff = sizeof(buffer) * json.length();
    delay(10);
    esp_now_send(direcciones[(int)datos[x][0]-48], buffer, sizeBuff);
  }
  Serial.println("Se actualizaron los datos");
}

void generarError(uint8_t *emisor){
  String json;
  StaticJsonDocument<48> doc;
  doc["error"] = "";
  serializeJson(doc, json);
  uint8_t *buffer = (uint8_t*) json.c_str();
  size_t sizeBuff = sizeof(buffer) * json.length();
  esp_now_send(emisor, buffer, sizeBuff);
}

void buscarConsumidor(char *dato){  
  char actuador[15];
  if (strcmp("soil_humi", dato)==0){
   for (size_t x=0; x<actuadores_escritos; x++){
    for (size_t y=1; y<15; y++){
      actuador[y-1]=actuadores[x][y];
    }
    if (strcmp("sprinklers", actuador)==0){
      generarRespuesta("soil_humi", direcciones[(int)actuadores[x][0]-48]);
      generarConsumidor(actuador, 1, direcciones[(int)actuadores[x][0]-48]);
      return;
    }
   }
   Serial.println("No se encontro un consumidor 1");
  }
  
  else if (strcmp("envi_temp", dato)==0){
    for (size_t x=0; x<actuadores_escritos; x++){
      for (size_t y=1; y<15; y++){
        actuador[y-1]=actuadores[x][y];
      }
      if (strcmp("awning", actuador)==0){
        generarRespuesta("envi_temp", direcciones[(int)actuadores[x][0]-48]);
        generarConsumidor(actuador, 1, direcciones[(int)actuadores[x][0]-48]);
        return;
      }
   }
   Serial.println("No se encontro un consumidor 2");
  }
  
  else if (strcmp("envi_humi", dato)==0){
    for (size_t x=0; x<actuadores_escritos; x++){
      for (size_t y=1; y<15; y++){
        actuador[y-1]=actuadores[x][y];
      }
      if (strcmp("windows", actuador)==0){
        generarRespuesta("envi_humi", direcciones[(int)actuadores[x][0]-48]);
        generarConsumidor(actuador, 1, direcciones[(int)actuadores[x][0]-48]);
        return;
      }
   }
   Serial.println("No se encontro un consumidor 3");
  }
  
  else if (strcmp("presence_", dato)==0){
    for (size_t x=0; x<actuadores_escritos; x++){
      for (size_t y=1; y<15; y++){
        actuador[y-1]=actuadores[x][y];
      }
      if (strcmp("sprinklers", actuador)==0){
        generarRespuesta("presence_", direcciones[(int)actuadores[x][0]-48]);
        generarConsumidor(actuador, 0, direcciones[(int)actuadores[x][0]-48]);
        return;
      }
   }
   Serial.println("No se encontro un consumidor 4");
  }
  
  else if (strcmp("co2_level", dato)==0){
    for (size_t x=0; x<actuadores_escritos; x++){
      for (size_t y=1; y<15; y++){
        actuador[y-1]=actuadores[x][y];
      }
      if (strcmp("ventilation", actuador)==0){
        generarRespuesta("co2_level", direcciones[(int)actuadores[x][0]-48]);
        generarConsumidor(actuador, 1, direcciones[(int)actuadores[x][0]-48]);
        return;
      }
   }
   Serial.println("No se encontro un consumidor 5");
  }
  
  else if (strcmp("illuminan", dato)==0){
   for (size_t x=0; x<actuadores_escritos; x++){
      for (size_t y=1; y<15; y++){
        actuador[y-1]=actuadores[x][y];
      }
      if (strcmp("lights", actuador)==0){
        generarRespuesta("illuminan", direcciones[(int)actuadores[x][0]-48]);
        generarConsumidor(actuador, 1, direcciones[(int)actuadores[x][0]-48]);
        return;
      }
   }
   Serial.println("No se encontro un consumidor 6");
  }
}

void generarConsumidor(char *actuador, int estado, uint8_t *mac){
  String json;
  StaticJsonDocument<48> doc;
  doc[actuador] = estado;
  serializeJson(doc, json);
  uint8_t *buffer = (uint8_t*) json.c_str();
  size_t sizeBuff = sizeof(buffer) * json.length();
  esp_now_send(mac, buffer, sizeBuff);
}

//Funciones del IDS
void actualizarArreglo(){
  features[0]=soil_temp;
  features[1]=soil_humi;
  features[2]=envi_temp;
  features[3]=envi_humi;
  features[4]=illuminan;
  features[5]=co2_level;
  features[6]=presence_;
}

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void inferir(){
  //Empezar a inferir
  //ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

  if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
      //ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
      delay(1000);
      return;
  }

  ei_impulse_result_t result = { 0 };

  // the features are stored into flash, and we don't want to load everything into RAM
  signal_t features_signal;
  features_signal.total_length = sizeof(features) / sizeof(features[0]);
  features_signal.get_data = &raw_feature_get_data;

  // invoke the impulse
  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
  //ei_printf("run_classifier returned: %d\n", res);

  if (res != 0) return;

  // print the predictions
  //ei_printf("Predictions ");
  //ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)", result.timing.dsp, result.timing.classification, result.timing.anomaly);
  //ei_printf(": \n");
  //ei_printf("[");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      //ei_printf("%.5f", result.classification[ix].value);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        //ei_printf(", ");
#else
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
            //ei_printf(", ");
        }
#endif
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    //ei_printf("%.3f", result.anomaly);
#endif
    //ei_printf("]\n");

    // human-readable predictions
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        resultados[ix]=result.classification[ix].value;
        //ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
    if (resultados[0]>0.75){
      Serial.println("Se detecto una anomalia!!!");
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    //ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}

//Setup y loop principal
void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
  
  WifiEspNowBroadcast.onReceive(processRx, nullptr);

  modoDiscovery(inicioDiscovery);
  
  verificarActivos=millis();
  tActualizarDatos=millis();
  tiempoRandom=random(3,7)*10000;

  //generarPreguntas();
}

void loop() {
  tiempoActual=millis();
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    modoDiscovery(presionarBoton);
  }

  if (tiempoActual>(tAvanzarHora+150000)){ //Avanzar hora cada 150 segundos (cada 2.5 min)
    tAvanzarHora=millis();
    hora++;
    if (hora>24){hora=0;}
  }

  if(tiempoActual>(verificarActivos+tiempoRandom)){ //Verificar nodos activos
    tiempoRandom=random(3,7)*100000;
    verificarActivos=millis();
    verificarestado();
  }

  if(tiempoActual-tActualizarDatos>tPreguntar){ //Actualizar datos aprox. cada 75 seg (1.25 min)
    primeraActualizacion=1;
    tPreguntar=75000UL;
    tActualizarDatos=millis();
    generarPreguntas();
    actualizarArreglo();
    Serial.printf("features: [%f,%f,%f,%f,%f,%f,%f]", features[0], features[1], features[2], features[3], features[4], features[5], features[6]);
    inferir();
    verDatos();
  }
}
