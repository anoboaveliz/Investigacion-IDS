#define generarDatos 1000 

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Gaussian.h>

//Variables de simulación
int hora;
float soil_temp;
float soil_humi;
float envi_temp;
float envi_humi;
float illuminan;
float co2_level;
int presence_;

//Temporizadores
unsigned long datosTimer; 

//Credenciales del AP
const char* ssid = "InvestigacionESP";
const char* password = "Itinerario2023";

//Direcciones IP
WiFiUDP Udp;
char *broadcast="10.42.0.255";
unsigned int localPort = 4321;  // local port to listen on

//Funciones UDP
double generarTemperaturaSuelo(){ //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
  double media[2]={22.0,38.0};
  int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}
}

double generarHumedadSuelo(){ //Funcion que genera una humedad random siguiendo una distribucion gaussiana
  double media[2]={45.0, 90.0};
  int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}
}

double generarTemperaturaAmbiente(){ //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
  double media[2]={22.0, 38.0};
  int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}

}

double generarHumedadAmbiente(){ //Funcion que genera una humedad random siguiendo una distribucion gaussiana
  double media[2]={45.0, 90.0};
  int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}
}

float generarIlluminan(){
  double media[24]={300.0, 1000.0};
  int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}
}

double generarCO2(){
  double media[24]={500.0, 1000.0};
 int numRandom1 = random(0,2);
  float numRandom2 = random(0,100);
  float numRandom3 = random(100,200);
  float respuesta1 = media[numRandom1]*(numRandom2/100);
  float respuesta2 = media[numRandom1]*(numRandom3/100);
  if (numRandom1==0){return respuesta1;}
  else if (numRandom1==1){return respuesta2;}
}

int generarPresencia(){
  int media[24]={50,50,50,50,50,53,65,70,75,80,65,60,55,50,50,50,50,60,70,60,50,50,50,50};
  int numero=random(0,media[hora])+1;
  if (numero>50){return 1;}
  else {return 0;}
}

void enviarDatos(){
  char output[200];
  StaticJsonDocument<48> doc;
  //doc["soil_temp"] = soil_temp;
  //doc["soil_humi"] = soil_humi;
  doc["envi_humi"] = envi_humi;
  doc["envi_temp"] = envi_temp;
  //doc["illuminan"] = illuminan;
  doc["co2_level"] = co2_level;
  //doc["presence_"] = presence_;
  serializeJson(doc, output);

  Udp.beginPacket(broadcast, localPort);
  Udp.write(output);
  Udp.endPacket();

  Serial.println(output);
  
  datosTimer=millis();
}

void setup(){
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Conectandose a %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  randomSeed(analogRead(A0));
  hora = random(0,24);
}

void loop(){
  if (millis()-datosTimer>=generarDatos){
    soil_temp=generarTemperaturaSuelo();
    soil_humi=generarHumedadSuelo();
    envi_temp=generarTemperaturaAmbiente();
    envi_humi=generarHumedadAmbiente();
    illuminan=generarIlluminan();
    co2_level=generarCO2();
    presence_=generarPresencia();
    enviarDatos();
  }
}
