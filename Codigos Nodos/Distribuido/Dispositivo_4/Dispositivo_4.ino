#define avanzarHora 150000 //2.5 min -> 1 hora (60 min -> 1 día)
#define generarDatos 12500 //12.5 seg -> 5 min

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Gaussian.h>

//Variables de simulación
int hora=0;
float co2_level;
int presence_;
int awning=0;
int lights=0;

//Variables recibidas
float illuminan;
float envi_temp;

//Temporizadores
unsigned long horaTimer;
unsigned long datosTimer; 

//Credenciales del AP
const char* ssid = "InvestigacionESP";
const char* password = "Itinerario2023";

//Direcciones IP
char *broadcast="10.42.0.255";

WiFiUDP Udp;
unsigned int localPort = 4321;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets

//Funciones UDP
void escucharUDP(){
  delay(1);
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    //receive incoming UDP packets
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    //Serial.printf("UDP packet contents: %s\n", incomingPacket);
    deserializarJson(incomingPacket);
  }
}

int generarPresencia(){
  int media[24]={50,50,50,50,50,53,65,70,75,80,65,60,55,50,50,50,50,60,70,60,50,50,50,50};
  int numero=random(0,media[hora])+1;
  if (numero>50){return 1;}
  else {return 0;}
}

double generarCO2(){
  double media[24]={1000.0, 980.0, 930.0, 900.0, 800.0, 700.0, 700.0, 700.0, 800.0, 700.0, 650.0, 650.0, 600.0, 550.0, 500.0, 550.0, 600.0, 650.0, 750.0, 800.0, 800.0, 850.0, 900.0, 950.0};
  Gaussian generador = Gaussian(media[hora], 1.2); 
  double numero = generador.random();
  return numero;
}

void deserializarJson(char* input){
  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  bool tieneI = doc.containsKey("illuminan");
  bool tieneET = doc.containsKey("envi_temp");
  bool tieneHora = doc.containsKey("hora");
  
  if (tieneET){
    envi_temp=doc["envi_temp"];
    if (awning==0 and (envi_temp>=34)){
      awning=1;
      Serial.println("Se bajo el toldo!");
    }
    else if (awning==1 and (envi_temp>0 and envi_temp<=32)){
      awning=0;
      Serial.println("Se subio el toldo!");
    }
  }

  else if (tieneI){
    illuminan=doc["illuminan"];
    if (illuminan==0 and (co2_level<=500)){
      illuminan=1;
      Serial.println("Se encendio el sistema de iluminacion!");
    }
    else if (illuminan==1 and (co2_level>=550)){
      illuminan=0;
      Serial.println("Se apago el sistema de iluminacion!");
    }
  }

  else if (tieneHora){
    hora=doc["hora"];
  }
}

void enviarDatos(){
  char output[100];
  StaticJsonDocument<48> doc;
  doc["co2_level"] = co2_level;
  doc["presence_"] = presence_;
  serializeJson(doc, output);

  Udp.beginPacket(broadcast, localPort);
  Udp.write(output);
  Udp.endPacket();
  
  datosTimer=millis();
}

void enviarHora(){
  char output[15];
  StaticJsonDocument<16> doc;
  doc["hora"] = hora;
  serializeJson(doc, output);
  
  Udp.beginPacket(broadcast, localPort);
  Udp.write(output);
  Udp.endPacket();
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

  Udp.begin(localPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localPort);

  horaTimer=millis();
}

void loop(){
  escucharUDP();
  
  if (millis()-horaTimer>=avanzarHora){ //Avanzar hora
    hora++;
    if (hora>=24){
      hora=0;
    }
    enviarHora();
    horaTimer=millis();
  }
  
  if (millis()-datosTimer>=generarDatos){
    co2_level=generarCO2();
    presence_=generarPresencia();
    enviarDatos();
  }
}
