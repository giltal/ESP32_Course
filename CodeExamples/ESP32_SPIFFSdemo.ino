/*
 Name:		ESP32_CoursePlatform.ino
 Created:	14-Jun-21 9:14:46 PM
 Author:	giltal
*/
#include "SPIFFS.h"

// the setup function runs once when you press reset or power the board
void setup() 
{
	// Launch SPIFFS file system  
	if (!SPIFFS.begin()) 
	{
		Serial.println("An Error has occurred while mounting SPIFFS");
	}
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	File file = SPIFFS.open("/TestFile.txt");
	char buf[100];
	int fileSize;
	if (!file) 
	{
		Serial.println("Failed to open file for reading");
		while (1);
	}
	Serial.print("File size: ");
	Serial.println(fileSize = file.size());
	if (fileSize < 100)
	{
		file.readBytes(buf, fileSize);
		for (size_t i = 0; i < fileSize; i++)
		{
			printf("%c", buf[i]);
		}
		printf("\n");
	}
	
	file.close();
	while (1);
}
