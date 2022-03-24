/*
 Name:		ESP32_WiFi_Lab.ino
 Created:	14-Jul-21 9:38:27 PM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"

// WiFi part
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

WebServer server(80);
void handle_OnConnect();
void wifiSetup_page();
void toggleIO_page();
void handle_NotFound();

bool wifiSetupPageVisited = false;
bool toggleIOpageVisited = false;

PCF8574 pcf8574(0x21);
SPI_LCD_FrameBuffer lcd;

class ttgoCDacc : public lcdHwAccessor
{
public:
	ttgoCDacc() {};
	~ttgoCDacc() {};
	void setup()
	{
		pinMode(5, OUTPUT); //chip select
		pinMode(23, OUTPUT); //reset
		pinMode(4, OUTPUT); //Back Light
	}
	void reset()
	{
		digitalWrite(23, LOW);
		delay(250);
		digitalWrite(23, HIGH);
		delay(250);
	};
	void assertCS()
	{
		digitalWrite(5, LOW);
	}
	void deAssertCS()
	{
		digitalWrite(5, HIGH);
	}
	void backLightOn()
	{
		digitalWrite(4, HIGH);
	}
	void backLightOff()
	{

	}
} ttgoLCDaccessor;
// the setup function runs once when you press reset or power the board
void setup()
{
	// Setup the IOs
	Serial.begin(115200);
	pcf8574.begin();
	pcf8574.pinMode(P6, OUTPUT); // Red LED
	pcf8574.pinMode(P7, OUTPUT); // Yellow LED
	pcf8574.digitalWrite(P7, LOW);
	pcf8574.digitalWrite(P6, LOW);

	if (!lcd.init(st7789_240x135x16_FB, &ttgoLCDaccessor, 16, 19, 18, 40000000))
	{
		printf("LCD init error\n");
		while (1);
	}

#define WIFI_TIMEOUT	20 // 10 seconds
	// WiFi Part
	WiFi.mode(WIFI_AP);
	if (!WiFi.softAP("ESP32AP", "12345678"))
	{
		Serial.println("Failed to init WiFi AP");
	}
	else
	{
		Serial.print("AP IP address: ");
		Serial.println((WiFi.softAPIP()));
	}
	if (!MDNS.begin("esp32"))
	{
		printf("Error setting up MDNS responder!\n");
	}
	else
		printf("mDNS responder started\n");
	MDNS.addService("http", "tcp", 80);
	// Web pages setup
	server.on("/", handle_OnConnect);
	server.on("/wifiSetupSelected", wifiSetup_page);
	server.on("/ioPageSelected", toggleIO_page);//toggleIO_page
	server.onNotFound(handle_NotFound);
	server.begin();
}

// the loop function runs over and over again until power down or reset
void loop()
{
	int numOfClientsConnected;
	lcd.loadFonts(ORBITRON_LIGHT24);
	lcd.fillScr(255, 255, 255);
	lcd.setColor(255, 0, 0);
	lcd.print("ON", 0, 40, true);
	lcd.flushFrameBuffer();
	bool IO3set = true;

	while (1)
	{
		numOfClientsConnected = WiFi.softAPgetStationNum();
		if (numOfClientsConnected > 0)
		{
			server.handleClient();
			if (wifiSetupPageVisited)
			{
				if (server.args() >= 2)
				{ // Arguments were received
					String ssidName = server.arg(0);
					String ssidPassword = server.arg(1);

					Serial.println(server.args());
					Serial.println((const char*)ssidName.c_str());
					Serial.println((const char*)ssidPassword.c_str());
					wifiSetupPageVisited = false;
				}
			}
			if (toggleIOpageVisited)
			{
				toggleIOpageVisited = false;
				if (server.args() > 0)
				{ // Arguments were received
					String ioState = server.arg(0);

					Serial.println(server.args());
					Serial.println((const char*)ioState.c_str());
					if (!strcmp(ioState.c_str(), "IO 1 ON"))
					{
						pcf8574.digitalWrite(P7, HIGH);
					}
					if (!strcmp(ioState.c_str(), "IO 1 OFF"))
					{
						pcf8574.digitalWrite(P7, LOW);
					}
					if (!strcmp(ioState.c_str(), "IO 2 ON"))
					{
						pcf8574.digitalWrite(P6, HIGH);
					}
					if (!strcmp(ioState.c_str(), "IO 2 OFF"))
					{
						pcf8574.digitalWrite(P6, LOW);
					}
					if (!strcmp(ioState.c_str(), "IO 3 ON"))
					{
						IO3set = true;
					}
					if (!strcmp(ioState.c_str(), "IO 3 OFF"))
					{
						IO3set = false;
					}
				}
			}
		}
		lcd.fillScr(255, 255, 255);
		lcd.setColor(255, 0, 0);
		if (IO3set)
		{
			lcd.print("IO3 ON", 0, 20, true);
		}
		else
		{
			lcd.print("IO3 OFF", 0, 20, true);
		}
		lcd.print("AP address", 0, 60, true);
		lcd.print((char *)WiFi.softAPIP().toString().c_str(), 0, 100, true);
		lcd.flushFrameBuffer();
	}
}

const char mainMenuPage[]PROGMEM = R"rawliteral(
<!DOCTYPE html> 
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 WebPage</title>
<style>
html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
</head>
<body>
<meta charset="utf-8">
<html lang="he">
<h1>Intel's Makers Demo Web Page</h1>
<a class="button button-on" href="/wifiSetupSelected">WiFi Setup</a>
<a class="button button-on" href="/ioPageSelected">IO Control</a>
</body>
<href="/">
</html>
)rawliteral";

const char setupWiFiHTML[]PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
html {font-family: Arial; display: inline-block; text-align: center;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
.boxStyle {  padding: 12px 20px;  margin: 8px 0;  box-sizing: border-box;  border: 2px solid red;  border-radius: 10px; font-size: 20px;text-align: center;}
</style>
</head>
<body>
<form action="/" method="POST">
Access Point Name:<br>
<input type="text" class="boxStyle" name="AccessPoint" value=""><br>
Password:<br>
<input type="text" class="boxStyle" name="Password" value=""><br>
<input type="submit" class="button" value="OK">
</form>
</body>
<href="/">
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
  </style>
</head>
<body>
<h2>ESP Web Server</h2>
<h4>IO #1</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="1" checked><span class="slider"></span></label>
<h4>IO #2</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="2" checked><span class="slider"></span></label>
<h4>IO #3</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="3" checked><span class="slider"></span></label>
<h4><a class="button button-on" href="/">Home</a></h4>
<script>function toggleCheckbox(element) {
	var request = new XMLHttpRequest();
	var strOn	= "IO " + element.id + " ON";
	var strOff	= "IO " + element.id + " OFF";
	if(element.checked){ request.open("POST", "", true); request.setRequestHeader("Content-type", " ");request.send(strOn);}
	else { request.open("POST", "", true); request.setRequestHeader("Content-type", " ");request.send(strOff);}
	die();
}</script>
</body>
<href="/">
</html>
)rawliteral";

void handle_OnConnect()
{
	toggleIOpageVisited = false;
	server.send(200, "text/html", mainMenuPage);
}

void wifiSetup_page()
{
	wifiSetupPageVisited = true;
	toggleIOpageVisited = false;
	server.send(200, "text/html", setupWiFiHTML);
	printf("wifiSetupPageVisited\n");
}

void toggleIO_page()
{
	toggleIOpageVisited = true;
	wifiSetupPageVisited = false;
	server.send(200, "text/html", index_html);
	printf("toggleIOpageVisited\n");
}

void handle_NotFound()
{
	server.send(404, "text/plain", "Not found");
}