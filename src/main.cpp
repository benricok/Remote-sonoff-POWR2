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
#include <CSE7766.h>
#include <POWR2.h>
#include <main.h>

CSE7766 _cseSensor;
ESP8266WebServer _server(80);

// Hardcoded wifi credentials
const char* wifiSSID = "";
const char* wifiPassword = "";
const char *adminUser = "";
const char *adminPassword = "";

boolean LED_on = true;
boolean relayState = false;

void setup() {

  pinMode(LED_PIN, OUTPUT); // Initialise builtin LED
  digitalWrite(LED_PIN, HIGH);

  //Initialize CSE7766 sensor
  _cseSensor.setRX(1);
  _cseSensor.setInverted(false);
  _cseSensor.begin(); // serial: 4800 bps

  //Serial.begin(115200);
  ////Serial.println("\nInitializing!\n\nConnecting to wifi ssid: " + String(wifiSSID) + " ");

  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) 
  {
    // Flash LED
    (LED_on) ? digitalWrite(LED_PIN, LOW) : digitalWrite(LED_PIN, HIGH); 
    (LED_on) ? LED_on = false : LED_on = true; 
    delay(100);
    //Serial.print(".");
  }

  digitalWrite(LED_PIN, HIGH);

  //Serial.print("\n\nConnected!");
  //Serial.print("\n\nIP address: ");
  //Serial.print(WiFi.localIP());

  // Initialise and OPEN relay 
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);\

  //Defines accessable pages to web_server
  _server.on("/", handleRoot);
  _server.on("/data", handelGetData);
  _server.on("/login", handleLogin);
  _server.on("/info", []() {_server.send(200, "text/plain", "Sonoff POWR2");});
  _server.onNotFound(handleNotFound);

  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  _server.collectHeaders(headerkeys, headerkeyssize );
  _server.begin();
  //Serial.println("\nHTTP _server started\n\n");
}

void loop() {
  _server.handleClient();
}

void handleLogin()
{
  String msg;

  if (_server.hasHeader("Cookie"))
  {   
    //Serial.print("Found cookie: ");
    String cookie = _server.header("Cookie");
    //Serial.println(cookie);
  }

  //User Logout (remove cookie)
  if (_server.hasArg("DISCONNECT"))
  {
    //Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    _server.sendContent(header);
    return;
  }

  //Check username and password
  if (_server.hasArg("USERNAME") && _server.hasArg("PASSWORD"))
  {
    if (_server.arg("USERNAME") == adminUser &&  _server.arg("PASSWORD") == adminPassword )
    {
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      _server.sendContent(header);
      //Serial.println("Log in Successful by user");
      return;
    }
    //Username or password failed
    msg = "<p class='wrongInput'>Wrong username/password! try again.</p>";
    //Serial.println("Log in Failed");
  }

  //Login HTML (/login.html) | Converted to string by using included tool (/HTML_to-String.html)
  String html ="<!DOCTYPE html> <html> <head> <title>SWITCH</title> <meta name=\"viewport\" content=\"width=device-width, minimumscale=1.0, maximum-scale=1.0, initial-scale=1\" /> <style> body {text-align: center; margin-top: 50px } .inputSubmit { clear: both; position:relative; margin-top: 10px; -moz-box-shadow: 0px 1px 0px 0px #fff6af; -webkit-box-shadow: 0px 1px 0px 0px #fff6af; box-shadow: 0px 1px 0px 0px #fff6af; background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffec64), color-stop(1, #ffab23)); background:-moz-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-webkit-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-o-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-ms-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:linear-gradient(to bottom, #ffec64 5%, #ffab23 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffec64', endColorstr='#ffab23',GradientType=0); background-color:#ffec64; -moz-border-radius:6px; -webkit-border-radius:6px; border-radius:6px; border:1px solid #ffaa22; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:15px; padding:8px 50px; text-decoration:none; text-shadow:0px 1px 0px #ffee66; } .inputSubmit:hover { background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffab23), color-stop(1, #ffec64)); background:-moz-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-webkit-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-o-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-ms-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:linear-gradient(to bottom, #ffab23 5%, #ffec64 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffab23', endColorstr='#ffec64',GradientType=0); background-color:#ffab23; } .inputSubmit:active { position:relative; top:2px; } .password {margin-top: 4px;margin-right: 30px} .wrongInput {color: red; margin-top: 6px;} </style> </head> <header> <h2>Please log in to view the control page</h2> </header> <body> <br> <form action='/login' method='POST'> User: <input type='text' name='USERNAME' placeholder=' username'><br> Password: <input type='password' class=\"password\" name='PASSWORD' placeholder=' password'> <br> <br class=\"error\"> ";
  html += msg;
  html +=" <br> <br> <input type='submit' class=\"inputSubmit\" name='SUBMIT' value='Submit'><br> </form> <br><br> <a href='/info'>info</a> </body> </html>";
  
  //send html to client
  _server.send(200, "text/html", html);
}

//check if user has cookie
bool is_authenticated()
{
  //Serial.println("Enter is_authenticated");
  if (_server.hasHeader("Cookie"))
  {   
    //Serial.print("Found cookie: ");
    String cookie = _server.header("Cookie");
    //Serial.println(cookie);

    if (cookie.indexOf("ESPSESSIONID=1") != -1) 
    {
      //Serial.println("Authentication Successful");
      return true;
    }
  }
  //Serial.println("Authentication Failed");
  return false;  
}

//generate JSON string to be served on /data
void handelGetData()
{
  _cseSensor.handle();

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

  //checks if user is authenticated before serving /data
  if (!is_authenticated())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    _server.sendContent(header);
    return;
  }
  
  //log 
  //Serial.println("Client updated webpage\n\n");
  //send JSON string
  _server.send(200, "application/json", jsonData);
}

void handleRoot()
{
  //Serial.println("Enter handleRoot");
  String header;
  _cseSensor.handle();

  if (!is_authenticated())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    _server.sendContent(header);
    return;
  }

  //Main HTML page (/Index.html) | Converted to string by using included tool (/HTML_to-String.html)
  String html ="<!DOCTYPE html> <html> <head> <title>SWITCH</title> <meta name=\"viewport\" content=\"width=device-width, minimumscale=1.0, maximum-scale=1.0, initial-scale=1\" /> <style> h2 { font-size: 30px; font-family: Arial, Helvetica, sans-serif} #Settings { display: none; margin-top: 30px } #Data { display: block; margin-top: 30px } #SENSORHEAD { font-size: 30px; font-family: Arial, Helvetica, sans-serif} body { margin-left: 50px; margin-top: 50px } .changeViewButton { clear: both; position:relative; margin-top: 30px; -moz-box-shadow: 0px 1px 0px 0px #fff6af; -webkit-box-shadow: 0px 1px 0px 0px #fff6af; box-shadow: 0px 1px 0px 0px #fff6af; background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffec64), color-stop(1, #ffab23)); background:-moz-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-webkit-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-o-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:-ms-linear-gradient(top, #ffec64 5%, #ffab23 100%); background:linear-gradient(to bottom, #ffec64 5%, #ffab23 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffec64', endColorstr='#ffab23',GradientType=0); background-color:#ffec64; -moz-border-radius:6px; -webkit-border-radius:6px; border-radius:6px; border:1px solid #ffaa22; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:15px; padding:8px 50px; text-decoration:none; text-shadow:0px 1px 0px #ffee66; } .changeViewButton:hover { background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #ffab23), color-stop(1, #ffec64)); background:-moz-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-webkit-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-o-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:-ms-linear-gradient(top, #ffab23 5%, #ffec64 100%); background:linear-gradient(to bottom, #ffab23 5%, #ffec64 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffab23', endColorstr='#ffec64',GradientType=0); background-color:#ffab23; } .changeViewButton:active { position:relative; top:4px; } .onButton { clear: both; position:relative; margin-left: 5px; margin-top: 10px; -moz-box-shadow: 0px 1px 0px 0px #5bda2a; -webkit-box-shadow: 0px 1px 0px 0px #5bda2a; box-shadow: 0px 1px 0px 0px #5bda2a; background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #5bda2a), color-stop(1, #5bda2a)); background:-moz-linear-gradient(top, #5bda2a 5%, #5bda2a 100%); background:-webkit-linear-gradient(top, #5bda2a 5%, #5bda2a 100%); background:-o-linear-gradient(top, #5bda2a 5%, #5bda2a 100%); background:-ms-linear-gradient(top, #5bda2a 5%, #5bda2a 100%); background:linear-gradient(to bottom, #5bda2a 5%, #5bda2a 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffec64', endColorstr='#ffab23',GradientType=0); background-color:#5bda2a; -moz-border-radius:6px; -webkit-border-radius:6px; border-radius:6px; border:1px solid #5bda2a; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:12px; padding:6px 17px; text-decoration:none; text-shadow:0px 1px 0px #5bda2a; } .onButton:hover { background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #a7ff8c), color-stop(1, #a7ff8c)); background:-moz-linear-gradient(top, #a7ff8c 5%, #a7ff8c 100%); background:-webkit-linear-gradient(top, #a7ff8c 5%, #a7ff8c 100%); background:-o-linear-gradient(top, #a7ff8c 5%, #a7ff8c 100%); background:-ms-linear-gradient(top, #a7ff8c 5%, #a7ff8c 100%); background:linear-gradient(to bottom, #a7ff8c 5%, #a7ff8c 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#a7ff8c', endColorstr='#a7ff8c',GradientType=0); background-color:#a7ff8c; } .onButton:active { position:relative; top:4px; } .offButton { clear: both; position:relative; margin-left: 8px; margin-top: 10px; -moz-box-shadow: 0px 1px 0px 0px #f06262; -webkit-box-shadow: 0px 1px 0px 0px #f06262; box-shadow: 0px 1px 0px 0px #f06262; background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #f06262), color-stop(1, #f06262)); background:-moz-linear-gradient(top, #f06262 5%, #f06262 100%); background:-webkit-linear-gradient(top, #f06262 5%, #f06262 100%); background:-o-linear-gradient(top, #f06262 5%, #f06262 100%); background:-ms-linear-gradient(top, #f06262 5%, #f06262 100%); background:linear-gradient(to bottom, #f06262 5%, #f06262 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffec64', endColorstr='#ffab23',GradientType=0); background-color:#f06262; -moz-border-radius:6px; -webkit-border-radius:6px; border-radius:6px; border:1px solid #f06262; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:12px; padding:6px 15px; text-decoration:none; text-shadow:0px 1px 0px #f06262; } .offButton:hover { background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #df7e7e), color-stop(1, #df7e7e)); background:-moz-linear-gradient(top, #df7e7e 5%, #df7e7e 100%); background:-webkit-linear-gradient(top, #df7e7e 5%, #df7e7e 100%); background:-o-linear-gradient(top, #df7e7e 5%, #df7e7e 100%); background:-ms-linear-gradient(top, #df7e7e 5%, #df7e7e 100%); background:linear-gradient(to bottom, #df7e7e 5%, #df7e7e 100%); filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#df7e7e', endColorstr='#df7e7e',GradientType=0); background-color:#df7e7e; } .offButton:active { position:relative; top:4px; } </style> </head> <header> <h2> REMOTE MONITOR & CONTROL </h2> </header> <body> <div id='Data' > <b id=\"SENSORHEAD\">Sensor:</b> <div id='HVoltage'> <h3> Voltage: <b id='Voltage'>";
  html += String(_cseSensor.getVoltage());
  html +="</b> V </h3> </div> <div id='HCurrent'> <h3> Current: <b id='Current'>";
  html += String(_cseSensor.getCurrent());
  html +="</b> A </h3> </div> <div id='HActivePower'> <h3> ActivePower: <b id='ActivePower'>";
  html += String(_cseSensor.getActivePower());
  html +="</b> W </h3> </div> <div id='HApparentPower'> <h3> ApparentPower: <b id='ApparentPower'>";
  html += String(_cseSensor.getApparentPower());
  html +="</b> VA </h3> </div> <div id='HReactivePower'> <h3> ReactivePower: <b id='ReactivePower'>";
  html += String(_cseSensor.getReactivePower());
  html +="</b> VAr </h3> </div> <div id='HEnergy'> <h3> Energy: <b id='Energy'>";
  html += String(_cseSensor.getEnergy());
  html +="</b> kWh </h3> </div> <form action=\"/\" method=\"POST\"> <div id='RELAY'> <h2> Device Status: <b id=\"RELAY_INFO\">";
  html += (relayState) ? "ON" : "OFF";
  html +="</b> <br> <button class=\"onButton\" name=\"RELAY-ON\">ON</button><button class=\"offButton\" name=\"RELAY-OFF\">OFF</button> </h2> </div> </form> </div> <div id='Settings'> <h2>Update rate:</h2> <h3>Update rate:</h3> <input type=\"range\" style=\"width: 300px\" min=\"200\" max=\"5000\" value=\"2000\" id=\"fader\" step=\"1\" oninput=\"outputUpdate(value)\"> <output for=\"fader\" id=\"#slide\">2000</output> </div> <div> <button onClick=\"ChangeView()\" id=\"changeView1\" class=\"changeViewButton\">Update rate</button> </div> <script> \"use strict\"; var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) { function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); } return new (P || (P = Promise))(function (resolve, reject) { function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } } function rejected(value) { try { step(generator[\"throw\"](value)); } catch (e) { reject(e); } } function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); } step((generator = generator.apply(thisArg, _arguments || [])).next()); }); }; var __generator = (this && this.__generator) || function (thisArg, body) { var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g; return g = { next: verb(0), \"throw\": verb(1), \"return\": verb(2) }, typeof Symbol === \"function\" && (g[Symbol.iterator] = function() { return this; }), g; function verb(n) { return function (v) { return step([n, v]); }; } function step(op) { if (f) throw new TypeError(\"Generator is already executing.\"); while (_) try { if (f = 1, y && (t = op[0] & 2 ? y[\"return\"] : op[0] ? y[\"throw\"] || ((t = y[\"return\"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t; if (y = 0, t) op = [op[0] & 2, t.value]; switch (op[0]) { case 0: case 1: t = op; break; case 4: _.label++; return { value: op[1], done: false }; case 5: _.label++; y = op[1]; op = [0]; continue; case 7: op = _.ops.pop(); _.trys.pop(); continue; default: if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; } if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; } if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; } if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; } if (t[2]) _.ops.pop(); _.trys.pop(); continue; } op = body.call(thisArg, _); } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; } if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true }; } }; var Changed = false; var slide = document.querySelector('#fader'); slide.addEventListener('input', function () { outputUpdate(slide.valueAsNumber); }); var interval = -1; document.querySelector('button#changeView1').addEventListener('click', function () { if (Changed == false) { document.getElementById('Data').style.display = 'none'; document.getElementById('Settings').style.display = 'block'; document.getElementById('changeView1').innerHTML = \"Main\"; Changed = true; } else { document.getElementById('Data').style.display = 'block'; document.getElementById('Settings').style.display = 'none'; document.getElementById('changeView1').innerHTML = \"Update rate\"; Changed = false; } }); function loadDoc() { return __awaiter(this, void 0, void 0, function () { var res, obj; return __generator(this, function (_a) { switch (_a.label) { case 0: return [4 /*yield*/, fetch('/data', { credentials: \"include\" })]; case 1: res = _a.sent(); return [4 /*yield*/, res.json()]; case 2: obj = _a.sent(); document.getElementById(\"Voltage\").innerHTML = obj.data[0].dataValue; document.getElementById(\"Current\").innerHTML = obj.data[1].dataValue; document.getElementById(\"ActivePower\").innerHTML = obj.data[2].dataValue; document.getElementById(\"ApparentPower\").innerHTML = obj.data[3].dataValue; document.getElementById(\"ReactivePower\").innerHTML = obj.data[4].dataValue; document.getElementById(\"Energy\").innerHTML = obj.data[5].dataValue; document.getElementById(\"RELAY_INFO\").innerHTML = obj.data[6].dataValue; return [2 /*return*/]; } }); }); } function outputUpdate(vol) { if (interval > 0) clearInterval(interval); interval = setInterval(function () { loadDoc()[\"catch\"](console.error); }, vol); } outputUpdate(2000); </script> </body> </html>";

  _server.send(200, "text/html", html);

  //Checks if client provided input on main webpage and changes corresponding value
  if (_server.hasArg("RELAY-ON"))
  {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    //Serial.println("Relay switched on!\n\n");
  } 
  if (_server.hasArg("RELAY-OFF"))
  {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    //Serial.println("Relay switched off!\n\n");
  } 
}

//handles requested page that was not found 
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += _server.uri();
  message += "\nMethod: ";
  message += (_server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += _server.args();
  message += "\n";

  for (uint8_t i=0; i<_server.args(); i++)
  {
    message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
  }
  _server.send(404, "text/plain", message);
}
