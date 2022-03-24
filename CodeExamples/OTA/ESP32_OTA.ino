/*
 Name:		ESP32_Blynk.ino
 Created:	15-Jul-21 6:32:18 PM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"
// OTA related
#include <ESP32httpUpdate.h>
#include <HTTPClient.h>
#define VERSION 1.0

#define URL_fw_Version	"https://raw.githubusercontent.com/giltal/ESP32_Course/main/CodeExamples/OTA/version.txt"
#define URL_fw_Bin		"https://raw.githubusercontent.com/giltal/ESP32_Course/main/CodeExamples/OTA/firmware.bin"
#define	FINGER_PRINT	"84 63 B3 A9 29 12 CC FD 1D 31 47 05 98 9B EC 13 99 37 D0 D7"

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

#include <WiFi.h>
char ssid[] = "TPLINK";
char pass[] = "";

bool connectToWiFiSTA(const char * name, const char * pass);
bool OTA();

void setup()
{
	Serial.begin(115200);
	pcf8574.begin();
	pcf8574.pinMode(P6, OUTPUT); // Red LED
	pcf8574.pinMode(P7, OUTPUT); // Yellow LED
	pcf8574.digitalWrite(P7, HIGH);
	pcf8574.digitalWrite(P6, HIGH);

	if (!lcd.init(st7789_240x135x16_FB, &ttgoLCDaccessor, 16, 19, 18, 40000000))
	{
		printf("LCD init error\n");
		while (1);
	}
	// Blynk related
	WiFi.mode(WIFI_STA);
}

void loop()
{
	char str[50];
	lcd.fillScr(255, 255, 255);
	lcd.setColor(0, 0, 0);
	lcd.loadFonts(ORBITRON_LIGHT24);
	sprintf(str, "OTA ver: %.02f", VERSION);
	lcd.print(str, 0, 30, true);
	if (connectToWiFiSTA(ssid, pass))
	{
		lcd.print("WiFi connected", 0, 70, true);
		lcd.flushFrameBuffer();
		OTA();
	}
	lcd.flushFrameBuffer();
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


bool OTA()
{
	HTTPClient * http;
	http = new HTTPClient();
	if (http == NULL)
	{
		printf("Cannot allocate HTTPClient!\n");
	}
	else
	{
		float currentVer, serverVersion;
		String verStr(VERSION);
		currentVer = verStr.toFloat();
		bool httpOK = http->begin(URL_fw_Version, FINGER_PRINT);
		printf("Current version %.02f\n", currentVer);
		if (httpOK)
		{
			delay(100);
			int httpCode = http->GET();       // get data from version file
			delay(100);
			String payload;
			if (httpCode == HTTP_CODE_OK)    // if version received
			{
				printf("Try to get version file...\n");
				payload = http->getString();  // save received version
				printf("Got string...\n");
				Serial.println(payload.c_str());
				serverVersion = payload.toFloat();
				if (serverVersion > currentVer)
				{
					printf("New firmware available\n");
					printf("Current version: %f\n", currentVer);
					printf("New version: %f\n", serverVersion);

					//http->end();
					//delete http;

					printf("Free heap: %d\n", ESP.getFreeHeap());

					for (size_t i = 0; i < 3; i++)
					{
						t_httpUpdate_return ret = ESPhttpUpdate.update(URL_fw_Bin, "", FINGER_PRINT, true);

						switch (ret)
						{
						case HTTP_UPDATE_FAILED:
							printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
							break;

						case HTTP_UPDATE_NO_UPDATES:
							Serial.println("HTTP_UPDATE_NO_UPDATES");
							break;

						case HTTP_UPDATE_OK:
							Serial.println("HTTP_UPDATE_OK");
							break;
						}
					}
					return false;
				}
				http->end();
			}
			else
			{
				Serial.print("error in downloading version file:");
				Serial.println(httpCode);
			}
		}
		else
		{
			printf("Cannot connect to HTTP URL\n");
		}
		delete http;
	}
}