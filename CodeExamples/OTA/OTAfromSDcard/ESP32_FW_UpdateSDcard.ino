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
#include "OTAutil.h"

SPIClass SDspi(HSPI); // VSPI is used by LCD
bool SDmountOK;
// OTA related

#define VERSION 1.0

#define fw_Version_file	"/fw/version.txt"
#define fw_Bin_file		"/fw/firmware.bin"


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
	// SD Card
	SDspi.begin(15, 36, 13, -1);
	pcf8574.digitalWrite(P0, LOW); // Set SD card chip select to low (enabled)
	if (!SD.begin(50, SDspi, 1000000, "/sd", 5))
	{
		printf("Failed to mount SD card!\n");
		SDmountOK = false;
	}
	else
	{
		printf("SD card mounted on VSPI.\n");
		SDmountOK = true;
	}

	if (SDmountOK)
	{
		uint8_t cardType = SD.cardType();
		if (cardType == CARD_NONE)
		{
			Serial.println("No SD card attached\n");
			SDmountOK = false;
		}
	}
}

void loop()
{
	String str;
	float SDversion;
	char buf[50];
	lcd.fillScr(255, 255, 255);
	lcd.setColor(0, 0, 0);
	lcd.loadFonts(ORBITRON_LIGHT24);
	sprintf(buf, "FW ver: %.02f", VERSION);
	lcd.print(buf, 0, 30, true);
	lcd.flushFB();
	File verFile = SD.open(fw_Version_file, "r");
	if (verFile)
	{
		str = verFile.readString();
		printf("Current version %.02f\n", VERSION);
		SDversion = str.toFloat();
		printf("SD FW file version %.02f\n", SDversion);
		if (SDversion > (float)VERSION)
		{
			printf("New FW available - updating...\n");
			if (!updateFromSD(SD, fw_Bin_file))
			{
				printf("Error updating new FW!\n");
			}
			else
			{
				ESP.restart();
			}
		}
		verFile.close();
	}
	while (1);
}

