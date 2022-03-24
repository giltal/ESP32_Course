/*
 Name:		ESP32_DRV8833.ino
 Created:	22-Feb-22 8:26:53 AM
 Author:	giltal
*/
#include "graphics.h"
#include <Wiimote.h>
#include "Wire.h"
#include "PCF8574.h"
#include "DRV8833.h"
#include <WiFi.h>

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

#define BLYNK_PRINT Serial

#define nWIFI_BLYNK_DEMO
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
char auth[] = "i-r19ccru5l7wr0_ziAAfd8FKFlX5Kmb";

unsigned int virtualJoyX = 0, virtualJoyY = 0, readX = 0, readY = 0;

BLYNK_WRITE(V0)
{
	virtualJoyX = param[0].asInt();
	if (virtualJoyX > 511)
		readX = 0;
	else
		readX = virtualJoyX;
	virtualJoyY = param[1].asInt();
	if (virtualJoyY > 511)
		readY = 0;
	else
		readY = virtualJoyY;
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

PCF8574 pcf8574(0x21);

void drv8833sleep(bool on)
{
	if (on)
	{
		pcf8574.digitalWrite(P4, LOW);
	}
	else
	{
		pcf8574.digitalWrite(P4, HIGH);
	}
}

DRV8833 motors(33, 32, 2, 17, 1, 2, &drv8833sleep);

TaskHandle_t Task1;
unsigned int LmotorSpeed = 0, RmotorSpeed = 0;
motorDir	LmotorDir = FORWARD, RmotorDir = FORWARD;

void Task1code(void * pvParameters)
{
	motors.sleep(false);
	while (1)
	{
		/*for (size_t i = 0; i < 255; i += 5)
		{
			motors.driveMotor(MOTOR_A, FORWARD, i);
			motors.driveMotor(MOTOR_B, BACKWARD, i);
			delay(50);
		}
		delay(1000);
		for (size_t i = 0; i < 255; i += 5)
		{
			motors.driveMotor(MOTOR_A, BACKWARD, i);
			motors.driveMotor(MOTOR_B, FORWARD, i);
			delay(50);
		}*/
		motors.driveMotor(MOTOR_A, LmotorDir, LmotorSpeed);
		motors.driveMotor(MOTOR_B, RmotorDir, RmotorSpeed);
		delay(100);
	}
}

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(115200);
	pcf8574.begin();

	pcf8574.pinMode(P4, OUTPUT); // Motors Sleep

#ifdef WIFI_BLYNK_DEMO
	Blynk.begin(auth, ssid, pass);
#else
	Blynk.setDeviceName("Gil-Blynk");
	Blynk.begin(auth);
#endif	

	xTaskCreatePinnedToCore(
		Task1code, /* Task function. */
		"Task1",   /* name of task. */
		8000,     /* Stack size of task */
		NULL,      /* parameter of the task */
		5,         /* priority of the task */
		&Task1,    /* Task handle to keep track of created task */
		0);        /* pin task to core 0 */

	lcd.init(st7789_240x135x16_FB, &ttgoLCDaccessor, 16, 19, 18);
}

//unsigned int LmotorSpeed = 0, RmotorSpeed = 0;
//motorDir	LmotorDir = FORWARD, RmotorDir = FORWARD;

void loop() 
{
	short x, y;
	float power, powerSplit, lmPower, rmPower;
	lcd.loadFonts(ORBITRON_LIGHT24);
	lcd.setColor(0, 0, 255);
	char valStr[20];
	while (1)
	{
		Blynk.run();
		lcd.fillScr(255, 255, 255);
		//lcd.setColor(virtualPinR * 255, virtualPinG * 255, virtualPinB * 255);
		lcd.setColor(0, 0, 0);
		x = map(readX, 0, 511, -255, 255);
		y = map(readY, 0, 511, -255, 255);
		sprintf(valStr,"X: %d", x);
		lcd.print(valStr, 0, 20, true);
		sprintf(valStr, "Y: %d", y);
		lcd.print(valStr, 0, 90, true);
		power = sqrt(x * x + y * y);
		if (power > 255)
		{
			power = 0;
		}
		if (y <= 0)
		{
			LmotorDir = FORWARD, RmotorDir = FORWARD;
			if (y != 0)
			{
				printf("F:");
			}
		}
		else
		{
			LmotorDir = BACKWARD, RmotorDir = BACKWARD;
			y = -y;
			printf("B:");
		}
		if (x >= 0)
		{
			powerSplit = (255.0 - x) / (255.0 + x);
			LmotorSpeed = power;
			rmPower = (power * powerSplit);
			RmotorSpeed = rmPower;
		}
		else
		{
			powerSplit = (255.0 + x) / (255.0 - x);
			RmotorSpeed = power;
			lmPower = (power * powerSplit) ;
			LmotorSpeed = lmPower;
		}
		if (!(LmotorSpeed == 0 && RmotorSpeed == 0))
		{
			printf("LM:%d RM:%d ", LmotorSpeed, RmotorSpeed);
			printf("Speed: %f Power Split: %f %d %d\n", power, powerSplit, map(LmotorSpeed, 0, 255, 67, 134), map(LmotorSpeed, 0, 255, 67, 0));
			if (LmotorDir == BACKWARD)
			{
				lcd.setColor(255, 0, 0);
				lcd.drawRect(10, 67, 50, map(LmotorSpeed, 0, 255, 67, 134), true);
				lcd.setColor(0, 0, 255);
				lcd.drawRect(190, 67, 230, map(RmotorSpeed, 0, 255, 67, 134), true);
			}
			else
			{
				lcd.setColor(255, 0, 0);
				lcd.drawRect(10, 67, 50, map(LmotorSpeed, 0, 255, 67, 0), true);
				lcd.setColor(0, 0, 255);
				lcd.drawRect(190, 67, 230, map(RmotorSpeed, 0, 255, 67, 0), true);
			}
		}
		lcd.flushFrameBuffer();
	}
}
