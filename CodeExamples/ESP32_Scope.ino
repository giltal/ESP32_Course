/*
 Name:		ESP32_Scope.ino
 Created:	28-Jun-21 7:57:34 AM
 Author:	giltal
*/

#include <Wire.h>
#include <EEPROM.h>
#include "graphics.h"
#include "PCF8574.h"

#define SCREEN_WIDTH 240                // OLED display width
#define SCREEN_HEIGHT 135               // OLED display height
#define REC_LENGTH 240                  // 

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
ST77XX_FB display;
PCF8574 pcf8574(0x21);
//
const char vRangeName[10][5] = { "A50V", "A 5V", " 50V", " 20V", " 10V", "  5V", "  2V", "  1V", "0.5V", "0.2V" }; // \0
const char * const vstring_table[] = { vRangeName[0], vRangeName[1], vRangeName[2], vRangeName[3], vRangeName[4], vRangeName[5], vRangeName[6], vRangeName[7], vRangeName[8], vRangeName[9] };
const char hRangeName[8][6] = { " 50ms", " 20ms", " 10ms", "  5ms", "  2ms", "  1ms", "500us", "200us" };          // (48
const char * const hstring_table[] = { hRangeName[0], hRangeName[1], hRangeName[2], hRangeName[3], hRangeName[4], hRangeName[5], hRangeName[6], hRangeName[7] };

int waveBuff[REC_LENGTH];      //  (RAM)
char chrBuff[10];              // 
String hScale = "xxxAs";
String vScale = "xxxx";

float lsb5V = 0.0055549;       // 5V0.005371 V/1LSB
float lsb50V = 0.051513;       // 50V 0.05371

volatile int vRange = 5;           //   0:A50V, 1:A 5V, 2:50V, 3:20V, 4:10V, 5:5V, 6:2V, 7:1V, 8:0.5V
volatile int hRange = 6;           // 0:50m, 1:20m, 2:10m, 3:5m, 4;2m, 5:1m, 6:500u, 7;200u
volatile int trigD = 1;            // 0:1:
volatile int scopeP = 1;           //  0:, 1:, 2:
volatile boolean hold = false; // 
volatile boolean paraChanged = false; //  true
volatile int saveTimer;        // EEPROM
int timeExec;                  // (ms)

int dataMin;                   // (min:0)
int dataMax;                   // (max:1023)
int dataAve;                   // 10 max:10230)
int rangeMax = 4095;                  // 
int rangeMin = 0;                  // 
int rangeMaxDisp;              // max100
int rangeMinDisp;              // min
int trigP;                     // 
boolean trigSync;              // 
int att10x;                    // 1

#define PROBE_PIN 12

void setup()
{
	pcf8574.begin();
	pcf8574.pinMode(1, INPUT);
	pcf8574.pinMode(2, INPUT);
	pcf8574.pinMode(3, INPUT);
	pinMode(PROBE_PIN, ANALOG);
	//pinMode(39, INPUT); // pcf8574 interrupt
	pinMode(25, OUTPUT);

	display.init();

	//loadEEPROM();                             // EEPROM
	//attachInterrupt(39, pin2IRQ, FALLING);     // 
	startScreen();
	ledcSetup(1/*Channel*/, 2000/*Freq*/, 8/*Number of bits for resolution of duty cycle*/);
	ledcAttachPin(25, 1);
	ledcWriteTone(1/*Channel*/, 2000/*Freq*/);

	ledcWrite(1/*Channel*/, 500 /*Duty Cycle*/);
	//dacWrite(25, 255);
}

void loop()
{
	//dacWrite(25, 10);
	setConditions();                          // RAM40
	readWave();                               //  (1.6ms )
	//digitalWrite(13, LOW);                    //
	dataAnalize();                            // (0.4-0.7ms)
	writeCommonImage();                       // (4.6ms)
	plotData();                               // (5.4ms+)
	dispInf();                                // (6.2ms)
	display.flushFB();                        // (37ms)
	saveEEPROM();                             // EEPROM
	while (hold == true) {                    // Hold
		dispHold();
		delay(10);
	}
	/*if (pcf8574.digitalRead(3) == HIGH)
	{           // DOWN
		while (pcf8574.digitalRead(3) == HIGH);
		hRange--;
		if (hRange < 0)
		{
			hRange = 0;
		}
	}
	if (pcf8574.digitalRead(1) == HIGH)
	{           // UP
		while (pcf8574.digitalRead(1) == HIGH);
		hRange++;
		if (hRange > 7)
		{
			hRange = 7;
		}
	}*/
}

void setConditions()
{
	hScale = hstring_table[hRange]; // hScale
	vScale = vstring_table[vRange];                                                   // vScale

	switch (vRange)
	{              // 
	case 0: {                    // Auto50V
		//        rangeMax = 1023;
		//        rangeMin = 0;
		att10x = 1;              // 
		break;
	}
	case 1: {                    // Auto 5V
		//        rangeMax = 1023;
		//        rangeMin = 0;
		att10x = 0;              // 
		break;
	}
	case 2: {                    // 50V
		rangeMax = 50 / lsb50V;  // 
		rangeMaxDisp = 5000;     // 100
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 1;              // 
		break;
	}
	case 3: {                    // 20V
		rangeMax = 20 / lsb50V;  // 
		rangeMaxDisp = 2000;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 1;              // 
		break;
	}
	case 4: {                    // 10V
		rangeMax = 10 / lsb50V;  // 
		rangeMaxDisp = 1000;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 1;              // 
		break;
	}
	case 5: {                    // 5V
		rangeMax = 5 / lsb5V;
		rangeMaxDisp = 500;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 0;
		break;
	}
	case 6: {                    // 2V
		rangeMax = 2 / lsb5V;    // 
		rangeMaxDisp = 200;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 0;              // 
		break;
	}
	case 7: {                    // 1V
		rangeMax = 1 / lsb5V;    // 
		rangeMaxDisp = 100;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 0;              // 
		break;
	}
	case 8: {                    // 0.5V
		rangeMax = 0.5 / lsb5V;  // 
		rangeMaxDisp = 50;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 0;              // 
		break;
	}
	case 9: {                    // 0.5V
		rangeMax = 0.2 / lsb5V;  // 
		rangeMaxDisp = 20;
		rangeMin = 0;
		rangeMinDisp = 0;
		att10x = 0;              // 
		break;
	}
	}
}

void writeCommonImage()
{     // 
	display.fillScr(0, 0, 0);                   // (0.4ms)
	display.setColor(255, 255, 255);
	//display.print("av  V", 86 * 1.875, 0);
	display.drawVLine(26 * 1.875, 9 * 2, 55 * 2);
	display.drawVLine(127 * 1.875, 9 * 2, 55 * 2);

	display.drawHLine(24 * 1.875, 9 * 2, 7 * 1.875);   // Max
	display.drawHLine(24 * 1.875, 36 * 2, 2 * 1.875);  //
	display.drawHLine(24 * 1.875, 63 * 2, 7 * 1.875);  //

	display.drawHLine(51 * 1.875, 9 * 2, 3 * 1.875);   // Max
	display.drawHLine(51 * 1.875, 63 * 2, 3 * 1.875);  //

	display.drawHLine(76 * 1.875, 9 * 2, 3 * 1.875);   // Max
	display.drawHLine(76 * 1.875, 63 * 2, 3 * 1.875);  //

	display.drawHLine(101 * 1.875, 9 * 2, 3 * 1.875);  // Max
	display.drawHLine(101 * 1.875, 63 * 2, 3 * 1.875); //

	display.drawHLine(123 * 1.875, 9 * 2, 5 * 1.875);  // Max
	display.drawHLine(123 * 1.875, 63 * 2, 5 * 1.875); // 

	for (int x = 26; x <= 128; x += 5) {
		display.drawHLine(x * 1.875, 36 * 2, 2 * 1.875); // ()
	}
	for (int x = (127 - 25); x > 30; x -= 25) {
		for (int y = 10; y < 63; y += 5) {
			display.drawVLine(x * 1.875, y * 2, 2 * 2); // 3
		}
	}
}

void readWave()
{                            // 
	/*if (att10x == 1)
	{                         // 1/10
		pinMode(12, OUTPUT);                     //
		digitalWrite(12, LOW);                   // LOW
	}
	else
	{                                   //
		pinMode(12, INPUT);                      // Hi-z
	}*/

	switch (hRange)
	{                          // 

	case 0: {                                // 50ms
		timeExec = 400 + 50;                 // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x07;              // 128 (arduino
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 112s
			delayMicroseconds(1888);           // 
		}
		break;
	}

	case 1: {                                // 20ms
		timeExec = 160 + 50;                 // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x07;              // 128 (arduino
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 112s
			delayMicroseconds(688);            // 
		}
		break;
	}

	case 2: {                                // 10 ms
		timeExec = 80 + 50;                  // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x07;              // 128 (arduino
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 112s
			delayMicroseconds(288);            // 
		}
		break;
	}

	case 3: {                                // 5 ms
		timeExec = 40 + 50;                  // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x07;              // 128 (arduino
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 112s
			delayMicroseconds(88);             // 
		}
		break;
	}

	case 4: {                                // 2 ms
		timeExec = 16 + 50;                  // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x06;              // 64 (0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 56s
			delayMicroseconds(24);             // 
		}
		break;
	}

	case 5: {                                // 1 ms
		timeExec = 8 + 50;                   // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x05;              // 16 (0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 28s
			//delayMicroseconds(12);             // 
		}
		break;
	}

	case 6: {                                // 500us
		timeExec = 4 + 50;                   // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x04;              // 16(0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
		for (int i = 0; i < REC_LENGTH; i++) {     // 200
			waveBuff[i] = analogRead(PROBE_PIN);       // 16s
			delayMicroseconds(4);              // 
			// 1.875snop 110.0625s @16MHz)
			asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
			asm("nop"); asm("nop"); asm("nop");
		}
		break;
	}

	case 7: {                                // 200us
		timeExec = 2 + 50;                   // (ms) EEPROM
		//ADCSRA = ADCSRA & 0xf8;              // 3
		//ADCSRA = ADCSRA | 0x02;              // :4(0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
		for (int i = 0; i < REC_LENGTH; i++) {
			waveBuff[i] = analogRead(PROBE_PIN);       // 6s
			// 1.875snop 110.0625s @16MHz)
			asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
			asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
			asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
		}
		break;
	}
	}
}

void dataAnalize() {                   // 
	int d;
	long sum = 0;

	// 
	dataMin = 4095;                         // 
	dataMax = 0;                            // 
	for (int i = 0; i < REC_LENGTH; i++) {  // 
		d = waveBuff[i];
		sum = sum + d;
		if (d < dataMin) {                    // 
			dataMin = d;
		}
		if (d > dataMax) {                    // 
			dataMax = d;
		}
	}

	// 
	dataAve = (sum + 10) / 20;               // 10

	// max,min
	if (vRange <= 1) {                       // Auto1
		rangeMin = dataMin - 20;               // -20
		rangeMin = (rangeMin / 10) * 10;       // 10
		if (rangeMin < 0) {
			rangeMin = 0;                        // 0
		}
		rangeMax = dataMax + 20;               // +20
		rangeMax = ((rangeMax / 10) + 1) * 10; // 10
		if (rangeMax > 4090) {
			rangeMax = 4095;                     // 10201023
		}

		if (att10x == 1) {                            // 
			rangeMaxDisp = 100 * (rangeMax * lsb50V);   // ADC
			rangeMinDisp = 100 * (rangeMin * lsb50V);   // 
		}
		else {                                      // 
			rangeMaxDisp = 100 * (rangeMax * lsb5V);
			rangeMinDisp = 100 * (rangeMin * lsb5V);
		}
	}
	else {                                   // 
   // 
	}

	// 
	for (trigP = ((REC_LENGTH / 2) - 51); trigP < ((REC_LENGTH / 2) + 50); trigP++) { // 
		if (trigD == 0) {                        // 0
			if ((waveBuff[trigP - 1] < (dataMax + dataMin) / 2) && (waveBuff[trigP] >= (dataMax + dataMin) / 2)) {
				break;                              // 
			}
		}
		else {                                // 0
			if ((waveBuff[trigP - 1] > (dataMax + dataMin) / 2) && (waveBuff[trigP] <= (dataMax + dataMin) / 2)) {
				break;
			}                                    // 
		}
	}
	trigSync = true;
	if (trigP >= ((REC_LENGTH / 2) + 50)) {  // 
		trigP = (REC_LENGTH / 2);
		trigSync = false;                      // Unsync
	}
}

void startScreen()
{
	display.fillScr(0, 0, 0);
	display.loadFonts(ORBITRON_LIGHT24);
	display.setColor(255, 100, 100);
	display.print("Intel Makers", 0, 25, true);
	display.print("Pen Scope", 0, 60, true);
	display.flushFB();
	delay(1000);
}

void dispHold()
{                            // Hold
	display.setColor(0, 0, 0);
	display.drawRect(32, 12 * 2, 24, 8 * 2, true);    // 4
	//display.setCursor(32, 12);
	display.setColor(0, 255, 0);
	display.print("Hold", 32, 12 * 2);                  // Hold 
	display.flushFB();                         //
}

void dispInf()
{                             // 
	float voltage;
	// 
	//display.setCursor(2, 0);                   // 
	//display.print(vScale);                     //
	display.loadFonts(SANS9);
	display.print((char *)vScale.c_str(), 2 * 1.875, 0);
	if (scopeP == 0)
	{                         // 
		display.drawHLine(0 * 1.875, 7 * 2, 27 * 1.875);  // 
		display.drawVLine(0 * 1.875, 5 * 2, 2 * 2);
		display.drawVLine(26 * 1.875, 5 * 2, 2 * 2);
	}

	// 
	//display.setCursor(34, 0);                  //
	//display.print(hScale);                     // (time/div)
	display.print((char *)hScale.c_str(), 34 * 1.875, 0);
	if (scopeP == 1) {                         // 
		display.drawHLine(32 * 1.875, 7 * 2, 33 * 1.875); // 
		display.drawVLine(32 * 1.875, 5 * 2, 2 * 2);
		display.drawVLine(64 * 1.875, 5 * 2, 2 * 2);
	}

	// 
	//display.setCursor(75, 0);                  // 
	/*if (trigD == 0) {
		display.print(char(0x18));               //
	}
	else {
		display.print(char(0x19));               //
	}*/
	if (scopeP == 2) {      // 
		display.drawHLine(71 * 1.875, 7 * 2, 13 * 1.875); // 
		display.drawVLine(71 * 1.875, 5 * 2, 2 * 2);
		display.drawVLine(83 * 1.875, 5 * 2, 2 * 2);
	}

	// 
	if (att10x == 1) {                         // 10
		voltage = dataAve * lsb50V / 10.0;       // 50V
	}
	else {
		voltage = dataAve * lsb5V / 10.0;        // 5V
	}
	dtostrf(voltage, 4, 2, chrBuff);           // x.xx 
	//display.setCursor(98, 0);                  // 
	display.print(chrBuff, 98 * 1.875, 0 * 2);                    // 
	//  display.print(saveTimer);                  // 

	// 
	voltage = rangeMaxDisp / 100.0;            // Max
	if (vRange == 1 || vRange > 4) {           // 5VAuto5V
		dtostrf(voltage, 4, 2, chrBuff);         //  *.** 
	}
	else {                                   //
		dtostrf(voltage, 4, 1, chrBuff);         // **.* 
	}
	//display.setCursor(0, 9);
	display.print(chrBuff, 0 * 1.875, 9 * 2);                    // Max

	voltage = (rangeMaxDisp + rangeMinDisp) / 200.0; // 
	if (vRange == 1 || vRange > 4) {           // 5VAuto5V
		dtostrf(voltage, 4, 2, chrBuff);         // 2
	}
	else {                                   //
		dtostrf(voltage, 4, 1, chrBuff);         // 1
	}
	//display.setCursor(0, 33);
	display.print(chrBuff, 0 * 1.875, 33 * 2);                    // 

	voltage = rangeMinDisp / 100.0;            // Min
	if (vRange == 1 || vRange > 4) {           // 5VAuto5V
		dtostrf(voltage, 4, 2, chrBuff);         // 2
	}
	else {
		dtostrf(voltage, 4, 1, chrBuff);         // 1
	}
	//display.setCursor(0, 57);
	display.print(chrBuff, 0 * 1.875, 57 * 2);                    // Min

	// 
	if (trigSync == false) {                   // 
		//display.setCursor(60, 55);               // 
		display.print("Unsync", 60 * 1.875, 55 * 2);              // Unsync 
	}
}

void plotData() {                    // 
	long y1, y2;
	display.setColor(100, 255, 100);
	for (int x = 0; x <= 98 * 2; x++)
	{
		// map(value, fromLow, fromHigh, toLow, toHigh)
		//y1 = map(waveBuff[x + trigP - 50], rangeMin, rangeMax, 63 * 1.875, 9 * 2); // 
		y1 = map(waveBuff[x + trigP - 50], 0, 4095, 63 * 2, 9 * 2); // 
		//y1 = constrain(y1*2, 9*2, 63*2);                                     // 
		//y2 = map(waveBuff[x + trigP - 49], rangeMin, rangeMax, 63 * 1.875, 9 * 2); //
		y2 = map(waveBuff[x + trigP - 49], 0, 4095, 63 * 2, 9 * 2); //
		//y2 = constrain(y2*2, 9*2, 63*2);                                     //
		display.drawLine((x + 27) * 1.875, y1, (x + 28) * 1.875, y2);               // 
	}
	display.setColor(255, 255, 255);
}

void saveEEPROM() {                    // EEPROM
	if (paraChanged == true) {           // 
		saveTimer = saveTimer - timeExec;  // 
		if (saveTimer < 0) {               // 
			paraChanged = false;             // 
			EEPROM.write(0, vRange);        // 
			EEPROM.write(1, hRange);
			EEPROM.write(2, trigD);
			EEPROM.write(3, scopeP);
		}
	}
}

void loadEEPROM() {                // EEPROM
	int x;
	x = EEPROM.read(0);             // vRange
	if ((x < 0) || (x > 9)) {        // 0-9
		x = 3;                         // 
	}
	vRange = x;

	x = EEPROM.read(1);             // hRange
	if ((x < 0) || (x > 7)) {        // 0-9
		x = 3;                         // 
	}
	hRange = x;
	x = EEPROM.read(2);             // trigD
	if ((x < 0) || (x > 1)) {        // 0-9
		x = 1;                         // 
	}
	trigD = x;
	x = EEPROM.read(3);             // scopeP
	if ((x < 0) || (x > 2)) {        // 0-9
		x = 1;                         // 
	}
	scopeP = x;
}

void pin2IRQ()
{
	/*if (pcf8574.readDigital(2) == HIGH)
	{       // 3High
		saveTimer = 5000;              // EEPROM(ms
		paraChanged = true;            // ON
	}*/

	/*if ((x & 0x01) == 0)
	{
		scopeP++;
		if (scopeP > 2) {
			scopeP = 0;
		}
	}*/

	if (pcf8574.digitalRead(1) == HIGH) {           // UP
		if (scopeP == 0) {             // 
			vRange++;
			if (vRange > 9) {
				vRange = 9;
			}
		}
		if (scopeP == 1) {             // 
			hRange++;
			if (hRange > 7) {
				hRange = 7;
			}
		}
		if (scopeP == 2) {             // 
			trigD = 0;                   // 
		}
	}

	if (pcf8574.digitalRead(3) == HIGH) {           // DOWN
		if (scopeP == 0) {             // 
			vRange--;
			if (vRange < 0) {
				vRange = 0;
			}
		}
		if (scopeP == 1) {             // 
			hRange--;
			if (hRange < 0) {
				hRange = 0;
			}
		}
		if (scopeP == 2) {             // 
			trigD = 1;                   // 
		}
	}

	if (pcf8574.digitalRead(2) == HIGH) {           // HOLD
		hold = !hold;                 // 
	}
}
