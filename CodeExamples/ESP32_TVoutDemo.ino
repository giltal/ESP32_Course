#include "graphics.h"

TVout tvOut;

void setup()
{
	tvOut.init();
}

void loop()
{
	unsigned long time;
	time = ESP.getCycleCount();
	tvOut.fillScr(0xff, 0xff, 0xff);
	tvOut.loadFonts(ORBITRON_LIGHT32);
	tvOut.setColor(0, 0, 0);
	tvOut.print("Driver Test...", 0, 100, true);
	tvOut.setColor(0xff, 0x00, 0x00);
	tvOut.drawCircle(50, 200, 20, true);
	tvOut.setColor(0x00, 0xff, 0x00);
	tvOut.drawCircle(100, 200, 20, true);
	tvOut.setColor(0x00, 0x00, 0xff);
	tvOut.drawCircle(150, 200, 20, true);
	printf("Time: %f\n", (ESP.getCycleCount() - time) / 240000000.0);
	while (1);
}