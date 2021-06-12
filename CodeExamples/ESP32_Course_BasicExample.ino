/*
 Name:		ESP32_TTGO_LCD.ino
 Created:	8/6/2020 1:56:38 PM
 Author:	giltal
*/
#include "graphics.h"
#include <Wiimote.h>
#include "FS.h"
#include "SD.h"
#include "esp_partition.h"
#include "Wire.h"
#include "PCF8574.h"

WII_KEYS wiiMoteKeys;
PCF8574 pcf8574(0x21);

void wiiMoteTask(void * pvParameters)
{
	while (1)
	{
		Wiimote::handle();
		Wiimote::readKeys(&wiiMoteKeys);
		delay(50);
	}
}

SPIClass SDspi(HSPI); // VSPI is used by LCD
bool SDmountOK;

//ST77XX lcd(_320x240);
ST77XX lcd(_240x135);

// the setup function runs once when you press reset or power the board
void setup() 
{
	Wire.begin(-1, -1, 100000); // -1 means: use default pins 
	
	Serial.begin(115200);
	printf("ST77XX init\n");
	lcd.init(40000000);

	pcf8574.begin();
	
	pcf8574.pinMode(P0, OUTPUT); // SD CS
	pcf8574.digitalWrite(P0, HIGH);

	pcf8574.pinMode(P7, OUTPUT); // Yellow LED
	pcf8574.digitalWrite(P7, HIGH);
	pcf8574.pinMode(P6, OUTPUT);
	pcf8574.digitalWrite(P6, HIGH); // Red LED
	// Setup the 3 switches (buttons)
	pcf8574.pinMode(P1, INPUT); // S1 - Right
	pcf8574.pinMode(P2, INPUT); // S2 - Middle
	pcf8574.pinMode(P3, INPUT); // S3 - Left

	pinMode(39, INPUT); // IO expander interrupt
	// Motor 1
	pinMode(32, OUTPUT); // 
	pinMode(33, OUTPUT); // 
	digitalWrite(32, LOW);
	digitalWrite(33, LOW);
	pcf8574.pinMode(P4, OUTPUT); // Motors enable
	pcf8574.digitalWrite(P4, HIGH);
	
	if (psramInit())
		printf("PSRAM init OK\n");
	printf("PSRAM size: %d\n", ESP.getFreePsram());
	if (ESP.getFreePsram() > 0)
	{
		Wiimote::init(true);
	}
	else
	{
		Wiimote::init(false);
	}

	xTaskCreatePinnedToCore(
		wiiMoteTask, /* Task function. */
		"wiiMoteTask",   /* name of task. */
		3000,     /* Stack size of task */
		NULL,      /* parameter of the task */
		5,         /* priority of the task */
		NULL,    /* Task handle to keep track of created task */
		1);        /* pin task to core 1 */

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
	// Search for I2C devices
	for (char i = 0; i < 128; i++)
	{
		Wire.beginTransmission(i);
		if (Wire.endTransmission() == 0)
		{
			Serial.print("Device found at address: ");
			Serial.println((int)i, HEX);
		}
	}
}

bool listSDcardFiles(char * StartFolder);

// the loop function runs over and over again until power down or reset
void loop() 
{
	unsigned long cycles = ESP.getCycleCount();
	lcd.fillScr(0, 0, 0);
	printf("Full screen fill FPS = %f\n", 1.0/((ESP.getCycleCount() - cycles) / 240000000.0));
	lcd.setColor(0, 255, 0);
	lcd.drawRect(0, 0, lcd.getXSize() - 1, lcd.getYSize() - 1);
		
	/*lcd.loadFonts(ORBITRON_LIGHT32);
	lcd.setColor(255, 0, 0);
	lcd.print("Red", 0, 5, true);
	lcd.setColor(0, 255, 0);
	lcd.print("Green", 0, 50, true);
	lcd.setColor(0, 0, 255);
	lcd.print("Blue", 0, 100, true);
	cycles = ESP.getCycleCount();
	for (size_t i = 120; i < 240; i++)
	{
		lcd.setColor(i, 0, 0);
		lcd.drawLine(0, i, 134, i);
	}
	printf("Time: %f\n", (ESP.getCycleCount() - cycles) / 240000000.0);*/
	lcd.loadFonts(ORBITRON_LIGHT32);
	lcd.setColor(255, 255, 255);
	lcd.print("ESP32", 0, 52, true);
	listSDcardFiles("/myFolder");
	while (1)
	{
		//printf("%s\n", wiiStr);
		/*if (wiiMoteKeys.nunDataAvailable)
		{
			printf("*** sx- %d sy- %d c- %d z- %d 1-%d 2-%d\n", (unsigned int)wiiMoteKeys.nX, (unsigned int)wiiMoteKeys.nY, wiiMoteKeys.nC, wiiMoteKeys.nZ, wiiMoteKeys.b1,
					wiiMoteKeys.b2);
			delay(1);
		}
		else
		{
			printf("### 1-%d 2-%d\n", wiiMoteKeys.b1, wiiMoteKeys.b2);

			delay(1);
		}*/	

		printf("Int: %d ", digitalRead(39));
		pcf8574.digitalWrite(P6, HIGH);
		pcf8574.digitalWrite(P7, HIGH);
		delay(100);
		printf("S1: %d S2: %d S3: %d \n", pcf8574.digitalRead(P1), pcf8574.digitalRead(P2), pcf8574.digitalRead(P3));
		pcf8574.digitalWrite(P6, LOW);
		pcf8574.digitalWrite(P7, LOW);
		delay(100);
		if (pcf8574.digitalRead(P1) == HIGH)
		{
			digitalWrite(32, HIGH);
			digitalWrite(33, LOW);
		}
		else if(pcf8574.digitalRead(P3) == HIGH)
		{
			digitalWrite(32, LOW);
			digitalWrite(33, HIGH);
		}
		else
		{
			digitalWrite(32, LOW);
			digitalWrite(33, LOW);
		}
	}
}

bool listSDcardFiles(char * StartFolder)
{
	File root;
	File entry;
	String fileName;

	printf("Start folder str size: %d\n", strlen(StartFolder));
	root = SD.open(StartFolder);
	int totalEntries = 0;
	// First: get the number of files within the directory
	if (!root)
	{
		printf("Error - no such directory...\n");
		return false;
	}

	do
	{
		entry = root.openNextFile();
		if (entry)
		{
			if (!entry.isDirectory()) // File
			{
				fileName = entry.name();
				if (strlen(StartFolder) > 2)
				{
					fileName.remove(0, strlen(StartFolder) + 1);
				}
				printf("%s %d\n", fileName.c_str(), entry.size());
			}
			else // Directory
			{
				printf("DIR: %s\n", entry.name());
			}
			totalEntries++;
		}
	} while (entry);

	if (totalEntries == 0)
	{
		printf("No files in directory\n");
		return false;
	}
	return true;
}
