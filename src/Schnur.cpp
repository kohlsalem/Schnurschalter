

#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <WiFiUdp.h>
#include <ArduinoOTA.h>    //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

WiFiUDP listener;
ESP8266WebServer server(80);

// adapt this to your needs //////////////////////////

#define RELAIS    D1  // the relais connection pin
#define CONTACT   D3  // the cordswitch pin

const char* openhab_host = "192.168.178.38"; // openhab server ip
String openhab_item = "LampeSchreibtisch"; // item name to update

/////////////////////////////////////////////////////

int currentState = LOW;
int currentSwitch;

void updateState(int newState);

void setOn(){
  digitalWrite(RELAIS,HIGH);
  updateState( HIGH );
}
void setOff(){
  digitalWrite(RELAIS,LOW);
  updateState( LOW );
}

void toggleState(){
  if(currentState==LOW){
    setOn();
  }else{
    setOff();
  }
}

// send new state to OH server
void updateState(int newState){
  currentState = newState;
  String s = "";
  String value = "";
  String vlength = "";

  switch(currentState){
    case LOW:          value = "OFF";    vlength = "3"; break;
    case HIGH:         value = "ON";     vlength = "2"; break;
  }

  WiFiClient client; // Webclient initialisieren
  if (!client.connect(openhab_host, 8080)) { // connect with openhab 8080
    Serial.println("Fehler: Verbindung zur OH konnte nicht aufgebaut werden");
     delay(100);
    return;
  }
  Serial.println("Sending"+value);
  client.println("PUT /rest/items/"+openhab_item+"/state HTTP/1.1");
  client.print("Host: ");
  client.println(openhab_host);
  client.println("Connection: close");
  client.println("Content-Type: text/plain");
  client.println("Content-Length: "+vlength+"\r\n");
  client.print(value);
}



void loop() {
  int newSwitch;

  // read current state of cord switch
  newSwitch = digitalRead(CONTACT);

  if(newSwitch != currentSwitch){
     toggleState();
  }

  // remember for next loop
  currentSwitch = newSwitch;

  // handle OTA update and webserver requests
  ArduinoOTA.handle();
  server.handleClient();
  delay(100);
}


void setupWebServer(){
 // listen to brightnes changes
 server.on("/", []()
 {
   String s = "{\n   \"state\":";
   s += currentState;
   s += "\n}";

   server.send(200, "text/plain", s);
 });
 server.on("/on", []()
 {
   setOn();
   String s = "{\non\n }";
   server.send(200, "text/plain", s);
 });
 server.on("/off", []()
 {
   setOff();
   String s = "{\noff\n }";
   server.send(200, "text/plain", s);
 });
 // start the webserver
 server.begin();
}

// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(9600);
  pinMode(RELAIS, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CONTACT, INPUT_PULLUP ); // pullup, since i want to pull it to GND
  currentSwitch = digitalRead(CONTACT);

  WiFiManager wifiManager;

  digitalWrite(LED_BUILTIN, LOW);

 // Set configuration portal time out  - for 3 mins ( enough?)
  wifiManager.setConfigPortalTimeout(180);
 //fetches ssid and pass from eeprom and tries to connect
 //if it does not connect it starts an access point with the specified name
 //here  "GARAGE"
 //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("SCHNUR")) {
    //if it is not able to connect - restart it and try again :)
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

 // if wlan fails we sit and wait the watchdog ;-) but actualle we should never get here
 while (WiFi.status() != WL_CONNECTED) {
   delay(150);
   digitalWrite(LED_BUILTIN, HIGH);
   delay(150);
   digitalWrite(LED_BUILTIN, LOW);
 }
 digitalWrite(LED_BUILTIN, HIGH);
 // initialize OTA update
 // in case, debug output can be placed in the stubs
 ArduinoOTA.setHostname("SCHNUR");
 ArduinoOTA.onStart([](){});
 ArduinoOTA.onEnd([](){});
 ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){});
 ArduinoOTA.onError([](ota_error_t error){});
 ArduinoOTA.begin();

 setupWebServer();
}
