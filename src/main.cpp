// -----------------------------------------------------------------------------
//
//  Copyright (C) 2021  Benrico Krog
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as published
//  by the Free Software Foundation version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>
//
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <CSE7766.h>
#include <POWR2.h>
#include <main.h>

CSE7766 _cseSensor;
ESP8266WebServer _server(80);

// Hardcoded wifi credentials
const char* wifiSSID = "";
const char* wifiPassword = "";

boolean LED_on = true;
boolean relayState = false;

void setup() {

  pinMode(LED_PIN, OUTPUT); // Initialise builtin LED
  digitalWrite(LED_PIN, HIGH);

  //Initialize CSE7766 sensor
  _cseSensor.setRX(1);
  _cseSensor.begin(); // serial: 4800 bps

  Serial.begin(115200);
  Serial.println("\nInitializing!\n\nConnecting to wifi ssid: " + String(wifiSSID) + " ");

  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) 
  {
    // Flash LED
    (LED_on) ? digitalWrite(LED_PIN, LOW) : digitalWrite(LED_PIN, HIGH);  
    delay(500);
    Serial.print(".");
  }

  digitalWrite(LED_PIN, HIGH);

  Serial.print("\n\nConnected!");
  Serial.print("\n\nIP address: ");
  Serial.print(WiFi.localIP());

  // Initialise and OPEN relay 
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);\

  //Defines accessable pages to webserver
  server.on("/", handleRoot);
  server.on("/data", handelGetData);
  server.on("/login", handleLogin);
  server.on("/info", []() {server.send(200, "text/plain", "Sonoff POWR2");});
  server.onNotFound(handleNotFound);

  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );
  server.begin();
  Serial.println("\nHTTP server started\n\n");
}

void loop() {
  server.handleClient();
}

void handleLogin()
{
  String msg;

  if (server.hasHeader("Cookie"))
  {   
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }

  //User Logout (remove cookie)
  if (server.hasArg("DISCONNECT"))
  {
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }

  //Check username and password
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
  {
    if (server.arg("USERNAME") == adminUser &&  server.arg("PASSWORD") == adminPassword )
    {
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      Serial.println("Log in Successful by user");
      return;
    }
    //Username or password failed
    msg = "<p class='wrongInput'>Wrong username/password! try again.</p>";
    Serial.println("Log in Failed");
  }

  //Login HTML (/login.html) | Converted to string by using included tool (/HTML_to-String.html)
  String html ="<!DOCTYPE html> <html> <head> <title>Iot-controller dashboard</title> <meta name=\"viewport\" content=\"width=device-width, minimumscale=1.0, maximum-scale=1.0, initial-scale=1\" /> <style> header {text-align: center; min-height: 110px; background-color: lightseagreen; color: white} h1 {font-size: 60px; margin-top: 0px; font-family: Georgia, 'Times New Roman', Times, serif} h2 {font-family: Arial, Helvetica, sans-serif} body {text-align: center} .inputSubmit { clear: both; position:relative; margin-top: 10px; -moz-box-shadow: 0px 1px 0px 0px #fff6af; -webkit-box-shadow: 0px 1px 0px 0px #fff6af; box-shadow: 0px 1px 0px 0px #fff6af; background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffec64), color-stop(1, #ffab23)); background:-moz-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-webkit-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-o-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-ms-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:linear-gradient(to bottom, #ffec64 5%, #ffab23 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffec64', endColorstr='#ffab23',GradientType=0); background-color:#ffec64; -moz-border-radius:6px; -webkit-border-radius:6px; border-radius:6px; border:1px solid #ffaa22; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:15px; padding:8px 50px; text-decoration:none; text-shadow:0px 1px 0px #ffee66; } .inputSubmit:hover { background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffab23), color-stop(1, #ffec64)); background:-moz-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-webkit-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-o-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-ms-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:linear-gradient(to bottom, #ffab23 5%, #ffec64 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffab23', endColorstr='#ffec64',GradientType=0); background-color:#ffab23; } .inputSubmit:active { position:relative; top:2px; } .password {margin-top: 4px;margin-right: 30px} .wrongInput {color: red; margin-top: 6px;} </style> </head> <header> <h1> Dashboard </h1> </header> <body> <form action='/login' method='POST'> User: <input type='text' name='USERNAME' placeholder=' username'><br> Password: <input type='password' class=\"password\" name='PASSWORD' placeholder=' password'> <br class=\"error\"> ";
  html += msg;
  html +=" <br> <input type='submit' class=\"inputSubmit\" name='SUBMIT' value='Submit'><br> </form> <br> <a href='/info'>info</a> </body> </html>";
  
  //send html to client
  server.send(200, "text/html", html);
}

//check if user has cookie
bool is_authenticated()
{
  Serial.println("Enter is_authenticated");
  if (server.hasHeader("Cookie"))
  {   
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);

    if (cookie.indexOf("ESPSESSIONID=1") != -1) 
    {
      Serial.println("Authentication Successful");
      return true;
    }
  }
  Serial.println("Authentication Failed");
  return false;  
}

//generate JSON string to be served on /data
void handelGetData()
{
  myCSE7766.handle();

  String jsonData = "{\"data\":[";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getVoltage());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getCurrent());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getActivePower());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getApparentPower());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getReactivePower());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += String(_cseSensor.getEnergy());
  jsonData += "\"},";
  jsonData += "{\"dataValue\":\"";
  jsonData += (relayState) ? "ON" : "OFF";
  jsonData += "\"}";
  jsonData += "]}";

  String header;

  //checks if user is athenticated before serving /data
  if (!is_authentified())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  
  //log 
  Serial.println();
  Serial.println("Client updated webpage");
  Serial.println();
  //send JSON string
  server.send(200, "application/json", jsonData);
}

//handles requested page that was not found 
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

  for (uint8_t i=0; i<server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}