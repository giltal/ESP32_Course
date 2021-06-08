#include "OTAutil.h"
#include "Update.h"
#include <FS.h>
#include <SD.h>

// perform the actual update from a given stream
bool performUpdate(Stream &updateSource, size_t updateSize)
{
	if (Update.begin(updateSize))
	{
		size_t written = Update.writeStream(updateSource);
		if (written == updateSize)
		{
			Serial.println("Written : " + String(written) + " successfully");
		}
		else
		{
			Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
		}
		if (Update.end())
		{
			Serial.println("OTA done!");
			if (Update.isFinished())
			{
				Serial.println("Update successfully completed. Please reboot.");
				return true;
			}
			else
			{
				Serial.println("Update not finished? Something went wrong!");
				return false;
			}
		}
		else
		{
			Serial.println("Error Occurred. Error #: " + String(Update.getError()));
		}

	}
	else
	{
		Serial.println("Not enough space to begin OTA");
	}
	return false;
}

// Useage: updateFromSD(SD,"/ESP32_TV_EMU_SMS.bin"); // Place file on SD
bool updateFromSD(fs::FS &fs, char * fileName)
{
	File updateBin = fs.open(fileName);
	if (updateBin)
	{
		if (updateBin.isDirectory())
		{
			printf("Error, %s is not a file\n", fileName);
			updateBin.close();
			return false;
		}

		size_t updateSize = updateBin.size();

		if (updateSize > 0)
		{
			printf("Try to start update\n");
			performUpdate(updateBin, updateSize);
			updateBin.close();
			return true;
		}
		else
		{
			Serial.println("Error, file is empty");
		}
		updateBin.close();
	}
	else
	{
		printf("Could not load update.bin from sd root\n");
	}
	return false;
}
