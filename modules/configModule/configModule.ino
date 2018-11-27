/*
  TODO:
    - Get time for disarm
    - Send config/time to controller module
    - Reset controller module?
    - Communication options:
      - I2C
      - Normal Serial
      - DSerial
      - Shared memory?
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "KTANECommon.h"
#include <EEPROM.h>

int led_pin = 2;

// Fill in your WiFi router SSID and password
const char* ssid = "Workshop-2G";
const char* password = "";
MDNSResponder mdns;

ESP8266WebServer server(80);

raw_config_t stored_config;
int num_minutes;

const char INDEX_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>KTANE SETUP</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>KTANE setup</h1>"
"<FORM action=\"/\" method=\"post\">"
"<P>"
"<b>Configure external features</b><br><br>"
"Serial Number: <INPUT type=\"text\" name=\"serial_num\"><BR>"
"Defuse time in minutes: "
"<select name=\"num_minutes\">"
"  <option value=\"0\">0</option>"
"  <option value=\"1\">1</option>"
"  <option value=\"2\">2</option>"
"  <option value=\"3\">3</option>"
"  <option value=\"4\">4</option>"
"  <option value=\"5\">5</option>"
"  <option value=\"6\" selected>6</option>"
"  <option value=\"7\">7</option>"
"  <option value=\"8\">8</option>"
"  <option value=\"9\">9</option>"
"</select><BR>"
"Number of batteries: "
"<select name=\"num_batteries\">"
"  <option value=\"0\">0</option>"
"  <option value=\"1\">1</option>"
"  <option value=\"2\">2</option>"
"  <option value=\"3\">3</option>"
"  <option value=\"4\">4</option>"
"  <option value=\"5\">5</option>"
"  <option value=\"6\">6</option>"
"  <option value=\"7\">7</option>"
"</select><BR>"
"Other items: <br>"
"<input type=\"checkbox\" name=\"port1\" value=\"port1\"> Lit FRK indicator<br>"
"<input type=\"checkbox\" name=\"port2\" value=\"port2\"> Lit CAR indicator<br>"
"<input type=\"checkbox\" name=\"port3\" value=\"port3\"> Parallel port<br>"
"<input type=\"checkbox\" name=\"port4\" value=\"port4\"> RJ45 port<br>"
"<input type=\"checkbox\" name=\"port5\" value=\"port5\"> Stereo RCA port<br>"
"<br>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit()
{
  config_t config;
  int addr = 0;

  if (!server.hasArg("serial_num") || !server.hasArg("num_batteries")) {
    return returnFail("BAD ARGS");
  }
  server.arg("serial_num").toCharArray(config.serial, 7);

  config.batteries = server.arg("num_batteries").toInt();
  config.indicators = ((!!server.hasArg("port1")) || 
                       ((!!server.hasArg("port2")) << 1)
                      );
  config.ports = (!!server.hasArg("port3") || 
                  ((!!server.hasArg("port4")) << 1) ||
                  ((!!server.hasArg("port5")) << 2)
                 );
  config_to_raw(&config, &stored_config);

  for(int i = 0; i < 6; i++){
    byte b = ((byte *)(&stored_config))[i];
    EEPROM.write(addr++, b);
  }
  // Write time
  EEPROM.commit();

  server.send(200, "text/html", INDEX_HTML);
}

void handleRoot()
{
  if (server.hasArg("serial_num")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

void handleNotFound()
{
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

void setup(void)
{
  Serial.begin(115200);

  EEPROM.begin(512);
  int addr = 0;
  for(int i = 0; i < 6; i++){
    byte b = EEPROM.read(addr++);
    ((byte *)(&stored_config))[i] = b;
  }
  // Read time

  pinMode(led_pin,  OUTPUT);

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

  if (mdns.begin("ktane-setup", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("Connect to http://ktane-setup.local or http://");
  Serial.println(WiFi.localIP());
}

void loop(void)
{
  server.handleClient();
  if(Serial.available() > 0) {
    while(Serial.available() > 0){
      // Throw away data
      Serial.read();
    }
    Serial.write((char *)(&stored_config), 6);
    Serial.write(num_minutes);
  }
}