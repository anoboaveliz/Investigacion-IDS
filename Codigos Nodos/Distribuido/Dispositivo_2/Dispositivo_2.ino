#define avanzarHora 150000 //2.5 min -> 1 hora (60 min -> 1 día)
#define generarDatos 12500 //12.5 seg -> 5 min

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Gaussian.h>

//Variables de simulación
int hora=0;
float soil_temp;
float soil_humi;
int ventilation=0;
int windows=0;

//Variables recibidas
float envi_humi;
float co2_level;

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

double generarTemperaturaSuelo(){ //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
  double media[24]={22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  if (hora>=20 or hora<6){return numero*0.9;}
  if (hora>7 or hora<=19){return numero*1.1;}
}

double generarHumedadSuelo(){ //Funcion que genera una humedad random siguiendo una distribucion gaussiana
  double media[24]={90.0, 88.0, 82.0, 78.0, 75.0, 73.0, 70.0, 68.0, 66.0, 65.0, 65.0, 60.0, 55.0, 50.0, 45.0, 80.0, 75.0, 70.0, 65.0, 60.0, 55.0, 50.0, 45.0, 90.0};
  Gaussian generador = Gaussian(media[hora], 1.0);
  double numero = generador.random();
  return numero*1.1;
}

void deserializarJson(char* input){
  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  bool tieneEH = doc.containsKey("envi_humi");
  bool tieneCO2 = doc.containsKey("co2_level");
  bool tieneHora = doc.containsKey("hora");
  
  if (tieneEH){
    envi_humi=doc["envi_humi"];
    if (windows==0 and (envi_humi>=60)){
      windows=1;
      Serial.println("Se abrieron las ventanas!");
    }
    else if (windows==1 and (envi_humi>0 and envi_humi<=55)){
      windows=0;
      Serial.println("Se cerraron las ventanas!");
    }
  }

  else if (tieneCO2){
    co2_level=doc["co2_level"];
    if (ventilation==0 and (co2_level<=300 or co2_level>=1000)){
      ventilation=1;
      Serial.println("Se encendio el sistema de vetilacion!");
    }
    else if (ventilation==1 and (co2_level>=350 and co2_level<=950)){
      ventilation=0;
      Serial.println("Se apago el sistema de vetilacion!");
    }
  }

  else if (tieneHora){
    hora=doc["hora"];
  }
}

void enviarDatos(){
  char output[100];
  StaticJsonDocument<48> doc;
  doc["soil_temp"] = soil_temp;
  doc["soil_humi"] = soil_humi;
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
    soil_temp=generarTemperaturaSuelo();
    soil_humi=generarHumedadSuelo();
    enviarDatos();
  }
}
