#include <IOXhop_FirebaseESP32.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

#include <Wire.h>
#include <Adafruit_BMP085.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_AUTH "https://incubadora-c0cb0-default-rtdb.firebaseio.com/"
#define FIREBASE_HOST "AIzaSyAm37yL4BYxQsXboOz72jcKYJdDo3iOxog"

const char *ssid = "ssid";
const char *password = "password";

const char* timezone = "CST6CDT,M3.2.0/2,M11.1.0/2"; 

WebServer server(80);
DHT dht(5, DHT11);

Adafruit_BMP085 bmp;

void handleRoot() {
  char msg[1500];

  snprintf(msg, 1500,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='4'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>Incubadora</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    .container { display: flex; flex-direction: column; align-items: center }\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .datos { border: solid 1px #3e3e3e; border-radius: 5px; padding: 10px; background-color: #e6e6e6 }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
   <div class='container'>\
      <h2>Informacion del contenedor</h2>\
      <div class='datos'>\
      <p>\
        <span class='dht-labels'>Temperatura de la incubadora</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <span class='dht-labels'>Humedad dentro</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <p>\
        <span class='dht-labels'>mm. de mercurio</span>\
        <span>%.6f mmHg</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      </div>\
      </div>\
  </body>\
</html>",
           readDHTTemperature(), readDHTHumidity(), readBMPPressureHg()
          );
  server.send(200, "text/html", msg);
}

void setup(void) {

  Serial.begin(115200);
  dht.begin();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_AUTH, FIREBASE_HOST);

  configTime(-6 * 3600, 0, "pool.ntp.org");

  dht.begin();
  if (!bmp.begin()) {
    Serial.println("BMP180 no detected");
    while (1) {}    
  }

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {

  time_t now = time(nullptr);

  String estado = "", mensaje = ""; 


  char dateTimeString[20];
  strftime(dateTimeString, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

  Serial.print("Humedad:");
    Serial.print(readDHTHumidity());
    Serial.print(" %\t  -- ");

    Serial.print("Temperatura:");
    Serial.print(readDHTTemperature());
    Serial.println(" *C");

  if(readDHTTemperature() >= 26){ 
    mensaje = "¡Alert! temperatura de la incubadora muy alta";
    estado = "Erroneo";

  }
  else if(readDHTHumidity() <= 25){
    mensaje = "¡Alert! la humedad de la incubadora es baja";
    estado = "Erroneo";
  }
  else{
   mensaje = "El ambiente es adecuado";
   estado = "Correcto";
  }

  String jsonData = "{ \"Data\": { \"Fecha\": \"" + String(dateTimeString) + "\", \"Temperatura\": " + String(readDHTTemperature()) + "\", \"Humedad\": " + String(readDHTHumidity()) + "\", \"Presión atmosferica\": " + String(readBMPPressure()) + "\", \"Estado\": " + estadoDHT() + "\", \"Mensaje\": " + mensajeDHT() + " } }";

  Firebase.pushString("Data:", jsonData);

   if(Firebase.failed()){
    Serial.print("Configuracion erronea");
    Serial.println(Firebase.error());
    return;        
  }


  Serial.println("Datos de temperatura y humedad enviados");

  server.handleClient();
  delay(1000 * 3);
}


float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    return h;
  }
}

float readBMPPressure() {

  float p = bmp.readPressure() * 0.000009869;
  if (isnan(p)) {
    Serial.println("Failed to read Pressure");
    return -1;
  }
  else{
    return p;
  }
}

float readBMPPressureHg() {

  float p = bmp.readPressure() * 0.0075;
  if (isnan(p)) {
    Serial.println("Failed to read Pressure");
    return -1;
  }
  else{
    return p;
  }
}

String estadoDHT(){
  if(readDHTTemperature() >= 326){ 
    return "Erroneo";
  }
  else if(readDHTHumidity() <= 30){
    return "Erroneo";
  }
  else{
   return "Correcto";
  }

}


String mensajeDHT(){
  if(readDHTTemperature() >= 26){ 
    return "¡Alert! la temperatura interna esta demasiado alta";
  }
  else if(readDHTHumidity() <= 19){
    return "¡Alert! la humedad dentro es alta";
  }
  else{
   return "Temperatura y humedad de la incubadora son estables";
  }

}