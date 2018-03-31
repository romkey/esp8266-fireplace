// this sketch waits for a push-button press, which it debounces
// then it closes a relay for a few seconds
// this tells our coffee bot that we've emptied the grounds bucket
// also turn on an LED and remember the fact that we did this

#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifdef OTA_UPDATES
#include <ESP8266HTTPUpdateServer.h>
#endif // OTA_UPDATES

#ifdef MDNS_NAME
#include <ESP8266mDNS.h>
#endif

#include <ESP.h>

#include <BootstrapWebSite.h>
#include <BootstrapWebPage.h>

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
#include <IFTTTWebhook.h>
#endif

// hardware configuration
// 4 - Wemos D1 mini lite, 5 - Sparkfun Thing
const int ledPin = 5;      // the number of the LED pin  -- 5 for sparkfun
const int relayPin = 1;   // 0 Sparkfun, D1 Wemos D1

bool fireplaceState = false;

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
IFTTTWebhook ifttt(IFTTT_KEY, IFTTT_EVENT_NAME);
#endif

ESP8266WebServer server(80);

#ifdef OTA_UPDATES
ESP8266HTTPUpdateServer httpUpdater;
#endif /* OTA_UPDATES */

BootstrapWebSite ws("en");

int ledState = LOW;         // the current state of the output pin
int relayState = LOW;         // the current state of the output pin
unsigned long startTime;

volatile bool interrupted = false;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
volatile unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers

void handleRoot(), handleOn(), handleOff(), handleInfo(), handleESP(), handleStateJSON(), handleNotFound();

void setup() {
  startTime = millis();

  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

#if DEBUG
  Serial.begin(115200);
  Serial.println("");
  Serial.println("attempting to connect");
  Serial.println("mac address");
  Serial.println(WiFi.macAddress());
#endif

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#if DEBUG
    Serial.println("derp");
#endif    
  }

#if DEBUG
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

#ifdef MDNS_NAME
  if (MDNS.begin(MDNS_NAME)) {
#if DEBUG
    Serial.println("MDNS responder started");
#endif
  }
#endif

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
  ifttt.trigger("boot");
#endif

  ws.addBranding(String(BRANDING_IMAGE), BRANDING_IMAGE_TYPE);

  ws.addPageToNav(String("Info"), String("/info"));
  ws.addPageToNav(String("ESP"), String("/esp"));

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  server.on("/state.json", handleStateJSON);
  server.on("/info", handleInfo);
  server.on("/esp", handleESP);

  server.onNotFound(handleNotFound);

  server.begin();
#if DEBUG  
  Serial.println("HTTP server started");
#endif

#ifdef OTA_UPDATES
  httpUpdater.setup(&server);
#endif /* OTA_UPDATES */
}

void loop() {
  server.handleClient();
}

void fireplaceOn() {
  if(fireplaceState) {
    return;
  }

#if DEBUG  
  Serial.println("fireplace on");
#endif

  fireplaceState = true;

  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, HIGH);

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
  ifttt.trigger("on");
#if DEBUG
  Serial.println("IFTTT notified");
#endif  
#endif
}

void fireplaceOff() {
  if(!fireplaceState) {
    return;
  }

#if DEBUG  
  Serial.println("fireplace off");
#endif

  fireplaceState = false;

  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW);

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
  ifttt.trigger("off");
#if DEBUG
  Serial.println("IFTTT notified");
#endif  
#endif
}

void handleInfo() {
  BootstrapWebPage page = BootstrapWebPage(&ws);
  page.addHeading("Info");
  page.addContent(String("<ul class='list-group'>" +
                                String("<li class='list-group-item'>Uptime ") + String((millis() - startTime)/1000) + String(" seconds</li>") +
                                String("<li class='list-group-item'>IP address ") + String(WiFi.localIP().toString()) + String("</li>") +
                                String("<li class='list-group-item'>Hostname ") + String(WiFi.hostname()) + String("</li>") +
                                String("<li class='list-group-item'>MAC address ") + WiFi.macAddress() + String("</li>") +
                                String("<li class='list-group-item'>Subnet mask ") + WiFi.subnetMask().toString() + String("</li>") +
                                String("<li class='list-group-item'>router IP ") + WiFi.gatewayIP().toString() + String("</li>") +
                                String("<li class='list-group-item'>first DNS server ") + WiFi.dnsIP(0).toString() + String("</li>") +
                                String("<li class='list-group-item'>SSID ") + WiFi.SSID() + String("</li>") +
                                String("<li class='list-group-item'>RSSi ") + WiFi.RSSI() + String("</li>") +
                                String("</ul>")));
  server.send(200, "text/html", page.getHTML());
}

void handleRoot() {
  BootstrapWebPage page = BootstrapWebPage(&ws);

  if(fireplaceState) {
    page.addHeading("Fireplace is on");
    page.addContent("<a href='/off' class='btn btn-secondary btn-lg' role='button'>Turn fireplace off</a>");
  } else {
    page.addHeading("Fireplace is off");
    page.addContent("<a href='/on' class='btn btn-primary btn-lg btn-danger' role='button'>Turn fireplace on</a>");
  }

 server.send(200, "text/html", page.getHTML());
}

void handleOn() {
  fireplaceOn();

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleOff() {
  fireplaceOff();

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleStateJSON() {
  char results[16];

  sprintf(results, "{ \"state\": %1d }", fireplaceState);
  server.send(200, "application/json", results);
}

void handleESP() {
   BootstrapWebPage page = BootstrapWebPage(&ws);
   page.addHeading(String("ESP"));
   page.addContent(String("<ul class='list-group'>") +
                                "<li class='list-group-item'>VCC " + ESP.getVcc() + " </li>" +
                                "<li class='list-group-item'>Free heap " + ESP.getFreeHeap() + "</li>" +
                                "<li class='list-group-item'>Chip ID " + ESP.getChipId() + "</li>" +
                                "<li class='list-group-item'>Flash chip ID " + ESP.getFlashChipId()  + "</li>" +
                                "<li class='list-group-item'>Flash chip size " + ESP.getFlashChipSize()  + "</li>" +
                                "<li class='list-group-item'>Flash chip speed " + ESP.getFlashChipSpeed()  + "</li>" +                             
                                "<li class='list-group-item'>Sketch Size " + ESP.getSketchSize() + " </li>" +
                                "<li class='list-group-item'>Free Sketch Space " + ESP.getFreeSketchSpace() + " </li></ul>");
   server.send(200, "text/html", page.getHTML());
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void format_seconds(unsigned long seconds, char* buffer) {
  
}

