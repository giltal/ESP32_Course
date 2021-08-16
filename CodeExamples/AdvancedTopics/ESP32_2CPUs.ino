/*
 Name:		ESP32_Blynk.ino
 Created:	15-Jul-21 6:32:18 PM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"

PCF8574 pcf8574(0x21);

ST77XX_FB lcd;

TaskHandle_t Task1;
TaskHandle_t Task2;

bool virtualPin1 = true, virtualPin2 = true;

void Task1code(void * pvParameters)
{
	while (1)
	{
		virtualPin1 = true;
		delay(1000);
		virtualPin1 = false;
		delay(1000);
	}
}

void Task2code(void * pvParameters)
{
	while (1)
	{
		virtualPin2 = true;
		delay(1000);
		virtualPin2 = false;
		delay(1000);
	}
}

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

	xTaskCreatePinnedToCore(
		Task1code, /* Task function. */
		"Task1",   /* name of task. */
		1000,     /* Stack size of task */
		NULL,      /* parameter of the task */
		5,         /* priority of the task */
		&Task1,    /* Task handle to keep track of created task */
		0);        /* pin task to core 0 */

	xTaskCreatePinnedToCore(
		Task2code, /* Task function. */
		"Task1",   /* name of task. */
		1000,     /* Stack size of task */
		NULL,      /* parameter of the task */
		5,         /* priority of the task */
		&Task2,    /* Task handle to keep track of created task */
		1);        /* pin task to core 0 */
}

void loop()
{
	lcd.fillScr(255, 255, 255);
	if (virtualPin1)
	{
		lcd.setColor(0, 0, 255);
		lcd.drawCircle(60, 65, 25, true);
	}
	else
	{
		lcd.setColor(0, 0, 0);
		lcd.drawCircle(60, 65, 25, true);
	}
	if (virtualPin2)
	{
		lcd.setColor(0, 255, 0);
		lcd.drawCircle(180, 65, 25, true);
	}
	else
	{
		lcd.setColor(0, 0, 0);
		lcd.drawCircle(180, 65, 25, true);
	}
	delay(50);
	lcd.flushFB();
}
