#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// ***** SETUP - CACHE
StaticJsonBuffer<600> jsonBuffer;

char jsonChar[600];

JsonObject& root = jsonBuffer.parseObject("{\"version\":\"1.0.0\", \"mac\":\"\", \"wifiStrength\":\"0\", \"setup\":null, \"disabled\":null, \"awake\":null, \"locked\":null}");

// ***** SETUP - MDNS - http://esp8266.local
MDNSResponder mdns;

// ***** SETUP - WEB SERVER
ESP8266WebServer server(80);

// ***** SETUP - WIFI  
WiFiClient client;

const char* ssid = "4585_IOT";
const char* password = "1234567890";

// ***** SETUP - HOOK
//const char* host = "dweet.io";
const char* host = "";
const int httpPort = 80;
const String path = "/dweet/for/formosa123";

// ***** SETUP - GPIO
int ledPin = 16;
int brownCable = 14;
int orangeCable = 12;
int blackCable = 13;

SoftwareSerial keypadEvents(brownCable, blackCable);
SoftwareSerial deadboltEvents(orangeCable, blackCable);

// ***** SETUP - INIT
void setup() {

  Serial.begin(115200);

  Serial.println("");
  Serial.println("ESP8266 mode: normal");
  
  // WIFI - INIT
  WiFi.begin(ssid, password);
  
  Serial.print("WiFi: connecting: ");
    
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  
  Serial.print("WiFi: connected: ");

  Serial.print(WiFi.localIP());

  long rssi = WiFi.RSSI();
  
  Serial.print(": ");
  Serial.print(rssi);
  Serial.println(" dBm");
  
  publish("booting");

  // MDNS - INIT
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS: started");
  }

  // WEB SERVER - ON - /
  server.on("/", []() {
    
    sync();

    server.send(200, "text/json", jsonChar);
    
  });

  // WEB SERVER - INIT
  server.begin();
  
  Serial.println("Web Server: Started");

  // EVENTS - INIT
  keypadEvents.begin(9600);
  deadboltEvents.begin(9600);

  // SET PIN MODE
  pinMode(brownCable, INPUT);
  pinMode(orangeCable, INPUT);
  pinMode(blackCable, INPUT);

}

void loop() {

  server.handleClient();

  int d = 0;

  String kBits = "";

  while (keypadEvents.available() > 0) kBits = kBits + keypadEvents.read();
  
  if (kBits.length()) {

    d = 100;
    
    command(kBits);
    
  }
  
  String dBits = "";

  while (deadboltEvents.available() > 0) dBits = dBits + deadboltEvents.read();

  if (dBits.length()) {

    d = 500;

    command(dBits);

  }

  delay(d);
  
}

void command(String data) {

  if (data.length() < 2) return;

  data = data.substring(0, 3);

  data.trim();

  if (data.length() == 2) data = data + "0";
  
  if (data == "010") data = "changed";
  if (data == "102") data = "done";
  if (data == "105") data = "waken";
  if (data == "106") data = "snooze";
  if (data == "107") data = "disabled";
  if (data == "108") data = "enabled";
  
  if (data == "424") data = "back";
  if (data == "353") data = "enter";
  if (data == "666") data = "smuge";
  if (data == "990") data = "setup";
  
  if (data == "980") data = "unlocking";
  if (data == "989") data = "unlocked";
  
  if (data == "970") data = "locking";
  if (data == "979") data = "locked";

  if (data.indexOf("48") > -1) data = "0";
  if (data.indexOf("49") > -1) data = "1";
  if (data.indexOf("50") > -1) data = "2";
  if (data.indexOf("51") > -1) data = "3";
  if (data.indexOf("52") > -1) data = "4";
  if (data.indexOf("53") > -1) data = "5";
  if (data.indexOf("54") > -1) data = "6";
  if (data.indexOf("55") > -1) data = "7";
  if (data.indexOf("56") > -1) data = "8";
  if (data.indexOf("57") > -1) data = "9";

  if (data == "locked") root["locked"] = true;
  if (data == "unlocked") root["locked"] = false;

  if (data == "disabled") root["disabled"] = true;
  if (data == "enabled") root["disabled"] = false;

  if (data == "waken") root["awake"] = true;
  if (data == "setup") root["setup"] = true;
  
  if (
    data == "done"
    || data == "snooze"
    || data == "changed"
   ) {

    root["setup"] = false;
    root["awake"] = false;
    
   }

  publish(data);
  
}

void publish(String data) {

  Serial.print("Event: ");
  Serial.println(data);

  // HOOK
  if (host && client.connect(host, httpPort)) {

    client.print(String("GET " + path + "?message=" + data) + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
                 
  }
 
}

void sync() {

  root[String("mac")] = WiFi.macAddress();
  root[String("wifiStrength")] = WiFi.RSSI();
    
  root.printTo((char*)jsonChar, root.measureLength() + 1);
  
}
