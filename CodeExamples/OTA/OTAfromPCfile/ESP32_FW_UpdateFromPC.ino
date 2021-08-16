/*
 Name:		ESP32_Blynk.ino
 Created:	15-Jul-21 6:32:18 PM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"
#include "FS.h"
#include "SD.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h> 

const char* ssid = "TPLINK";
const char* pass = "";

AsyncWebServer server(80);

PCF8574 pcf8574(0x21);

ST77XX_FB lcd;

void setup()
{
	Serial.begin(115200);
	pcf8574.begin();
	pcf8574.pinMode(P6, OUTPUT); // Red LED
	pcf8574.pinMode(P7, OUTPUT); // Yellow LED
	pcf8574.digitalWrite(P7, HIGH);
	pcf8574.digitalWrite(P6, HIGH);

	if (!lcd.init())
	{
		printf("LCD init error\n");
		while (1);
	}

	WiFi.mode(WIFI_STA);
	if (connectToWiFiSTA(ssid, pass))
	{
		printf("WiFi connected\n");
	}

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", "Hi! I am ESP32.");
	});

	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
}

void loop()
{
	String str;
	lcd.fillScr(255, 255, 255);
	lcd.setColor(0, 0, 0);
	lcd.loadFonts(ORBITRON_LIGHT24);
	str = "PC File update";
	lcd.print((char *)str.c_str(), 0, 30, true);
	str = WiFi.localIP().toString();
	lcd.print((char *)str.c_str(), 0, 70, true);
	lcd.flushFB();
	
	while (1);
}

#define WIFI_TIMEOUT	20 // 10 seconds
bool connectToWiFiSTA(const char * name, const char * pass)
{
	unsigned int timeOutCounter = 0;
	WiFi.begin(name, pass);
	while ((WiFi.status() != WL_CONNECTED) && (timeOutCounter < WIFI_TIMEOUT))
	{
		delay(500);
		Serial.print(".");
		timeOutCounter++;
	}
	if (timeOutCounter != WIFI_TIMEOUT)
	{
		printf("\nWiFi connected.\n");
		Serial.println(WiFi.localIP().toString().c_str());
		Serial.println(WiFi.macAddress());
		return true;
	}
	else
	{
		printf("WiFi: cannot connect to: %s\n", name);
		return false;
	}
}