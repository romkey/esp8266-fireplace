#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifdef OTA_UPDATES
#include <ArduinoOTA.h>
#endif

#ifdef MDNS_NAME
#include <ESP8266mDNS.h>
#endif

#include <ESP.h>

#include <BootstrapWebSite.h>
#include <BootstrapWebPage.h>

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
#include <IFTTTWebhook.h>
#endif

extern "C" {
  #include "user_interface.h"
};


// hardware configuration
// 4 - Wemos D1 mini lite, 5 - Sparkfun Thing
const int ledPin = 5;      // the number of the LED pin  -- 5 for sparkfun
const int relayPin = 1;   // 0 Sparkfun, D1 Wemos D1

bool fireplaceState = false;

#if defined(IFTTT_KEY) && defined (IFTTT_EVENT_NAME)
IFTTTWebhook ifttt(IFTTT_KEY, IFTTT_EVENT_NAME);
#endif

ESP8266WebServer server(80);

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

ADC_MODE(ADC_VCC);

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

  WiFi.hostname(MDNS_NAME);

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
    Serial.print("MDNS responder started: ");
    Serial.println(MDNS_NAME);

    MDNS.addService("http", "tcp", 80);
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
  ArduinoOTA.begin();
#endif /* OTA_UPDATES */
}

void loop() {
  server.handleClient();

#ifdef OTA_UPDATES
  ArduinoOTA.handle();
#endif
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
  page.addList(String("Uptime ") + String((millis() - startTime) / 1000) + " seconds",
	       String("IP address ") + WiFi.localIP().toString(),
	       String("Hostname ") + WiFi.hostname(),
	       String("MAC address ") + WiFi.macAddress(),
	       String("Subnet mask ") + WiFi.subnetMask().toString(),
	       String("router IP ") + WiFi.gatewayIP().toString(),
	       String("first DNS server ") + WiFi.dnsIP(0).toString(),
	       String("SSID ") + WiFi.SSID(),
	       String("RSSi ") + WiFi.RSSI());

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

  // from https://www.espressif.com/sites/default/files/documentation/esp8266_reset_causes_and_common_fatal_exception_causes_en.pdf
  char buffer[500] = "";
  struct rst_info* reset_info = system_get_rst_info();

  if(reset_info->reason == REASON_EXCEPTION_RST) {
    snprintf(buffer, 500, " cause %d, epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
	     reset_info->exccause, reset_info->epc1, reset_info->epc2, reset_info->epc3, reset_info->excvaddr, reset_info->depc);
  }

   page.addHeading(String("ESP"));
   page.addList(String("SDK ") + ESP.getSdkVersion() + String(" Core ") + ESP.getCoreVersion(),
		String("VCC ") + ESP.getVcc(),
		String("Free heap ") + ESP.getFreeHeap(),
		String("Chip ID ") + ESP.getChipId() ,
		String("Flash chip ID ") + ESP.getFlashChipId(),
		String("Flash chip size ") + ESP.getFlashChipSize(),
		String("Flash chip speed ") + ESP.getFlashChipSpeed(),
		String("Sketch Size ") + ESP.getSketchSize(),
		String("Free Sketch Space ") + ESP.getFreeSketchSpace(),
		String("Reset reason ") + ESP.getResetReason() + buffer);


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

