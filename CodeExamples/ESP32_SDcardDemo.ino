/*
 Name:		ESP32_CoursePlatform.ino
 Created:	14-Jun-21 9:14:46 PM
 Author:	giltal
*/
#include "FS.h"
#include "SD.h"
#include "PCF8574.h"

SPIClass SDspi(HSPI); // VSPI is used by LCD
bool SDmountOK;
PCF8574 pcf8574(0x21);

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(115200);
	pcf8574.begin();
	pcf8574.pinMode(P0, OUTPUT); // SD CS

	pcf8574.digitalWrite(P0, LOW); // Set SD card chip select to low (enabled)	
	SDspi.begin(15, 36, 13, -1);
	
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
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	if (SDmountOK)
	{
		listSDcardFiles("/");
	}
	while (1);
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

