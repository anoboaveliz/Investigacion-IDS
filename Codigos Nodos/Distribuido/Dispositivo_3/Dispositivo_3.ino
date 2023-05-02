#define avanzarHora 150000 //2.5 min -> 1 hora (60 min -> 1 día)
#define generarDatos 12500 //12.5 seg -> 5 min

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Gaussian.h>

//Variables de simulación
int hora=0;
float envi_temp;
float envi_humi;
float illuminan;
int sprinklers=0;

//Variables recibidas
float soil_humi;
int presence_;

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

double generarTemperaturaAmbiente(){ //Funcion que genera una temperatura random siguiendo una distribucion gaussiana
  double media[24]={22.0,23.0,23.0,23.0,24.0,25.0,26.0,28.0,30.0,33.0,35.0,37.0,38.0,37.0,36.0,35.0,33.0,31.0,29.0,28.0,26.0,25.0,23.0,22.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
  double numero = generador.random();
  return numero;

}

double generarHumedadAmbiente(){ //Funcion que genera una humedad random siguiendo una distribucion gaussiana
  double media[24]={90.0, 88.0, 82.0, 78.0, 75.0, 73.0, 70.0, 68.0, 66.0, 65.0, 65.0, 60.0, 55.0, 50.0, 45.0, 80.0, 75.0, 70.0, 65.0, 60.0, 55.0, 50.0, 45.0, 90.0};
  Gaussian generador = Gaussian(media[hora], 1.0);
  double numero = generador.random();
  return numero;
}

float generarIlluminan(){
  double media[24]={310.0, 320.0, 340.0, 370.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 900.0, 950.0, 1000.0, 950.0, 900.0, 800.0, 700.0, 600.0, 500.0, 350.0, 330.0, 310.0, 305.0, 300.0};
  Gaussian generador = Gaussian(media[hora], 1.0); 
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
  
  bool tieneP = doc.containsKey("presence_");
  bool tieneSH = doc.containsKey("soil_humi");
  bool tieneHora = doc.containsKey("hora");
  
  if (tieneSH){
    soil_humi=doc["soil_humi"];
    if (sprinklers==0 and soil_humi<=60 and presence_==0){
      sprinklers=1;
      Serial.println("Se encendieron los aspersores!");
    }
    else if (sprinklers==1 and soil_humi>=65){
      sprinklers=0;
      Serial.println("Se apagaron los aspersores!");
    }
  }

  else if (tieneP){
    presence_=doc["presence_"];
    if (presence_==1 and sprinklers==1){
      sprinklers=0;
      Serial.println("Se apagaron los aspersores!");
    }
  }

  else if (tieneHora){
    hora=doc["hora"];
  }
}

void enviarDatos(){
  char output[100];
  StaticJsonDocument<48> doc;
  doc["envi_humi"] = envi_humi;
  doc["envi_temp"] = envi_temp;
  doc["illuminan"] = illuminan;
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
    envi_temp=generarTemperaturaAmbiente();
    envi_humi=generarHumedadAmbiente();
    illuminan=generarIlluminan();
    enviarDatos();
  }
}
