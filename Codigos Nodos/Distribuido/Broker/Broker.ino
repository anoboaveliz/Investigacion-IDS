#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <IDS_MITM_inferencing.h>

//Variables de prueba *********************************BORRAR***************************************
int v1=0;
int v2=0;
int v3=0;
int v4=0;
int v5=0;
int v6=0;
int v7=0;
//Variables de prueba *********************************BORRAR***************************************

//Variables de la simulacion
int hora=0;
int presence_;
float soil_temp;
float soil_humi;
float envi_temp;
float envi_humi;
float co2_level;
float illuminan;
unsigned long datosTimer;

//Variables para trabajar con IDS
float features[7];
float resultados[2]; //[anormal, normal]

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

void deserializarJson(char* input){
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  bool tieneHora= doc.containsKey("hora");
  bool tieneEH = doc.containsKey("envi_humi");
  bool tieneCO2 = doc.containsKey("co2_level");
  bool tieneP = doc.containsKey("presence_");
  bool tieneSH = doc.containsKey("soil_humi");
  bool tieneI = doc.containsKey("illuminan");
  bool tieneET = doc.containsKey("envi_temp");
  bool tieneST = doc.containsKey("soil_temp");

  if (tieneEH){
    envi_humi=doc["envi_humi"];
    features[3]=envi_humi;
    v1=1;
  }

  if (tieneCO2){
    co2_level=doc["co2_level"];
    features[5]=co2_level;
    v2=1;
  }

  if (tieneSH){
    soil_humi=doc["soil_humi"];
    features[1]=soil_humi;
    v3=1;
  }

  if (tieneP){
    presence_=doc["presence_"];
    features[6]=presence_;
    v4=1;
  }

  if (tieneET){
    envi_temp=doc["envi_temp"];
    features[2]=envi_temp;
    v5=1;
  }

  if (tieneI){
    illuminan=doc["illuminan"];
    features[4]=illuminan;
    v6=1;
  }
  
  if (tieneST){
    soil_temp=doc["soil_temp"];
    features[0]=soil_temp;
    v7=1;
  }

  if (tieneHora){
    hora=doc["hora"];
  }
}

//Funciones del IDS
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

//Funciones de prueba **********************************BORRAR********************************************************
void verDatos(){
  Serial.println();
  Serial.printf("La hora es: %i:00", hora);Serial.println();
  Serial.printf("soil_temp: %f", soil_temp);Serial.println();
  Serial.printf("soil_humi: %f", soil_humi);Serial.println();
  Serial.printf("envi_temp: %f", envi_temp);Serial.println();
  Serial.printf("envi_humi: %f", envi_humi);Serial.println();
  Serial.printf("presence_: %i", presence_);Serial.println();
  Serial.printf("co2_level: %f", co2_level);Serial.println();
  Serial.printf("illuminan: %f", illuminan);Serial.println();
  datosTimer=millis();
}
//Funciones de prueba **********************************BORRAR********************************************************

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
  datosTimer=millis();
}

void loop(){
  escucharUDP();

  if (millis()-datosTimer>=25000){
    verDatos();
    if (v1+v2+v3+v4+v5+v6+v7==7){
      inferir();
    }
  }
}
