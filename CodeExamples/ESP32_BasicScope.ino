/*
 Name:		ESP32_BasicScope.ino
 Created:	06-Jul-21 11:55:42 AM
 Author:	giltal
*/

#include "graphics.h"
#include "PCF8574.h"

#define SCREEN_WIDTH		240                
#define SCREEN_HEIGHT		135
#define FONT_SIZE			9
#define VISUAL_WIN_Y_START	10
#define VISUAL_WIN_Y_END	(SCREEN_HEIGHT - 1)
#define ACTUAL_DISPLAY_HIGHT (VISUAL_WIN_Y_END - VISUAL_WIN_Y_START + 1)	 
#define VISUAL_WIN_X_START	40
#define VISUAL_WIN_X_END	(SCREEN_WIDTH - 1)
#define VISUAL_WIN_WIDTH	(VISUAL_WIN_X_END - VISUAL_WIN_X_START)
#define REC_LENGTH			(VISUAL_WIN_WIDTH + 1)

#define PROBE_PIN			12
#define NUM_OF_SUPPORTED_SAMPLE_WIN	5
unsigned int windowWidthTimeTable[NUM_OF_SUPPORTED_SAMPLE_WIN] = { 5,25,50,150,250 }; // ms

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
ST77XX_FB lcd;
PCF8574 pcf8574(0x21);
//
unsigned short displayDataBuffer[REC_LENGTH];

// the setup function runs once when you press reset or power the board
void setup() 
{
	pcf8574.begin();
	pcf8574.pinMode(1, INPUT);
	pcf8574.pinMode(2, INPUT);
	pcf8574.pinMode(3, INPUT);
	pinMode(PROBE_PIN, ANALOG);
	pinMode(25, OUTPUT);

	lcd.init();

	ledcSetup(1/*Channel*/, 2000/*Freq*/, 8/*Number of bits for resolution of duty cycle*/);
	ledcAttachPin(25, 1);
	ledcWriteTone(1/*Channel*/, 50/*Freq*/);

	ledcWrite(1/*Channel*/, 500 /*Duty Cycle*/);
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	unsigned int winWidthIndex = 0;
	char tempStr[10];
	lcd.fillScr(0, 0, 0);
	lcd.loadFonts(ORBITRON_LIGHT32);
	lcd.setColor(100, 100, 255);
	lcd.print("ESP32", 0, 10, true);
	lcd.print("Scope", 0, 50, true);
	lcd.flushFB();
	delay(2000);
	while (1)
	{
		displayBasicScreen();
		collectSamples(windowWidthTimeTable[winWidthIndex]);
		displaySamples();
		sprintf(tempStr, "%d ms", windowWidthTimeTable[winWidthIndex]);
		lcd.setColor(255, 150, 150);
		lcd.print(tempStr, 0, 4, true);
		lcd.flushFB();
		while (pcf8574.digitalRead(2) == HIGH);
		if (pcf8574.digitalRead(1) == HIGH)
		{
			while (pcf8574.digitalRead(1) == HIGH);
			winWidthIndex++;
			if (winWidthIndex == NUM_OF_SUPPORTED_SAMPLE_WIN)
			{
				winWidthIndex = 0;
			}
		}
	}
}

void displayBasicScreen()
{
	lcd.fillScr(0, 0, 0);
	lcd.setColor(255, 255, 255);
	lcd.loadFonts(SANS9);
	lcd.drawLine(VISUAL_WIN_X_START, VISUAL_WIN_Y_START, VISUAL_WIN_X_START, VISUAL_WIN_Y_END);
	lcd.drawLine(VISUAL_WIN_X_END, VISUAL_WIN_Y_START, VISUAL_WIN_X_END, VISUAL_WIN_Y_END);
	lcd.setColor(100, 100, 255);
	lcd.print("0V", 0, VISUAL_WIN_Y_END - FONT_SIZE);
	lcd.print("1.65", 0, VISUAL_WIN_Y_START + (VISUAL_WIN_Y_END - VISUAL_WIN_Y_START) / 2 - FONT_SIZE / 2);
	lcd.print("3.3V", 0, VISUAL_WIN_Y_START);
	int i;
	lcd.setColor(255, 255, 255);
	for (i = VISUAL_WIN_X_START + 1; i < VISUAL_WIN_X_END; i+=10)
	{
		lcd.drawLine(i, VISUAL_WIN_Y_START + (VISUAL_WIN_Y_END - VISUAL_WIN_Y_START) / 2, i + 5, VISUAL_WIN_Y_START + (VISUAL_WIN_Y_END - VISUAL_WIN_Y_START) / 2);
	}
	for (i = VISUAL_WIN_Y_START; i < VISUAL_WIN_Y_END; i+=10)
	{
		lcd.drawVLine(VISUAL_WIN_X_START + (VISUAL_WIN_WIDTH / 4) * 1, i, 5);
		lcd.drawVLine(VISUAL_WIN_X_START + (VISUAL_WIN_WIDTH / 4) * 2, i, 5);
		lcd.drawVLine(VISUAL_WIN_X_START + (VISUAL_WIN_WIDTH / 4) * 3, i, 5);
	}
	//lcd.flushFB();
}

void collectSamples(unsigned int windowWidthTime)
{
	// Calculate time gap between samples according to visiable window
	// ESP32 can perform analog read ~90400 per second  = 1.1 micro second
	unsigned int	timePeriod = 1000 / windowWidthTime, numberOfSamplesPerViewData = 90400/(timePeriod * REC_LENGTH), avarageSample;
	
	for (size_t i = 0; i < REC_LENGTH; i++)
	{
		avarageSample = 0;
		for (size_t j = 0; j < numberOfSamplesPerViewData; j++)
		{
			avarageSample += analogRead(PROBE_PIN); // Normalize data
		}
		avarageSample = (avarageSample / numberOfSamplesPerViewData);
		displayDataBuffer[i] = (VISUAL_WIN_Y_END - (avarageSample / (4096 / ACTUAL_DISPLAY_HIGHT)));//(VISUAL_WIN_Y_END - (analogRead(PROBE_PIN) / (4096/ ACTUAL_DISPLAY_HIGHT)))
	}
}

void displaySamples()
{
	lcd.setColor(100, 255, 100);
	//int currentY, prevSampleY = map(displayDataBuffer[0], 0, 4095, VISUAL_WIN_Y_END, VISUAL_WIN_Y_START);
	for (int i = 0; i < (REC_LENGTH - 1); i++)
	{
		// map(value, fromLow, fromHigh, toLow, toHigh)
		//currentY = map(displayDataBuffer[i], 0, 4095, VISUAL_WIN_Y_END, VISUAL_WIN_Y_START); // 
		lcd.drawLine(VISUAL_WIN_X_START + i, displayDataBuffer[i], VISUAL_WIN_X_START + i + 1, displayDataBuffer[i+1]);
	}
}