/*
 Name:		ESP32_Blynk.ino
 Created:	15-Jul-21 6:32:18 PM
 Author:	giltal
*/

/*************************************************************
  Download latest Blynk library here:
	https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

	Downloads, docs, tutorials: http://www.blynk.cc
	Sketch generator:           http://examples.blynk.cc
	Blynk community:            http://community.blynk.cc
	Follow us:                  http://www.fb.com/blynkapp
								http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  This example shows how to monitor a button state
  using interrupts mechanism.

  App project setup:
	LED widget on V1
 *************************************************************/
#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"

PCF8574 pcf8574(0x21);

ST77XX_FB lcd;

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#define noWIFI_BLYNK_DEMO
#ifdef WIFI_BLYNK_DEMO
#else
#define BLYNK_USE_DIRECT_CONNECT
#endif

#ifdef WIFI_BLYNK_DEMO
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "TPLINK";
char pass[] = "";
#else
#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#endif

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "_CAhRfN_3mWnYK7aK30gDLdr8CBfuUyL";

int virtualJoyX = 0, virtualJoyY = 0;

BLYNK_WRITE(V0)
{
	virtualJoyX = param[0].asInt();
	virtualJoyY = param[1].asInt();
	// Do something with x and y
	printf("X = %d, y = %d\n", virtualJoyX, virtualJoyY);
}

int virtualPinR = 0, virtualPinG = 0, virtualPinB = 0;

BLYNK_WRITE(V1)
{
	virtualPinR = param[0].asInt();
}
BLYNK_WRITE(V2)
{
	virtualPinG = param[0].asInt();
}
BLYNK_WRITE(V3)
{
	virtualPinB = param[0].asInt();
}

void setup()
{
	// Debug console
	Serial.begin(115200);
#ifdef WIFI_BLYNK_DEMO
	Blynk.begin(auth, ssid, pass);
#else
	Blynk.setDeviceName("ESP32Blynk");
	Blynk.begin(auth);
#endif	
	// Setup the IOs
	// Setup the IOs
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
	// Blynk related
	Serial.println("Waiting for connections...");
}

void loop()
{
	unsigned short x, y;
	Blynk.run();
	lcd.fillScr(255, 255, 255);
	lcd.setColor(virtualPinR, virtualPinG, virtualPinB);
	x = map(virtualJoyX, 0, 1023, 15, 225);
	y = map(virtualJoyY, 0, 1023, 120, 15);
	lcd.drawCircle(x ,y , 14, true);
	lcd.setColor(0, 0, 0);
	lcd.drawCircle(x, y, 15);
	lcd.flushFB();
}