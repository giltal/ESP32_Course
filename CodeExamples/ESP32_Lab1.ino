/*
 Name:		ESP32_Temp.ino
 Created:	24-Jun-21 3:03:20 PM
 Author:	giltal
*/

// the setup function runs once when you press reset or power the board
void setup() 
{
	pinMode(25, OUTPUT);
	pinMode(32, ANALOG);
	ledcSetup(1/*Channel*/, 2000/*Freq*/, 8/*Number of bits for resolution of duty cycle*/);
	ledcAttachPin(25, 1);
	ledcWriteTone(1/*Channel*/, 2000/*Freq*/);
}

void pwmDemo();
void analogOutDemo();
void digitalOut();

// the loop function runs over and over again until power down or reset
void loop() 
{
	digitalOut();
	//pwmDemo();
	//analogOutDemo();
}

void digitalOut()
{
	digitalWrite(25, HIGH);
	delay(100);
	digitalWrite(25, LOW);
	delay(100);
}

void pwmDemo()
{
	int val = analogRead(32);

	ledcWrite(1/*Channel*/, val /*Duty Cycle*/);
}

void analogOutDemo()
{
	int val = analogRead(32);

	dacWrite(25, val/16);
}
