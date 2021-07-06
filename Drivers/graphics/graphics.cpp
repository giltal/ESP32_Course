#include "graphics.h"
#include "SPI.h"
#include "Fonts/fonts.h"

#define ESP32

/*
 General graphics lib to be inherited
*/

graphics::graphics(short maxX, short maxY)
{	
	this->maxX = maxX;
	this->maxY = maxY;
	// Load default fonts
	fontTypeLoaded = ORBITRON_LIGHT24;
	currentFonts = &orbitronLight24Font;
}

void graphics::drawRect(short x1, short y1, short x2, short y2, bool fill)
{
	if (x1 > x2)
	{
		swap(&x1, &x2);
	}
	if (y1 > y2)
	{
		swap(&y1, &y2);
	}

	if (fill)
	{
		for (int i = 0; i < ((y2 - y1) / 2) + 1; i++)
		{
			drawHLine(x1, y1 + i, x2 - x1);
			drawHLine(x1, y2 - i, x2 - x1);
		}
	}
	else
	{
		drawHLine(x1, y1, x2 - x1);
		drawHLine(x1, y2, x2 - x1);
		drawVLine(x1, y1, y2 - y1);
		drawVLine(x2, y1, y2 - y1);
	}
}

void graphics::drawRoundRect(short x1, short y1, short x2, short y2, bool fill)
{
	if (x1 > x2)
	{
		swap(&x1, &x2);
	}
	if (y1 > y2)
	{
		swap(&y1, &y2);
	}
	if ((x2 - x1) > 4 && (y2 - y1) > 4)
	{
		if (fill)
		{
			for (int i = 0; i < ((y2 - y1) / 2) + 1; i++)
			{
				switch (i)
				{
				case 0:
					drawHLine(x1 + 2, y1 + i, x2 - x1 - 4);
					drawHLine(x1 + 2, y2 - i, x2 - x1 - 4);
					break;
				case 1:
					drawHLine(x1 + 1, y1 + i, x2 - x1 - 2);
					drawHLine(x1 + 1, y2 - i, x2 - x1 - 2);
					break;
				default:
					drawHLine(x1, y1 + i, x2 - x1);
					drawHLine(x1, y2 - i, x2 - x1);
				}
			}
		}
		else
		{
			drawPixel(x1 + 1, y1 + 1);
			drawPixel(x2 - 1, y1 + 1);
			drawPixel(x1 + 1, y2 - 1);
			drawPixel(x2 - 1, y2 - 1);
			drawHLine(x1 + 2, y1, x2 - x1 - 4);
			drawHLine(x1 + 2, y2, x2 - x1 - 4);
			drawVLine(x1, y1 + 2, y2 - y1 - 4);
			drawVLine(x2, y1 + 2, y2 - y1 - 4);
		}
	}
}

void graphics::drawCircle(short x, short y, int radius, bool fill)
{
	if (fill)
	{
		for (int y1 = -radius; y1 <= 0; y1++)
			for (int x1 = -radius; x1 <= 0; x1++)
				if (x1*x1 + y1 * y1 <= radius * radius)
				{
					drawHLine(x + x1, y + y1, 2 * (-x1));
					drawHLine(x + x1, y - y1, 2 * (-x1));
					break;
				}
	}
	else
	{
		int f = 1 - radius;
		int ddF_x = 1;
		int ddF_y = -2 * radius;
		int x1 = 0;
		int y1 = radius;

		drawPixel(x, y + radius);
		drawPixel(x, y - radius);
		drawPixel(x + radius, y);
		drawPixel(x - radius, y);

		while (x1 < y1)
		{
			if (f >= 0)
			{
				y1--;
				ddF_y += 2;
				f += ddF_y;
			}
			x1++;
			ddF_x += 2;
			f += ddF_x;
			drawPixel(x + x1, y + y1);
			drawPixel(x - x1, y + y1);
			drawPixel(x + x1, y - y1);
			drawPixel(x - x1, y - y1);
			drawPixel(x + y1, y + x1);
			drawPixel(x - y1, y + x1);
			drawPixel(x + y1, y - x1);
			drawPixel(x - y1, y - x1);
		}
	}
}

void graphics::drawTriangle(short x0, short y0, short x1, short y1, short x2, short y2, bool fill)
{
	if (!fill)
	{
		drawLine(x0, y0, x1, y1);
		drawLine(x1, y1, x2, y2);
		drawLine(x2, y2, x0, y0);
	}
	else
	{
		short a, b, y, last;

		// Sort coordinates by Y order (y2 >= y1 >= y0)
		if (y0 > y1) {
			swap(&y0, &y1);
			swap(&x0, &x1);
		}
		if (y1 > y2) {
			swap(&y2, &y1);
			swap(&x2, &x1);
		}
		if (y0 > y1) {
			swap(&y0, &y1);
			swap(&x0, &x1);
		}

		if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
			a = b = x0;
			if (x1 < a)
				a = x1;
			else if (x1 > b)
				b = x1;
			if (x2 < a)
				a = x2;
			else if (x2 > b)
				b = x2;
			drawHLine(a, y0, b - a + 1);
			return;
		}

		short	dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
		int		sa = 0, sb = 0;

		if (y1 == y2)
			last = y1; // Include y1 scanline
		else
			last = y1 - 1; // Skip it

		for (y = y0; y <= last; y++) 
		{
			a = x0 + sa / dy01;
			b = x0 + sb / dy02;
			sa += dx01;
			sb += dx02;

			if (a > b)
				swap(&a, &b);
			drawHLine(a, y, b - a + 1);
		}

		// For lower part of triangle, find scanline crossings for segments
		// 0-2 and 1-2.  This loop is skipped if y1=y2.
		sa = (int)dx12 * (y - y1);
		sb = (int)dx02 * (y - y0);
		for (; y <= y2; y++) 
		{
			a = x1 + sa / dy12;
			b = x0 + sb / dy02;
			sa += dx12;
			sb += dx02;

			if (a > b)
				swap(&a, &b);
			drawHLine(a, y, b - a + 1);
		}
	}
}

void graphics::swap(short * a, short * b)
{
	short c;
	c = *b;
	*b = *a;
	*a = c;
}

void graphics::drawLine(short x1, short y1, short x2, short y2)
{
	if (y1 == y2)
	{
		if (x1 > x2)
			drawHLine(x2, y1, abs(x2 - x1) + 1);
		else
			drawHLine(x1, y1, abs(x2 - x1) + 1);
	}
	else if (x1 == x2)
	{
		if (y1 > y2)
			drawVLine(x1, y2, abs(y2 - y1) + 1);
		else
			drawVLine(x1, y1, abs(y2 - y1) + 1);
	}
	else
	{
		unsigned int	dx = (x2 > x1 ? x2 - x1 : x1 - x2);
		short			xstep = x2 > x1 ? 1 : -1;
		unsigned int	dy = (y2 > y1 ? y2 - y1 : y1 - y2);
		short			ystep = y2 > y1 ? 1 : -1;
		int				col = x1, row = y1;

		if (dx < dy)
		{
			int t = -(dy >> 1);
			while (true)
			{
				drawPixel(col, row);
				if (row == y2)
					return;
				row += ystep;
				t += dx;
				if (t >= 0)
				{
					col += xstep;
					t -= dy;
				}
			}
		}
		else
		{
			int t = -(dx >> 1);
			while (true)
			{
				drawPixel(col, row);
				if (col == x2)
					return;
				col += xstep;
				t += dy;
				if (t >= 0)
				{
					row += ystep;
					t -= dx;
				}
			}
		}
	}
}

void graphics::drawHLine(short x, short y, int l)
{
	for (size_t i = 0; i < l; i++)
	{
		drawPixel(x + i, y);
	}
}

void graphics::drawVLine(short x, short y, int l)
{
	for (size_t i = 0; i < l; i++)
	{
		drawPixel(x, y + i);
	}
}

void graphics::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	fgColor[2] = b;
	fgColor[1] = g;
	fgColor[0] = r;
	_fgColor = ((int)b << 16) | ((int)g << 8) | r;
	fg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}

unsigned short graphics::rgbTo565(unsigned char r, unsigned char g, unsigned char b)
{
	return (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}

void graphics::setBackColor(unsigned char r, unsigned char g, unsigned char b)
{
	bgColor[2] = b;
	bgColor[1] = g;
	bgColor[0] = r;
	_bgColor = ((int)b << 16) | ((int)g << 8) | r;
	bg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}

void graphics::print(char * string, short x, short y,bool center)
{
	unsigned int strLen = strlen(string);
	short currentX = x, currentY, h, dataLength,dataIndex = 0,dataOffset, dataCounter = 0, widthCounter = 0, tempX;
	unsigned char currentChar, charWidth, charHeight, data;
	if (center)
	{
		unsigned short strWidth = getPrintWidth(string);
		if (strWidth < maxX)
		{
			currentX = (maxX - strWidth) / 2;
		}
	}
	for (size_t i = 0; i < strLen; i++)
	{
		if (string[i] < currentFonts->first || string[i] > currentFonts->last)
		{
			currentX += currentFonts->fontsInfoArray[0].xAdvance; // Space
		}
		else
		{
			currentChar = string[i] - currentFonts->first;
			
			charWidth = currentFonts->fontsInfoArray[currentChar].width;
			charHeight = currentFonts->fontsInfoArray[currentChar].height;
			currentY = y + currentFonts->fontHight + currentFonts->fontsInfoArray[currentChar].yOffset;
			dataLength = charWidth * charHeight;
			dataOffset = currentFonts->fontsInfoArray[currentChar].dataOffset;

			dataIndex = 0;
			widthCounter = 0;
			dataCounter = 0;
			data = currentFonts->fontsData[dataOffset + dataIndex];
			for (h = 0; h < dataLength; h++)
			{
				if (data & 0x80)
				{
					tempX = currentX + widthCounter + currentFonts->fontsInfoArray[currentChar].xOffset;
					if (!(tempX >= maxX || tempX < 0 || currentY < 0 || currentY >= maxY))
					{
						drawPixel(tempX, currentY);
					}
				}
				data = data << 1;
				if (widthCounter == charWidth - 1)
				{
					currentY++;
					widthCounter = 0;
				}
				else
					widthCounter++;
				if (dataCounter == 7)
				{
					dataIndex++;
					data = currentFonts->fontsData[dataOffset + dataIndex];
					dataCounter = 0;
				}
				else
					dataCounter++;
			}
			currentX += currentFonts->fontsInfoArray[currentChar].xAdvance;
		}
	}
}

void graphics::loadFonts(fontType fontsToLoad)
{
	switch (fontsToLoad)
	{
	case ORBITRON_LIGHT24:
		currentFonts = &orbitronLight24Font;
		break;
	case ORBITRON_LIGHT32:
		currentFonts = &orbitronLight32Font;
		break;
	case MONO_BOLD18:
		currentFonts = &monoBold18Font;
		break;
	case OBLIQUE18:
		currentFonts = &oblique18Font;
		break;
	case SANS_OBLIQUE56:
		currentFonts = &sansOblique56Font;
		break;
	case SANS9:
		currentFonts = &sans9Font;
		break;
	case PICO6:
		currentFonts = &pico6Font;
		break;
	default:
		currentFonts = &orbitronLight24Font;
		break;
	}
}
short graphics::getFontHieght()
{
	return currentFonts->fontHight;

}
short graphics::getPrintWidth(char * string)
{
	unsigned int strLen = strlen(string);
	short w = 0;
	for (size_t i = 0; i < strLen; i++)
	{
		if (string[i] < currentFonts->first || string[i] > currentFonts->last)
		{
			w += currentFonts->fontsInfoArray[0].xAdvance; // Space
		}
		else
		{
			w += currentFonts->fontsInfoArray[string[i] - currentFonts->first].xAdvance;
		}
	}
	return w;
}

//////////////////////// ILI9488 9 bit parallel class /////////////////////////////////

// IO 0 is D/C
// IO 2 is WR/CLK
#define ILI9488P_WR_CMD(X)\
		ESP_WRITE_REG(0x3FF4400c, 0x000CF035);\ 
		ESP_WRITE_REG(0x3FF44008, (0x000C0000 & ((int)(X) << 12)) | (0x0000F000 & ((int)(X) << 10)) | (0x00000030 & ((int)(X) << 4)) | 0x4);\
		ESP_WRITE_REG(0x3FF44008, 0x1); 

#define ILI9488P_WR_DATA8(X)\
		ESP_WRITE_REG(0x3FF4400c, 0x000CF034);\ 
		ESP_WRITE_REG(0x3FF44008, (0x000C0000 & ((int)(X) << 12)) | (0x0000F000 & ((int)(X) << 10)) | (0x00000030 & ((int)(X) << 4)));\
		ESP_WRITE_REG(0x3FF44008, 0x04); 

#define ILI9488_RESET_IO	32

void ILI9488_9BIT_PARALLEL::init(bool _9bit)
{
	pinMode(0, OUTPUT); // D/C
	pinMode(2, OUTPUT); // WR/CLK
	// Data 0-7
	pinMode(4, OUTPUT); // D0
	pinMode(5, OUTPUT); // 1
	pinMode(12, OUTPUT);// 2
	pinMode(13, OUTPUT);// 3
	pinMode(14, OUTPUT);// 4
	pinMode(15, OUTPUT);// 5
	pinMode(18, OUTPUT);// 6
	pinMode(19, OUTPUT);// 7
	if (_9bit)
	{
		pinMode(25, OUTPUT); // D8
		_9bitFlag = true;
	}
	else
	{
		_9bitFlag = false;
	}

	ILI9488SPI_PRE_INIT();
	ILI9488SPI_RESET();
	ILI9488SPI_ASSERT_CS();

	ILI9488P_WR_CMD(0xE0);
	ILI9488P_WR_DATA8(0x00);
	ILI9488P_WR_DATA8(0x03);
	ILI9488P_WR_DATA8(0x09);
	ILI9488P_WR_DATA8(0x08);
	ILI9488P_WR_DATA8(0x16);
	ILI9488P_WR_DATA8(0x0A);
	ILI9488P_WR_DATA8(0x3F);
	ILI9488P_WR_DATA8(0x78);
	ILI9488P_WR_DATA8(0x4C);
	ILI9488P_WR_DATA8(0x09);
	ILI9488P_WR_DATA8(0x0A);
	ILI9488P_WR_DATA8(0x08);
	ILI9488P_WR_DATA8(0x16);
	ILI9488P_WR_DATA8(0x1A);
	ILI9488P_WR_DATA8(0x0F);

	ILI9488P_WR_CMD(0XE1);
	ILI9488P_WR_DATA8(0x00);
	ILI9488P_WR_DATA8(0x16);
	ILI9488P_WR_DATA8(0x19);
	ILI9488P_WR_DATA8(0x03);
	ILI9488P_WR_DATA8(0x0F);
	ILI9488P_WR_DATA8(0x05);
	ILI9488P_WR_DATA8(0x32);
	ILI9488P_WR_DATA8(0x45);
	ILI9488P_WR_DATA8(0x46);
	ILI9488P_WR_DATA8(0x04);
	ILI9488P_WR_DATA8(0x0E);
	ILI9488P_WR_DATA8(0x0D);
	ILI9488P_WR_DATA8(0x35);
	ILI9488P_WR_DATA8(0x37);
	ILI9488P_WR_DATA8(0x0F);

	ILI9488P_WR_CMD(0XC0);     //Power Control 1
	ILI9488P_WR_DATA8(0x17);    //Vreg1out
	ILI9488P_WR_DATA8(0x15);    //Verg2out

	ILI9488P_WR_CMD(0xC1);     //Power Control 2
	ILI9488P_WR_DATA8(0x41);    //VGH,VGL

	ILI9488P_WR_CMD(0xC5);     //Power Control 3
	ILI9488P_WR_DATA8(0x00);
	ILI9488P_WR_DATA8(0x12);    //Vcom
	ILI9488P_WR_DATA8(0x80);

	ILI9488P_WR_CMD(0x36);      //Memory Access
	//ILI9488P_WR_DATA8(0x4A);
							 ////76543210
	ILI9488P_WR_DATA8(0b11101010);

	ILI9488P_WR_CMD(0x3A);      // Interface Pixel Format
	if (_9bit)
	{
		ILI9488P_WR_DATA8(0x66); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits
	}
	else
	{
		ILI9488P_WR_DATA8(0x55); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits  
	}
	ILI9488P_WR_CMD(0XB0);      // Interface Mode Control
	ILI9488P_WR_DATA8(0x80);     //SDO NOT USE

	ILI9488P_WR_CMD(0xB1);      //Frame rate
	ILI9488P_WR_DATA8(0xA0);    //60Hz

	ILI9488P_WR_CMD(0xB4);      //Display Inversion Control
	ILI9488P_WR_DATA8(0x02);    //2-dot

	ILI9488P_WR_CMD(0XB6);     //Display Function Control  RGB/MCU Interface Control

	ILI9488P_WR_DATA8(0x02);    //MCU
	ILI9488P_WR_DATA8(0x02);    //Source,Gate scan direction

	ILI9488P_WR_CMD(0XE9);     // Set Image Function
	ILI9488P_WR_DATA8(0x00);    // Disable 24 bit data

	ILI9488P_WR_CMD(0xF7);      // Adjust Control
	ILI9488P_WR_DATA8(0xA9);
	ILI9488P_WR_DATA8(0x51);
	ILI9488P_WR_DATA8(0x2C);
	ILI9488P_WR_DATA8(0x82);    // D7 stream, loose

	ILI9488P_WR_CMD(0x11);    //Exit Sleep
	delay(120);
	ILI9488P_WR_CMD(0x29);    //Display on
}

void ILI9488_9BIT_PARALLEL::setXY(short x1, short y1, short x2, short y2)
{
	ILI9488P_WR_CMD(0x2a);
	/*ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((x1 >> 8) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((x1) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((x2 >> 8) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((x2) << 12) & 0x000ff000) | 0x20);*/
	ILI9488P_WR_DATA8(x1 >> 8);
	ILI9488P_WR_DATA8(x1);
	ILI9488P_WR_DATA8(x2 >> 8);
	ILI9488P_WR_DATA8(x2);
	ILI9488P_WR_CMD(0x2b);
	/*ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((y1 >> 8) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((y1) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((y2 >> 8) << 12) & 0x000ff000) | 0x20);
	ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
	ESP_WRITE_REG(0x3FF44008, (((y2) << 12) & 0x000ff000) | 0x20);*/
	ILI9488P_WR_DATA8(y1 >> 8);
	ILI9488P_WR_DATA8(y1);
	ILI9488P_WR_DATA8(y2 >> 8);
	ILI9488P_WR_DATA8(y2);
	ILI9488P_WR_CMD(0x2b);
	ILI9488P_WR_CMD(0x2c);
}

void ILI9488_9BIT_PARALLEL::setXY(short x, short y)
{
	static short lastX = 1000, lastY = 1000;
	if (lastX != x)
	{
		ILI9488P_WR_CMD(0x2a);
		/*ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((x >> 8) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((x) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((x >> 8) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((x) << 12) & 0x000ff000) | 0x20);*/
		ILI9488P_WR_DATA8(x >> 8);
		ILI9488P_WR_DATA8(x);
		ILI9488P_WR_DATA8(x >> 8);
		ILI9488P_WR_DATA8(x);
	}
	if (lastY != y)
	{
		ILI9488P_WR_CMD(0x2b);
		/*
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((y >> 8) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((y) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((y >> 8) << 12) & 0x000ff000) | 0x20);
		ESP_WRITE_REG(0x3FF4400c, 0x000FF020);
		ESP_WRITE_REG(0x3FF44008, (((y) << 12) & 0x000ff000) | 0x20);*/
		ILI9488P_WR_DATA8(y >> 8);
		ILI9488P_WR_DATA8(y);
		ILI9488P_WR_DATA8(y >> 8);
		ILI9488P_WR_DATA8(y);
	}
	ILI9488P_WR_CMD(0x2c);
	lastX = x;
	lastY = y;
}

void ILI9488_9BIT_PARALLEL::drawPixel(short x, short y)
{
	setXY(x, y);
	ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
	ESP_WRITE_REG(0x3FF44008, bCh);
	ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
	ESP_WRITE_REG(0x3FF44008, bCl);
}

void ILI9488_9BIT_PARALLEL::fillScr(unsigned char r, unsigned char g, unsigned char b)
{
	setXY(0, 0, 479, 319);

	unsigned int bch, bcl;
	unsigned int i, colorH, colorL;

	if (_9bitFlag)
	{
		colorH = GET_RGB666_H(r, g, b);
		colorL = GET_RGB666_L(r, g, b);
		bcl = ILI9488P_MAP_9BIT(colorL);
		bch = ILI9488P_MAP_9BIT(colorH);
	}
	else
	{
		colorH = GET_RGB565_H(r, g, b);
		colorL = GET_RGB565_L(r, g, b);
		bcl = ILI9488P_MAP_8BIT(colorL);
		bch = ILI9488P_MAP_8BIT(colorH);
	}

	if (bch == bcl)
	{
		ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
		ESP_WRITE_REG(0x3FF44008, bcl);

		for (i = 0; i < 480 * 320 * 2; i++)
		{
			ESP_WRITE_REG(0x3FF4400c, 0x04);
			ESP_WRITE_REG(0x3FF44008, 0x04);
		}
	}
	else
	{
		for (i = 0; i < 480 * 320; i++)
		{
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bch);
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bcl);
		}
	}
}

void ILI9488_9BIT_PARALLEL::drawCompressed24bitBitmap(short x, short y, const unsigned int * dataArray)
{
	unsigned int	hight, width;
	unsigned int	buffer;
	int				index = 0;

	width = dataArray[index];
	index++;

	buffer = dataArray[index];
	hight = buffer;
	index++;

	unsigned int dataArraySize = hight * width, i, j, counter = 0,colorL, colorH;
	unsigned int bch, bcl;
	unsigned char copies,r,g,b;

	setXY(x, y, x + width - 1, y + hight - 1);
	for (i = 2; counter < dataArraySize; i++)
	{
		b = (dataArray[index] & 0x00ff0000) >> 16;
		g = (dataArray[index] & 0x0000ff00) >> 8;
		r = (dataArray[index] & 0x000000ff);
		copies = (dataArray[index] >> 24);
		index++;
		if (_9bitFlag)
		{
			colorL = GET_RGB666_L(r, g, b);
			bcl = ILI9488P_MAP_9BIT(colorL);
			colorH = GET_RGB666_H(r, g, b);
			bch = ILI9488P_MAP_9BIT(colorH);
		}
		else
		{
			colorH = GET_RGB565_H(r, g, b);
			colorL = GET_RGB565_L(r, g, b);
			bcl = ILI9488P_MAP_8BIT(colorL);
			bch = ILI9488P_MAP_8BIT(colorH);
		}

		for (size_t j = 0; j < copies; j++)
		{
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bch);
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bcl);
		}
		counter += copies;
	}
}

void ILI9488_9BIT_PARALLEL::drawCompressedGrayScaleBitmap(short x, short y, const unsigned short * dataArray, bool invert)
{
	unsigned int hight = dataArray[0], width = dataArray[1];
	unsigned int dataArraySize = hight * width, i, j, counter = 0, colorL, colorH;
	unsigned char copies, c;
	unsigned int bch, bcl;

	setXY(x, y, x + hight - 1, y + width - 1);
	for (i = 2 ; counter < dataArraySize; i++)
	{
		copies = (dataArray[i] >> 8);
		if (invert)
		{
			c = 255 - dataArray[i];
		}
		else
		{
			c = dataArray[i];
		}
		if (_9bitFlag)
		{
			colorL = GET_RGB666_L(c, c, c);
			bcl = ILI9488P_MAP_9BIT(colorL);
			colorH = GET_RGB666_H(c, c, c);
			bch = ILI9488P_MAP_9BIT(colorH);
		}
		else
		{
			colorH = GET_RGB565_H(c, c, c);
			colorL = GET_RGB565_L(c, c, c);
			bcl = ILI9488P_MAP_8BIT(colorL);
			bch = ILI9488P_MAP_8BIT(colorH);
		}

		for (size_t j = 0; j < copies; j++)
		{
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bch);
			ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
			ESP_WRITE_REG(0x3FF44008, bcl);
		}
		counter += copies;
	}
}

void ILI9488_9BIT_PARALLEL::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int colorL, colorH;
	if (_9bitFlag)
	{
		colorH = GET_RGB666_H(r, g, b);
		colorL = GET_RGB666_L(r, g, b);
		bCl = ILI9488P_MAP_9BIT(colorL);
		bCh = ILI9488P_MAP_9BIT(colorH);
	}
	else
	{
		colorH = GET_RGB565_H(r, g, b);
		colorL = GET_RGB565_L(r, g, b);
		bCl = ILI9488P_MAP_8BIT(colorL);
		bCh = ILI9488P_MAP_8BIT(colorH);
	}
}

void ILI9488_9BIT_PARALLEL::drawHLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		x -= l;
	}
	setXY(x, y, x + l, y);
	for (size_t i = 0; i < l; i++)
	{
		ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
		ESP_WRITE_REG(0x3FF44008, bCh);
		ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
		ESP_WRITE_REG(0x3FF44008, bCl);
	}
}

void ILI9488_9BIT_PARALLEL::drawVLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	setXY(x, y, x, y + l);
	for (size_t i = 0; i < l; i++)
	{
		ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
		ESP_WRITE_REG(0x3FF44008, bCh);
		ESP_WRITE_REG(0x3FF4400c, PAR_IOs_MASK);
		ESP_WRITE_REG(0x3FF44008, bCl);
	}
}

bool ILI9488_9BIT_PARALLEL::initI2Sparallel(I2Ssetup * i2sSetup)
{
	if (i2sSetup->port == 0)
	{
		port = I2S_NUM_0;
		i2sRegBase = I2S0_REG_BASE;
	}
	else
	{
		port = I2S_NUM_1;
		i2sRegBase = I2S1_REG_BASE;
	}
	dev = I2S[port];
	int irq_source;
	// Initialize I2S peripheral
	if (port == I2S_NUM_0) 
	{
		periph_module_reset(PERIPH_I2S0_MODULE);
		periph_module_enable(PERIPH_I2S0_MODULE);
		iomux_clock = I2S0O_WS_OUT_IDX;
		irq_source = ETS_I2S0_INTR_SOURCE;
		iomux_signal_base = I2S0O_DATA_OUT8_IDX;
	}
	else 
	{
		periph_module_reset(PERIPH_I2S1_MODULE);
		periph_module_enable(PERIPH_I2S1_MODULE);
		iomux_clock = I2S1O_WS_OUT_IDX;
		irq_source = ETS_I2S1_INTR_SOURCE;
		iomux_signal_base = I2S1O_DATA_OUT0_IDX;
	}

	// Setup GPIOs
	for (int i = 0; i < 8; i++) 
	{
		pinMode(i2sSetup->dataPins[i], OUTPUT);
		gpio_matrix_out(i2sSetup->dataPins[i], iomux_signal_base + i, false, false);
		pinList[i] = i2sSetup->dataPins[i];
	}

	gpio_matrix_out(i2sSetup->clockPin, iomux_clock, true, false);
	pinList[8] = i2sSetup->clockPin;

	// Setup I2S peripheral
	dev_reset(dev);

	// Set i2s mode to LCD mode
	dev->conf2.val = 0;
	dev->conf2.lcd_en = 1;
	dev->conf2.lcd_tx_wrx2_en = 1; // Gil
	dev->conf2.lcd_tx_sdx2_en = 0; // Gil
	dev->conf.tx_slave_mod = 0;

	// Setup i2s clock
	dev->sample_rate_conf.val = 0;
	// Third stage config, width of data to be written to IO (I think this should always be the actual data width?)
	dev->sample_rate_conf.rx_bits_mod = 16;// bus_width;
	dev->sample_rate_conf.tx_bits_mod = 16;// bus_width;
	dev->sample_rate_conf.rx_bck_div_num = 2;
	dev->sample_rate_conf.tx_bck_div_num = 2;

	dev->clkm_conf.clka_en = 0;
	dev->clkm_conf.clkm_div_a = 64;
	dev->clkm_conf.clkm_div_b = 1;
	dev->clkm_conf.clkm_div_num = i2sSetup->freq;// 6 (12.5MHz) - stable with no link list; // 8 = 10MHz (This is the only stable frequency) 4 = 20MHz, 2 = 40 MHz

		// Some fifo conf I don't quite understand 
	dev->fifo_conf.val = 0;
	// Dictated by datasheet
	dev->fifo_conf.rx_fifo_mod_force_en = 1;
	dev->fifo_conf.tx_fifo_mod_force_en = 1;
	// Not really described for non-pcm modes, although datasheet states it should be set correctly even for LCD mode
	// First stage config. Configures how data is loaded into fifo
	dev->fifo_conf.tx_fifo_mod = 1;
	// Probably relevant for buffering from the DMA controller
	dev->fifo_conf.rx_data_num = 0; //Thresholds. 
	dev->fifo_conf.tx_data_num = 0;
	// Enable DMA support
	dev->fifo_conf.dscr_en = 1;

	dev->conf1.val = 0;
	dev->conf1.tx_stop_en = 1;

	dev->conf1.tx_pcm_bypass = 1;

	// Second stage config
	// dev->conf_chan.val = 0;
	// Tx in mono mode, read 32 bit per sample from fifo

	dev->conf.tx_mono = 1; // Gil
	//dev->conf.tx_msb_right = 1; // Gil - Table 59 in I2S spec

	dev->conf_chan.tx_chan_mod = 1;
	dev->conf_chan.rx_chan_mod = 1;

	dev->conf.tx_right_first = 0;
	dev->conf.rx_right_first = 0;

	dev->lc_conf.out_eof_mode = 1;
	dev->lc_conf.check_owner = 0;
	//dev->lc_conf.out_auto_wrback = 1;

	dev->timing.val = 0;

	paraBusEnabled	= true;
	initialized		= true;

	return true;
}

void ILI9488_9BIT_PARALLEL::disableI2Sparallel()
{
	if (!paraBusEnabled)
		return;
	for (size_t i = 0; i < 9; i++)
	{
		gpio_matrix_out(pinList[i], 0x100, false, false);
	}
	paraBusEnabled = false;
}

bool ILI9488_9BIT_PARALLEL::restartI2Sparallel()
{
	if (!initialized)
		return false;
	for (int i = 0; i < 8; i++)
	{
		gpio_matrix_out(pinList[i], iomux_signal_base + i, false, false);
	}
	gpio_matrix_out(pinList[8], iomux_clock, true, false);

	paraBusEnabled = true;
	return true;
}

void ILI9488_9BIT_PARALLEL::fifo_reset(i2s_dev_t * dev)
{
	dev->conf.rx_fifo_reset = 1;
	//  while(dev->state.rx_fifo_reset_back);
	dev->conf.rx_fifo_reset = 0;
	dev->conf.tx_fifo_reset = 1;
	// while(dev->state.tx_fifo_reset_back);
	dev->conf.tx_fifo_reset = 0;
}

void ILI9488_9BIT_PARALLEL::dev_reset(i2s_dev_t * dev)
{
	unsigned int lcConf = *((volatile unsigned char *)(((i2sRegBase + 0x60))));
	unsigned int conf = *((volatile unsigned char *)(((i2sRegBase + 0x8))));
	// do rmw
	(*((volatile unsigned char *)(((i2sRegBase + 0x8)))) = conf | 0x5);
	(*((volatile unsigned char *)(((i2sRegBase + 0x8)))) = conf);
	(*((volatile unsigned char *)(((i2sRegBase + 0x60)))) = lcConf | 0x3);
	(*((volatile unsigned char *)(((i2sRegBase + 0x60)))) = lcConf);
}

bool ILI9488_9BIT_PARALLEL::parallelStartDMA(lldesc_t * dma_descriptor)
{
	if (!paraBusEnabled)
	{
		return false;
	}

	// Stop all ongoing DMA operations
	dev->out_link.stop = 1;
	dev->out_link.start = 0;
	dev->conf.tx_start = 0;
	dev_reset(dev);

	// Configure DMA burst mode
	dev->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
	// Set address of DMA descriptor
	dev->out_link.addr = (uint32_t)dma_descriptor;
	// Start DMA operation
	dev->out_link.start = 1;
	dev->conf.tx_start = 1;

	return true;
}

bool ILI9488_9BIT_PARALLEL::parallelIsDMAactiv()
{
	return (!((ESP_READ_REG(i2sRegBase + 0xBC) & 0x1) == 0x1));
}

bool ILI9488_9BIT_PARALLEL::parallelFIFOwriteWord(unsigned int data)
{
	if (!paraBusEnabled)
	{
		return false;
	}
	dev->fifo_conf.dscr_en = 0;
	dev->conf.tx_start = 0;
	dev->conf.tx_reset = 1;
	dev->conf.tx_reset = 0;
	dev->conf.tx_fifo_reset = 1;
	dev->conf.tx_fifo_reset = 0;
	(*((volatile unsigned int *)(((i2sRegBase)))) = data);
	dev->conf.tx_start = 1;
	dev->fifo_conf.dscr_en = 1;

	return true;
}
//////////////////////////// ILI9488 SPI base class ////////////////////////////////////
void ILI9488SPI_BASE::_init(unsigned char sck, unsigned char miso, unsigned char mosi, unsigned char ss, unsigned int freq, ili9488_mode mode)
{
	SPI.begin((char)sck, (char)-1, (char)mosi, (char)-1);
	SPI.setBitOrder(MSBFIRST);
	SPI.setFrequency(freq);
	SPI.setDataMode(3);

	//ESP_WRITE_REG(0x3FF49070, 0x1C00);
	//ESP_WRITE_REG(0x3FF49074, 0x1C00);
	//ESP_WRITE_REG(0x3FF4908C, 0x1C00);
	   
#ifdef ESP32
	ESP32_SPI_DIS_MOSI_MISO_FULL_DUPLEX(ESP32_VSPI);
#endif
	
	ILI9488SPI_PRE_INIT();
	ILI9488SPI_SELECT_DATA_COMM(ILI9488SPI_DATA);
	ILI9488SPI_ASSERT_CS();

	ILI9488SPI_LCD_WRITE_COM(0xE0);
	ILI9488SPI_LCD_WRITE_DATA8(0x00);
	ILI9488SPI_LCD_WRITE_DATA8(0x03);
	ILI9488SPI_LCD_WRITE_DATA8(0x09);
	ILI9488SPI_LCD_WRITE_DATA8(0x08);
	ILI9488SPI_LCD_WRITE_DATA8(0x16);
	ILI9488SPI_LCD_WRITE_DATA8(0x0A);
	ILI9488SPI_LCD_WRITE_DATA8(0x3F);
	ILI9488SPI_LCD_WRITE_DATA8(0x78);
	ILI9488SPI_LCD_WRITE_DATA8(0x4C);
	ILI9488SPI_LCD_WRITE_DATA8(0x09);
	ILI9488SPI_LCD_WRITE_DATA8(0x0A);
	ILI9488SPI_LCD_WRITE_DATA8(0x08);
	ILI9488SPI_LCD_WRITE_DATA8(0x16);
	ILI9488SPI_LCD_WRITE_DATA8(0x1A);
	ILI9488SPI_LCD_WRITE_DATA8(0x0F);

	ILI9488SPI_LCD_WRITE_COM(0XE1);
	ILI9488SPI_LCD_WRITE_DATA8(0x00);
	ILI9488SPI_LCD_WRITE_DATA8(0x16);
	ILI9488SPI_LCD_WRITE_DATA8(0x19);
	ILI9488SPI_LCD_WRITE_DATA8(0x03);
	ILI9488SPI_LCD_WRITE_DATA8(0x0F);
	ILI9488SPI_LCD_WRITE_DATA8(0x05);
	ILI9488SPI_LCD_WRITE_DATA8(0x32);
	ILI9488SPI_LCD_WRITE_DATA8(0x45);
	ILI9488SPI_LCD_WRITE_DATA8(0x46);
	ILI9488SPI_LCD_WRITE_DATA8(0x04);
	ILI9488SPI_LCD_WRITE_DATA8(0x0E);
	ILI9488SPI_LCD_WRITE_DATA8(0x0D);
	ILI9488SPI_LCD_WRITE_DATA8(0x35);
	ILI9488SPI_LCD_WRITE_DATA8(0x37);
	ILI9488SPI_LCD_WRITE_DATA8(0x0F);

	ILI9488SPI_LCD_WRITE_COM(0XC0);     //Power Control 1
	ILI9488SPI_LCD_WRITE_DATA8(0x17);    //Vreg1out
	ILI9488SPI_LCD_WRITE_DATA8(0x15);    //Verg2out

	ILI9488SPI_LCD_WRITE_COM(0xC1);     //Power Control 2
	ILI9488SPI_LCD_WRITE_DATA8(0x41);    //VGH,VGL

	ILI9488SPI_LCD_WRITE_COM(0xC5);     //Power Control 3
	ILI9488SPI_LCD_WRITE_DATA8(0x00);
	ILI9488SPI_LCD_WRITE_DATA8(0x12);    //Vcom
	ILI9488SPI_LCD_WRITE_DATA8(0x80);

	ILI9488SPI_LCD_WRITE_COM(0x36);      //Memory Access
	//ILI9488SPI_LCD_WRITE_DATA8(0x4A);
							 ////76543210
	ILI9488SPI_LCD_WRITE_DATA8(0b11101010);

	ILI9488SPI_LCD_WRITE_COM(0x3A);      // Interface Pixel Format

	if (mode == rgb666)
	{	
		ILI9488SPI_LCD_WRITE_DATA8(0x66); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits is not supported in SPI mode! 
	}
	else
	{
		ILI9488SPI_LCD_WRITE_DATA8(0x11); 	  // 0x11 = 3 bits (3 bits per pixel) 
	}	
	
	ILI9488SPI_LCD_WRITE_COM(0XB0);      // Interface Mode Control
	ILI9488SPI_LCD_WRITE_DATA8(0x80);     //SDO NOT USE

	ILI9488SPI_LCD_WRITE_COM(0xB1);      //Frame rate
	ILI9488SPI_LCD_WRITE_DATA8(0xA0);    //60Hz

	ILI9488SPI_LCD_WRITE_COM(0xB4);      //Display Inversion Control
	ILI9488SPI_LCD_WRITE_DATA8(0x02);    //2-dot

	ILI9488SPI_LCD_WRITE_COM(0XB6);     //Display Function Control  RGB/MCU Interface Control

	ILI9488SPI_LCD_WRITE_DATA8(0x02);    //MCU
	ILI9488SPI_LCD_WRITE_DATA8(0x02);    //Source,Gate scan direction

	ILI9488SPI_LCD_WRITE_COM(0XE9);     // Set Image Function
	ILI9488SPI_LCD_WRITE_DATA8(0x00);    // Disable 24 bit data

	ILI9488SPI_LCD_WRITE_COM(0xF7);      // Adjust Control
	ILI9488SPI_LCD_WRITE_DATA8(0xA9);
	ILI9488SPI_LCD_WRITE_DATA8(0x51);
	ILI9488SPI_LCD_WRITE_DATA8(0x2C);
	ILI9488SPI_LCD_WRITE_DATA8(0x82);    // D7 stream, loose

	ILI9488SPI_LCD_WRITE_COM(0x11);    //Exit Sleep
	delay(120);
	ILI9488SPI_LCD_WRITE_COM(0x29);    //Display on

}

void ILI9488SPI_BASE::setXY(short x1, short y1, short x2, short y2)
{
	int ex = x2 | (x1 << 16);
	int ey = y2 | (y1 << 16);

	ILI9488SPI_LCD_WRITE_COM(0x2a);
	SPI.writeUINT(ex);
	ILI9488SPI_LCD_WRITE_COM(0x2b);
	SPI.writeUINT(ey);
	ILI9488SPI_LCD_WRITE_COM(0x2c);
}

void ILI9488SPI_BASE::setXY(short x, short y)
{
	static short lastX = 1000, lastY = 1000;
	if (lastX != x)
	{
		int ex = x | (x << 16);
		ILI9488SPI_LCD_WRITE_COM(0x2a);
		SPI.writeUINT(ex); // Need to optimize !
	}
	if (lastY != y)
	{
		int ey = y | (y << 16);
		ILI9488SPI_LCD_WRITE_COM(0x2b);
		SPI.writeUINT(ey);
	}
	ILI9488SPI_LCD_WRITE_COM(0x2c);
	lastX = x;
	lastY = y;
}

void ILI9488SPI_264KC::init(unsigned char sck, unsigned char miso, unsigned char mosi, unsigned char ss, unsigned int freq)
{
	ILI9488SPI_BASE::_init(sck, miso, mosi, ss, freq, rgb666);
}

inline void ILI9488SPI_264KC::drawPixel(short x, short y)
{
	setXY(x, y);
	ILI9488SPI_LCD_WRITE_DATA(_fgColor);
}

void ILI9488SPI_264KC::fillScr(unsigned char r, unsigned char g,unsigned char b)
{
	unsigned int color = ((unsigned int)b << 16) | ((unsigned int)g << 8) | (unsigned int)r;

	setXY(0, 0, maxX - 1, maxY - 1);
	SPI.writeRGB(color, maxX * maxY);
}

void ILI9488SPI_264KC::drawHLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		x -= l;
	}
	setXY(x, y, x + l, y);
	SPI.writePattern(fgColor, 3, l);
}

void ILI9488SPI_264KC::drawVLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	setXY(x, y, x, y + l);
	SPI.writePattern(fgColor, 3, l);
}

void ILI9488SPI_264KC::drawRect(short x1, short y1, short x2, short y2, bool fill)
{
	if (x1 > x2)
	{
		swap(&x1, &x2);
	}
	if (y1 > y2)
	{
		swap(&y1, &y2);
	}

	if (fill)
	{
		setXY(x1, y1, x2, y2);
		SPI.writeRGB(_fgColor, (x2-x1 + 1)*(y2-y1 + 1));
	}
	else
	{
		drawHLine(x1, y1, x2 - x1);
		drawHLine(x1, y2, x2 - x1);
		drawVLine(x1, y1, y2 - y1);
		drawVLine(x2, y1, y2 - y1);
	}
}

#define DATA_START_OFFSET 2

void ILI9488SPI_264KC::drawCompressed24bitBitmap(short x, short y, const unsigned int * dataArray)
{
	unsigned int	hight, width;
	unsigned int	buffer;
	int				index = 0;

	width = dataArray[index];
	index++;

	buffer = dataArray[index];
	hight = buffer;
	index++;

	unsigned int dataArraySize = hight * width, i, j, counter = 0;
	unsigned char copies;

	setXY(x, y, x + width - 1, y + hight - 1);
	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		SPI.writeRGB(buffer & 0x00ffffff, copies);
		counter += copies;
	}
}

void ILI9488SPI_264KC::drawCompressedGrayScaleBitmap(short x, short y, const unsigned short * dataArray, bool invert)
{
	unsigned int hight = dataArray[0], width = dataArray[1];
	unsigned int dataArraySize = hight * width, i, j, counter = 0;
	unsigned char copies;

	setXY(x, y, x + hight - 1, y + width - 1);
	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		copies = (dataArray[i] >> 8);
		if (invert)
		{
			SPI.writeGrayScale(255 - dataArray[i], copies);
		}
		else
		{
			SPI.writeGrayScale(dataArray[i], copies);
		}
		counter += copies;
	}
}

extern unsigned short * hebCharSet[];
extern unsigned short * numberCharSet[];
extern unsigned short apostrophesCF[], periodCF[], minusCF[], tagCF[], colonCF[];
#define SPACE_SIZE		10
#define HEBREW_CHAR_SET	215

void ILI9488SPI_264KC::drawHebStringUTF8(short x, short y, const char * str, bool swapString, bool invert)
{
	// Heb Strings are swapped //
	char *	tempStr;
	int *	tempStrNum;
	int i, j, counter = 0, index;

	if (swapString)
	{
		int len = strlen(str);
		tempStr = new char[len + 1];
		tempStrNum = new int[len + 1];

		for (i = 0; i < len; i++)
		{
			tempStrNum[i] = 0;
		}
		for (i = 0; i < len; i++)
		{
			if (str[i] >= '0' && str[i] <= '9')
			{
				counter++;
				tempStrNum[i] = counter;
			}
			else
				counter = 0;
		}
		index = -1;
		for (i = len - 1, j = 0; i >= 0; i--, j++)
		{
			if (str[i] >= '0' && str[i] <= '9')
			{
				tempStr[index + tempStrNum[i]] = str[i];
			}
			else
			{
				index = j;
			}
		}
		j = len - 1;
		for (i = 0; i < len; )
		{
			if (str[i] == HEBREW_CHAR_SET)
			{
				tempStr[j - 1] = str[i];
				tempStr[j] = str[i + 1];
				i += 2;
				j -= 2;
			}
			else
			{
				if (!(str[i] >= '0' && str[i] <= '9'))
					tempStr[j] = str[i];
				i += 1;
				j -= 1;
			}
		}
		tempStr[len] = '\0';
	}
	else
	{
		tempStr = (char *)str;
	}

	/////////////////////////////
	int xIndex = x, strLen = strlen(tempStr), tempX, tempY;
	unsigned short * currentChar, tempChar;
	for (i = 0; i < strLen;) // UTF8 -> each char is 2 bytes 
	{
		if (tempStr[i] == HEBREW_CHAR_SET) // UTF8 char
		{
			i++;
			if (tempStr[i] >= 144 && tempStr[i] <= 170)
			{
				// Heb Char
				currentChar = hebCharSet[tempStr[i] - 144];
				tempX = xIndex;// -currentChar[0];
				if (tempStr[i] == 156 /* Lamed*/)
					tempY = y - 9;
				else
					tempY = y;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			i++; // Un-supported UTF8 char
		}
		else
		{
			if (tempStr[i] >= 48 && tempStr[i] <= 57) // Digit
			{
				currentChar = numberCharSet[tempStr[i] - 48];
				tempX = xIndex;// -currentChar[0];
				tempY = y;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			if (tempStr[i] == 32 /* space */)
				xIndex += SPACE_SIZE;
			if (tempStr[i] == 34)
			{
				// " 
				currentChar = apostrophesCF;
				tempX = xIndex;// -currentChar[0];
				tempY = y - 3;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			if (tempStr[i] == 45)
			{
				// - 
				currentChar = minusCF;
				tempX = xIndex;// -currentChar[0];
				tempY = y + 11;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			if (tempStr[i] == 46)
			{
				// . 
				currentChar = periodCF;
				tempX = xIndex;// -currentChar[0];
				tempY = y + 19;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			if (tempStr[i] == 39)
			{
				// ' 
				currentChar = tagCF;
				tempX = xIndex;// -currentChar[0];
				tempY = y - 3;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			if (tempStr[i] == 58)
			{
				// : 
				currentChar = colonCF;
				tempX = xIndex;// -currentChar[0];
				tempY = y + 3;
				drawCompressedGrayScaleBitmap(tempX, tempY, currentChar,invert);
				xIndex += (currentChar[0] + 1);
			}
			i++;
		}
	}
	if (swapString)
	{
		delete[] tempStr;
		delete[] tempStrNum;
	}
}

int	 ILI9488SPI_264KC::getStringWidth(const char * str)
{
	int size = 0, strLen = strlen(str),i;
	unsigned short * currentChar;

	for (i = 0; i < strLen;) // UTF8 -> each char is 2 bytes 
	{
		if (str[i] == HEBREW_CHAR_SET) // UTF8 char
		{
			i++;
			if (str[i] >= 144 && str[i] <= 170)
			{
				currentChar = hebCharSet[str[i] - 144];
				size += (currentChar[0] + 1);
			}
			i++; // Un-supported UTF8 char
		}
		else
		{
			if (str[i] >= 48 && str[i] <= 57) // Digit
			{
				currentChar = numberCharSet[str[i] - 48];
				size += (currentChar[0] + 1);
			}
			if (str[i] == 32 /* space */)
				size += SPACE_SIZE;
			if (str[i] == 34)
			{
				// " 
				currentChar = apostrophesCF;
				size += (currentChar[0] + 1);
			}
			if (str[i] == 45)
			{
				// - 
				currentChar = minusCF;
				size += (currentChar[0] + 1);
			}
			if (str[i] == 46)
			{
				// . 
				currentChar = periodCF;
				size += (currentChar[0] + 1);
			}
			if (str[i] == 39)
			{
				// ' 
				currentChar = tagCF;
				size += (currentChar[0] + 1);
			}
			if (str[i] == 58)
			{
				// : 
				currentChar = colonCF;
				size += (currentChar[0] + 1);
			}
			i++;
		}
	}
	return size;
}

void ILI9488SPI_264KC::testFunc()
{
	fillScr(255, 255, 255);
	setColor(255, 0, 0);
	drawRect(49, 49, 101, 101);
	setColor(0, 0, 0);
	setXY(50, 50, 100, 100);
	SPI.writeRGB(_fgColor);
	SPI.writeRGB(_fgColor);
	SPI.writeRGB(_fgColor);
}

#define ILI9488_8C_FRAME_BUFFER_SIZE	(480*320/2)

bool ILI9488SPI_8C::init(unsigned char sck, unsigned char miso, unsigned char mosi, unsigned char ss, unsigned int freq, ili9488_8C_mode mode)
{
	ILI9488SPI_BASE::_init(sck, miso, mosi, ss, freq, rgb111);
	
	currentFrameBufferIndex = 0;
	workingMode = mode;
	FGbitOn = false;

	switch (mode)
	{
	case singleFrameBuffer:
	case directMode:
		frameBuffers[0] = (unsigned char *)malloc(ILI9488_8C_FRAME_BUFFER_SIZE);
		if (frameBuffers[0] == NULL)
		{
			return false;
		}
		break;
	case dualFrameBuffers:
		frameBuffers[0] = (unsigned char *)malloc(ILI9488_8C_FRAME_BUFFER_SIZE);
		if (frameBuffers[0] == NULL)
		{
			return false;
		}
		frameBuffers[1] = (unsigned char *)malloc(ILI9488_8C_FRAME_BUFFER_SIZE);
		if (frameBuffers[1] == NULL)
		{
			free(frameBuffers[0]);
			return false;
		}
		break;
	default:
		return false;
	}

	return true;
}

void ILI9488SPI_8C::setColor(unsigned char r, unsigned char g,unsigned char b)
{
	fgColorL = ((r == 1) << 2) | ((g == 1) << 1) | (b == 1);
	fgColorH = fgColorL << 3;
	fgColorHL = fgColorL | fgColorH;
}

void ILI9488SPI_8C::setBackColor(unsigned char r, unsigned char g,unsigned char b)
{
	bgColorL = ((r == 1) << 2) | ((g == 1) << 1) | (b == 1);
	bgColorH = bgColorL << 3;
}

void ILI9488SPI_8C::fillScr(unsigned char r, unsigned char g,unsigned char b)
{
	unsigned char tempColorL = ((r == 1) << 2) | ((g == 1) << 1) | (b == 1);
	unsigned char tempColor = (tempColorL << 3) | tempColorL;
	if (FGbitOn)
	{
		tempColorL = tempColorL | 0xc0;
	}
	unsigned int tempC = (tempColor << 24) | (tempColor << 16) | (tempColor << 8 | tempColor);
	unsigned int *tempBuffer = (unsigned int *)frameBuffers[currentFrameBufferIndex];
	for (size_t i = 0; i < (ILI9488_8C_FRAME_BUFFER_SIZE / 4); i++)
	{
		tempBuffer[i] = tempC;
	}
}

void ILI9488SPI_8C::flushFrameBuffer()
{
	setXY(0, 0, maxX, maxY);
	SPI.writeBuffer((unsigned int *)frameBuffers[currentFrameBufferIndex], ILI9488_8C_FRAME_BUFFER_SIZE / 4);
}

inline void ILI9488SPI_8C::drawPixel(short x, short y)
{
	unsigned int index = (y*maxX + x) >> 1;

	if (FGbitOn)
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x38 | fgColorL | 0x40;
		}
		else
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x07 | fgColorH | 0x80;
		}
	}
	else
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x38 | fgColorL;
		}
		else
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x07 | fgColorH;
		}
	}
}

inline void ILI9488SPI_8C::drawPixel(short x, short y, unsigned char color)
{
	if (x < 0 || x > 479 || y < 0 || y > 319)
	{
		return;
	}
	unsigned int index = (y*maxX + x) >> 1;

	if (FGbitOn)
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x38 | color | 0x40;
		}
		else
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x07 | (color << 3) | 0x80;
		}
	}
	else
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x38 | color;
		}
		else
		{
			frameBuffers[currentFrameBufferIndex][index] = frameBuffers[currentFrameBufferIndex][index] & 0x07 | (color << 3);
		}
	}

}

void ILI9488SPI_8C::drawHLine(short x, short y, int l)
{
	size_t i = 0,lo = l;
	unsigned int index;
	
	if (x & 0x1 == 1) // x%2 == 1
	{
		drawPixel(x, y);
		i++;
		lo -= 1;
	}
	while (i <= l) // We are alighned to 2
	{
		if(lo >= 2)
		{
			index = (y*maxX + x + i) >> 1;
			if (index >= 480 * 320 / 2)
				return;
			if (FGbitOn)
			{
				frameBuffers[currentFrameBufferIndex][index] = fgColorHL | 0xC0;
			}
			else
			{
				frameBuffers[currentFrameBufferIndex][index] = fgColorHL;
			}
			i += 2;
			lo -= 2;
		}
		else
		{
			drawPixel(x + i, y);
			i++;
		}
	}
}

inline bool ILI9488SPI_8C::isFGbitSet(short x, short y)
{
	unsigned int index = (y*maxX + x) >> 1;
	if (x & 0x1 == 1) // x%2 == 1
	{
		return ((frameBuffers[currentFrameBufferIndex][index] & 0x40) == 0x40);
	}
	else
	{
		return ((frameBuffers[currentFrameBufferIndex][index] & 0x80) == 0x80);
	}
}
/* Structure of bitmap: */
/* array of unsigned chars*/
/* wh,wl,hh,hl,data .....*/
/* data: */
/* Each byte represents two pixels: SRGBSRGB - S = Skip, if set to 1 pixel is not drawn */

#define DRAW_PIX(X,Y,PIX_DATA)\
		if (X < maxX && Y < maxY) {\
		if (!isFGbitSet(X, Y)) drawPixel(X, Y, (PIX_DATA) & 0x7); else collision = true;}

bool ILI9488SPI_8C::drawBitmap(short x, short y, const unsigned char * dataArray,bool useSkipBit, flipOption flipOpt)
{
	bool collision = false;
	unsigned char pixelData;
	unsigned short width, hight;
	unsigned int index = 4,size;
	width = (dataArray[0] << 8) | dataArray[1];
	hight = (dataArray[2] << 8) | dataArray[3];
	size = (width / 2 + (width & 01 == 1)) * hight;

	if (flipOpt == flipY || flipOpt == flipXY)
	{
		for (int iy = (y + hight - 1); iy >= y ; iy--)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	else
	{
		for (int iy = y; iy < (y + hight); iy++)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	return collision;
}

/************** ST7789 ***************/

void ST77XX::setXY(short x1, short y1, short x2, short y2)
{
	x1 += Xoffset;
	y1 += Yoffset;
	x2 += Xoffset;
	y2 += Yoffset;

	int ex = x2 | (x1 << 16);
	int ey = y2 | (y1 << 16);

	ST77XXSPI_LCD_WRITE_COM(0x2a);
	SPI.writeUINT(ex);
	ST77XXSPI_LCD_WRITE_COM(0x2b);
	SPI.writeUINT(ey);
	ST77XXSPI_LCD_WRITE_COM(0x2c);
}

void ST77XX::setXY(short x, short y)
{
	static short lastX = 1000, lastY = 1000;
	x += Xoffset;
	y += Yoffset;
	if (lastX != x)
	{
		int ex = x | (x << 16);
		ST77XXSPI_LCD_WRITE_COM(0x2a);
		SPI.writeUINT(ex); // Need to optimize !
	}
	if (lastY != y)
	{
		int ey = y | (y << 16);
		ST77XXSPI_LCD_WRITE_COM(0x2b);
		SPI.writeUINT(ey);
	}
	ST77XXSPI_LCD_WRITE_COM(0x2c);
	lastX = x;
	lastY = y;
}

void ST77XX::init(unsigned int freq)
{
	pinMode(ST77XXSPI_DATA_COMMAND_PIN, OUTPUT);
	pinMode(ST77XXSPI_CS_PIN, OUTPUT);
	pinMode(ST77XXSPI_RESET_PIN, OUTPUT);
	if (_res == _135x240 || _res == _240x135)
	{
		pinMode(ST77XXSPI_BACK_LIGHT_PIN, OUTPUT);
	}

	SPI.begin((char)ST77XXSPI_CLK_PIN, (char)-1, (char)ST77XXSPI_MOSI_PIN, (char)ST77XXSPI_CS_PIN);
	SPI.setBitOrder(MSBFIRST);
	SPI.setFrequency(freq);
	SPI.setDataMode(3);

#ifdef ESP32
	ESP32_SPI_DIS_MOSI_MISO_FULL_DUPLEX(ESP32_VSPI);
#endif

	ST77XXSPI_SELECT_DATA_COMM(ST77XXSPI_DATA);
	ST77XXSPI_ASSERT_CS();

	digitalWrite(ST77XXSPI_RESET_PIN, LOW);
	delay(250);
	digitalWrite(ST77XXSPI_RESET_PIN, HIGH);
	delay(250);
	if (_res == _135x240 || _res == _240x135)
	{
		digitalWrite(ST77XXSPI_BACK_LIGHT_PIN, HIGH);
	}
	
	if (_res == _135x240 || _res == _240x135) // ST7789-TTGO
	{
		ST77XXSPI_LCD_WRITE_COM(0xE0);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);

		ST77XXSPI_LCD_WRITE_COM(0x01);	//Power Control 1
		delay(150);

		ST77XXSPI_LCD_WRITE_COM(0x11);	//Power Control 2
		delay(255);

		ST77XXSPI_LCD_WRITE_COM(0x3A);	// VCOM Control 1
		ST77XXSPI_LCD_WRITE_DATA8(0x55);
		delay(10);

		ST77XXSPI_LCD_WRITE_COM(0x36);	// MADCTL : Memory Data Access Control
		if (Xoffset == ST7789_X_OFFSET)
		{
			ST77XXSPI_LCD_WRITE_DATA8(0x00);
		}
		else
		{
			ST77XXSPI_LCD_WRITE_DATA8(0xB0);
		}

		ST77XXSPI_LCD_WRITE_COM(0x2A);	//Memory Access Control
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0xF0);

		ST77XXSPI_LCD_WRITE_COM(0x2B);	//Pixel Format Set
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0xF0);

		ST77XXSPI_LCD_WRITE_COM(0x21);	//Display Inversion OFF
		delay(10);

		ST77XXSPI_LCD_WRITE_COM(0x13);	//Frame Rate Control
		delay(10);

		ST77XXSPI_LCD_WRITE_COM(0x29);	//Display ON
		delay(255);
	}

	if (_res == _320x240) // ILI9341
	{
		ST77XXSPI_LCD_WRITE_COM(0x11);//sleep out 
		delay(20);
		ST77XXSPI_LCD_WRITE_COM(0x28); //display off
		delay(5);
		ST77XXSPI_LCD_WRITE_COM(0xCF); //power control b
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x83); //83 81 AA
		ST77XXSPI_LCD_WRITE_DATA8(0x30);
		ST77XXSPI_LCD_WRITE_COM(0xED); //power on seq control
		ST77XXSPI_LCD_WRITE_DATA8(0x64); //64 67
		ST77XXSPI_LCD_WRITE_DATA8(0x03);
		ST77XXSPI_LCD_WRITE_DATA8(0x12);
		ST77XXSPI_LCD_WRITE_DATA8(0x81);
		ST77XXSPI_LCD_WRITE_COM(0xE8); //timing control a
		ST77XXSPI_LCD_WRITE_DATA8(0x85);
		ST77XXSPI_LCD_WRITE_DATA8(0x01);
		ST77XXSPI_LCD_WRITE_DATA8(0x79); //79 78
		ST77XXSPI_LCD_WRITE_COM(0xCB); //power control a
		ST77XXSPI_LCD_WRITE_DATA8(0x39);
		ST77XXSPI_LCD_WRITE_DATA8(0X2C);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x34);
		ST77XXSPI_LCD_WRITE_DATA8(0x02);
		ST77XXSPI_LCD_WRITE_COM(0xF7); //pump ratio control
		ST77XXSPI_LCD_WRITE_DATA8(0x20);
		ST77XXSPI_LCD_WRITE_COM(0xEA); //timing control b
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_COM(0xC0); //power control 2
		ST77XXSPI_LCD_WRITE_DATA8(0x26); //26 25
		ST77XXSPI_LCD_WRITE_COM(0xC1); //power control 2
		ST77XXSPI_LCD_WRITE_DATA8(0x11);
		ST77XXSPI_LCD_WRITE_COM(0xC5); //vcom control 1
		ST77XXSPI_LCD_WRITE_DATA8(0x35);
		ST77XXSPI_LCD_WRITE_DATA8(0x3E);
		ST77XXSPI_LCD_WRITE_COM(0xC7); //vcom control 2
		ST77XXSPI_LCD_WRITE_DATA8(0xBE); //BE 94
		ST77XXSPI_LCD_WRITE_COM(0xB1); //frame control
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_DATA8(0x1B); //1B 70
		ST77XXSPI_LCD_WRITE_COM(0xB6); //display control
		ST77XXSPI_LCD_WRITE_DATA8(0x0A);
		ST77XXSPI_LCD_WRITE_DATA8(0x82);
		ST77XXSPI_LCD_WRITE_DATA8(0x27);
		ST77XXSPI_LCD_WRITE_DATA8(0x00);
		ST77XXSPI_LCD_WRITE_COM(0xB7); //emtry mode
		ST77XXSPI_LCD_WRITE_DATA8(0x07);
		ST77XXSPI_LCD_WRITE_COM(0x3A); //pixel format
		ST77XXSPI_LCD_WRITE_DATA8(0x55); //16bit
		ST77XXSPI_LCD_WRITE_COM(0x36); //  5: Memory access ctrl (directions), 1 arg:
		ST77XXSPI_LCD_WRITE_DATA8(0b11111100);// Row addr/col addr, bottom to top refresh
		ST77XXSPI_LCD_WRITE_COM(0x29); //display on
		delay(5);
	}
}

void ST77XX::drawPixel(short x, short y)
{
	setXY(x, y);
	ST77XXSPI_LCD_WRITE_DATA(fg565);
}

void ST77XX::fillScr(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short color = (((b >> 3) | (g << 5)) << 8) | ((r & 0xF8) | (g >> 5));
	unsigned int tempC = (color << 16) | color;

	setXY(0, 0, maxX - 1, maxY - 1);
	//SPI.writeShort(color, maxX * maxY);
	SPI.writeUINT(tempC, maxX * maxY >> 1);
}

void ST77XX::drawHLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		x -= l;
	}
	setXY(x, y, x + l, y);
	SPI.writeShort(fg565, l);
}

void ST77XX::drawVLine(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	setXY(x, y, x, y + l);
	SPI.writeShort(fg565, l);
}

#define DRAW_PIX_8BIT(X,Y,PIX_DATA)\
		if (X < maxX && Y < maxY) {\
			setXY(X, Y);\
			ST77XXSPI_LCD_WRITE_DATA(colorTable[(PIX_DATA) & 0x7]);\
		}

void ST77XX::draw8bBitMap(short x, short y, const unsigned char * dataArray, bool useSkipBit, flipOption flipOpt)
{
	unsigned char pixelData;
	unsigned short width, hight;
	unsigned int index = 4, size;
	width = (dataArray[0] << 8) | dataArray[1];
	hight = (dataArray[2] << 8) | dataArray[3];
	size = (width / 2 + (width & 01 == 1)) * hight;

	if (flipOpt == flipY || flipOpt == flipXY)
	{
		for (int iy = (y + hight - 1); iy >= y; iy--)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_8BIT(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_8BIT(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	else
	{
		for (int iy = y; iy < (y + hight); iy++)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_8BIT(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_8BIT(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
}

void ST77XX::drawCompressed24bitBitmap(short x, short y, const unsigned int * dataArray)
{
	unsigned int	hight, width;
	unsigned int	buffer;
	int				index = 0;
	unsigned short  rgb565;
	unsigned char	r, g, b;

	width = dataArray[index];
	index++;

	buffer = dataArray[index];
	hight = buffer;
	index++;

	unsigned int dataArraySize = hight * width, i, j, counter = 0;
	unsigned char copies;

	setXY(x, y, x + width - 1, y + hight - 1);
	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		//SPI.writeRGB(buffer & 0x00ffffff, copies);
		rgb565 = rgbTo565((buffer & 0x000000ff), (buffer & 0x0000ffff) >> 8, (buffer & 0x00ffffff) >> 16);
		SPI.writeShort(rgb565, copies);
		counter += copies;
	}
}

/***************** ST7789 - 240x135 Frame Buffer mode ****************************/

ST77XX_FB::~ST77XX_FB()
{
	if (!frameBuffer)
		delete[] frameBuffer;
}

bool ST77XX_FB::init(unsigned int freq)
{
	frameBuffer = new unsigned short[240*135];
	if (!frameBuffer)
	{
		printf("Cannot allocate frame buffer!\n");
		return false;
	}
	ST77XX::init(freq);

	return true;
}

void ST77XX_FB::drawPixel(short x, short y)
{
	if (x < 240 && y < 135)
	{
		frameBuffer[y * 240 + x] = fg565;
	}
}

void ST77XX_FB::fillScr(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short color = (((b >> 3) | (g << 5)) << 8) | ((r & 0xF8) | (g >> 5));
	unsigned int tempC = (color << 16) | color;
	unsigned int * tempPointer = (unsigned int *)(frameBuffer);
	
	for (size_t i = 0; i < 240*135/2; i++)
	{
		tempPointer[i] = tempC;
	}
}

void ST77XX_FB::flushFB()
{
	setXY(0, 0, maxX - 1, maxY - 1);
	SPI.writeBuffer((unsigned int *)(frameBuffer), 240*135/2);
}

void ST77XX_FB::drawHLine(short x, short y, int l)
{
	unsigned int fixedL, i;
	if (x + l >= maxX)
	{
		fixedL = maxX;
	}
	else
		fixedL = (x + l);
	for (i = x; i < fixedL; i++)
	{
		frameBuffer[240 * y + i] = fg565;
	}
}

void ST77XX_FB::drawVLine(short x, short y, int l)
{
	unsigned int fixedL, i;
	if (y + l >= maxX)
	{
		fixedL = maxY;
	}
	else
		fixedL = (y + l);
	for (i = y; i < fixedL; i++)
	{
		frameBuffer[240 * i + x] = fg565;
	}
}

#define DRAW_PIX_FAST(X,Y,PIX_DATA)\
		if (X < maxX && Y < maxY) {\
		frameBuffer[240*Y + X] = colorTable[(PIX_DATA) & 0x7];}

void ST77XX_FB::draw8bBitMap(short x, short y, const unsigned char * dataArray, bool useSkipBit, flipOption flipOpt)
{
	unsigned char pixelData;
	unsigned short width, hight;
	unsigned int index = 4, size;
	width = (dataArray[0] << 8) | dataArray[1];
	hight = (dataArray[2] << 8) | dataArray[3];
	size = (width / 2 + (width & 01 == 1)) * hight;

	if (flipOpt == flipY || flipOpt == flipXY)
	{
		for (int iy = (y + hight - 1); iy >= y; iy--)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_FAST(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_FAST(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_FAST(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_FAST(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	else
	{
		for (int iy = y; iy < (y + hight); iy++)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_FAST(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_FAST(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						DRAW_PIX_FAST(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						DRAW_PIX_FAST(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
}

void ST77XX_FB::drawCompressed24bitBitmap(short x, short y, const unsigned int * dataArray)
{
	unsigned int	hight, width;
	unsigned int	buffer;
	int				index = 0;
	unsigned short  tempX = x,tempY = y;
	unsigned char	r, g, b;

	width = dataArray[index];
	index++;

	buffer = dataArray[index];
	hight = buffer;
	index++;

	unsigned int dataArraySize = hight * width, i, j, counter = 0;
	unsigned char copies;

	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		fg565 = rgbTo565((buffer & 0x000000ff), (buffer & 0x0000ffff) >> 8, (buffer & 0x00ffffff) >> 16);
		for (int j = 0; j < copies; j++)
		{
			drawPixel(tempX, tempY);
			tempX++;
			if (tempX == (x + width))
			{
				tempY++;
				tempX = x;
			}
		}
		counter += copies;
	}
}

///////////////////////////////////// TVout /////////////////////////////////////////
bool TVout::init()
{
	bool error = false;
	int y, i;
	displayFrame = (char**)malloc(maxY * sizeof(char*));
	if (displayFrame == NULL)
	{
		return false;
	}
	workFrame = (char**)malloc(maxY * sizeof(char*));
	if (workFrame == NULL)
	{
		free(displayFrame);
		return false;
	}
	for (y = 0; y < maxY; y++)
	{
		displayFrame[y] = (char*)calloc(maxX,1);
		workFrame[y] = (char*)calloc(maxX,1);
		if (displayFrame[y] == NULL || workFrame[y] == NULL)
		{
			error = true;
			break;
		}
	}
	if (error)
	{
		for (size_t i = 0; i < y; i++)
		{
			free(displayFrame[i]);
			free(workFrame[i]);
		}
		return false;
	}
	
	// 8 bit bitmap color table
	int tempColor;
	tempColor = rgb888TO444(0, 0, 0);
	_fgColorYUVarray[0] = RGB2YUV[tempColor];
	_fgYUVc8array[0] = _fgColorYUVarray[0] >> 8;

	tempColor = rgb888TO444(0, 0, 255);
	_fgColorYUVarray[1] = RGB2YUV[tempColor];
	_fgYUVc8array[1] = _fgColorYUVarray[1] >> 8;

	tempColor = rgb888TO444(0, 255, 0);
	_fgColorYUVarray[2] = RGB2YUV[tempColor];
	_fgYUVc8array[2] = _fgColorYUVarray[2] >> 8;

	tempColor = rgb888TO444(0, 255, 255);
	_fgColorYUVarray[3] = RGB2YUV[tempColor];
	_fgYUVc8array[3] = _fgColorYUVarray[3] >> 8;

	tempColor = rgb888TO444(255, 0, 0);
	_fgColorYUVarray[4] = RGB2YUV[tempColor];
	_fgYUVc8array[4] = _fgColorYUVarray[4] >> 8;

	tempColor = rgb888TO444(255, 0, 255);
	_fgColorYUVarray[5] = RGB2YUV[tempColor];
	_fgYUVc8array[5] = _fgColorYUVarray[5] >> 8;

	tempColor = rgb888TO444(255, 255, 0);
	_fgColorYUVarray[6] = RGB2YUV[tempColor];
	_fgYUVc8array[6] = _fgColorYUVarray[6] >> 8;

	tempColor = rgb888TO444(255, 255, 255);
	_fgColorYUVarray[7] = RGB2YUV[tempColor];
	_fgYUVc8array[7] = _fgColorYUVarray[7] >> 8;

	
	//////////// I2S Related /////////////

	for (i = 0; i < syncSamples; i++)
	{
		shortSync[i ^ 1] = syncLevel << 8;
		longSync[(lineSamples - syncSamples + i) ^ 1] = blankLevel << 8;
		line[0][i ^ 1] = syncLevel << 8;
		line[1][i ^ 1] = syncLevel << 8;
		blank[i ^ 1] = syncLevel << 8;
	}
	for (i = 0; i < lineSamples - syncSamples; i++)
	{
		shortSync[(i + syncSamples) ^ 1] = blankLevel << 8;
		longSync[(lineSamples - syncSamples + i) ^ 1] = syncLevel << 8;
		line[0][(i + syncSamples) ^ 1] = blankLevel << 8;
		line[1][(i + syncSamples) ^ 1] = blankLevel << 8;
		blank[(i + syncSamples) ^ 1] = blankLevel << 8;
	}
	for (i = 0; i < burstSamples; i++)
	{
		int p = burstStart + i;
		unsigned short b0 = ((short)(blankLevel + sin(i * burstPerSample + burstPhase) * burstAmp)) << 8;
		unsigned short b1 = ((short)(blankLevel + sin(i * burstPerSample - burstPhase) * burstAmp)) << 8;
		line[0][p ^ 1] = b0;
		line[1][p ^ 1] = b1;
		blank[p ^ 1] = b0;
	}

	for (i = 0; i < imageSamples; i++)
	{
		int p = frameStart + i;
		int c = p - burstStart;
		SIN[i] = round(0.436798 * sin(c * burstPerSample) * 256);
		COS[i] = round(0.614777 * cos(c * burstPerSample) * 256);
	}

	for (i = 0; i < 16; i++)
	{
		YLUT[i] = (blankLevel << 8) + round(i / 15. * 256 * maxLevel);
		UVLUT[i] = round((i - 8) / 7. * maxLevel);
	}

	i2s_config_t i2s_config = {
   .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
   .sample_rate = 1000000,  //not really used
   .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT,
   .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
   .communication_format = I2S_COMM_FORMAT_I2S_MSB,
   .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
   .dma_buf_count = 10,
   .dma_buf_len = lineSamples   //a buffer per line
	};

	i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);    //start i2s driver
	i2s_set_pin(I2S_PORT, NULL);                           //use internal DAC
	i2s_set_sample_rates(I2S_PORT, 1000000);               //dummy sample rate, since the function fails at high values

	//this is the hack that enables the highest sampling rate possible ~13MHz, have fun
	SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
	SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
	SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S);
	SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);

	//untie DACs
	SET_PERI_REG_BITS(I2S_CONF_CHAN_REG(0), I2S_TX_CHAN_MOD_V, 3, I2S_TX_CHAN_MOD_S);
	SET_PERI_REG_BITS(I2S_FIFO_CONF_REG(0), I2S_TX_FIFO_MOD_V, 1, I2S_TX_FIFO_MOD_S);

	return true;
}

void TVout::drawPixel(short x, short y)
{
	if (x < maxX && y < maxY)
	{
		if (y & 1)
		{
			workFrame[y & ~1][x] = (workFrame[y & ~1][x] & 0xf) | (_fgColorYUV & 0xf0);
			workFrame[y][x] = _fgYUVc8;
		}
		else
		{
			workFrame[y][x] = _fgColorYUV;
			workFrame[y | 1][x] = (workFrame[y | 1][x] & 0xf) | ((_fgYUVc8) & 0xf0);
		}
	}
}

void TVout::drawHLine(short x, short y, int l)
{
	unsigned int fixedL,i;
	if (x + l >= maxX)
	{
		fixedL = maxX;
	}
	else
		fixedL = (x + l);
	
	for (i = x; i < fixedL; i++)
	{
		if (y & 1)
		{
			workFrame[y & ~1][i] = (workFrame[y & ~1][i] & 0xf) | (_fgColorYUV & 0xf0);
			workFrame[y][i] = _fgColorYUV >> 8;
		}
		else
		{
			workFrame[y][i] = _fgColorYUV;
			workFrame[y | 1][i] = (workFrame[y | 1][i] & 0xf) | ((_fgColorYUV >> 8) & 0xf0);
		}
	}
}

void TVout::fillScr(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int temp = _fgColor;
	setColor(r, g, b);
	/*for (int y = 0; y < maxY; y++)
		for (int x = 0; x < maxX; x++)
			dotFast(x, y);*/
	dotFast(0, 0);
	dotFast(0, 1);
	unsigned char data0, data1;
	data0 = workFrame[0][0];
	data1 = workFrame[1][0];
	unsigned int wData0 = (data0 << 8) | (data0 << 16) | (data0 << 24) | data0;
	unsigned int wData1 = (data1 << 8) | (data1 << 16) | (data1 << 24) | data1;
	unsigned int * pointr0;
	unsigned int * pointr1;

	for (int y = 0; y < maxY; y+=2)
	{
		pointr0 = (unsigned int *)workFrame[y];
		pointr1 = (unsigned int *)workFrame[y+1];
		for (int x = 0; x < maxX/4; x++)
		{
			pointr0[x] = wData0;
			pointr1[x] = wData1;
		}
	}
	_fgColor = temp;
}

void TVout::swapFrameBuffers()
{
	char **b = workFrame;
	workFrame = displayFrame;
	displayFrame = b;
}

void TVout::sendFrame()
{
	int l = 0;
	//long sync
	for (int i = 0; i < 3; i++)
		sendLine(longSync);
	//short sync
	for (int i = 0; i < 4; i++)
		sendLine(shortSync);
	//blank lines
	for (int i = 0; i < 37; i++)
		sendLine(blank);

	//image
	for (int i = 0; i < 240; i += 2)
	{
		char *pixels0 = displayFrame[i];
		char *pixels1 = displayFrame[i + 1];
		int j = frameStart;
		for (int x = 0; x < imageSamples; x += 2)
		{
			unsigned short p0 = *(pixels0++);
			unsigned short p1 = *(pixels1++);
			short y0 = YLUT[p0 & 15];
			short y1 = YLUT[p1 & 15];
			unsigned char p04 = p0 >> 4;
			unsigned char p14 = p1 >> 4;
			short u0 = (SIN[x] * UVLUT[p04]);
			short u1 = (SIN[x + 1] * UVLUT[p04]);
			short v0 = (COS[x] * UVLUT[p14]);
			short v1 = (COS[x + 1] * UVLUT[p14]);
			//word order is swapped for I2S packing (j + 1) comes first then j
			line[0][j] = y0 + u1 + v1;
			line[1][j] = y1 + u1 - v1;
			line[0][j + 1] = y0 + u0 + v0;
			line[1][j + 1] = y1 + u0 - v0;
			j += 2;
		}
		sendLine(line[0]);
		sendLine(line[1]);
	}

	for (int i = 0; i < 25; i++)
		sendLine(blank);
	for (int i = 0; i < 3; i++)
		sendLine(shortSync);
}

void TVout::drawCompressed24bitBitmap(short x, short y, const unsigned int * dataArray)
{
	unsigned int	hight, width;
	unsigned int	buffer;
	int				index = 0;
	unsigned short  tempX = x, tempY = y;
	unsigned char	r, g, b;

	width = dataArray[index];
	index++;

	buffer = dataArray[index];
	hight = buffer;
	index++;

	unsigned int dataArraySize = hight * width, i, j, counter = 0;
	unsigned char copies;

	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		setColor((buffer & 0x000000ff), (buffer & 0x0000ffff) >> 8, (buffer & 0x00ffffff) >> 16);
		for (int j = 0; j < copies; j++)
		{
			drawPixel(tempX, tempY);
			tempX++;
			if (tempX == (x + width))
			{
				tempY++;
				tempX = x;
			}
		}
		counter += copies;
	}
}
/*
if (y & 1)
{
	workFrame[y & ~1][x] = (workFrame[y & ~1][x] & 0xf) | (_fgColorYUV & 0xf0);
	workFrame[y][x] = _fgYUVc8;
}
else
{
	workFrame[y][x] = _fgColorYUV;
	workFrame[y | 1][x] = (workFrame[y | 1][x] & 0xf) | ((_fgYUVc8) & 0xf0);
}
*/
#define TV_DRAW_PIX_8BIT(X,Y,PIX_DATA)\
		if (X < maxX && Y < maxY) {\
			if (Y & 1)\
			{\
				workFrame[Y & ~1][X] = (workFrame[Y & ~1][X] & 0xf) | (_fgColorYUVarray[(PIX_DATA) & 0x7] & 0xf0);\
				workFrame[Y][X] = _fgYUVc8array[(PIX_DATA) & 0x7];\
			}\
			else\
			{\
				workFrame[Y][X] = _fgColorYUVarray[(PIX_DATA) & 0x7];\
				workFrame[Y | 1][X] = (workFrame[Y | 1][X] & 0xf) | ((_fgYUVc8array[(PIX_DATA) & 0x7]) & 0xf0);\
			}\
		}

void TVout::draw8bBitMap(short x, short y, const unsigned char * dataArray, bool useSkipBit, flipOption flipOpt)
{
	unsigned char pixelData;
	unsigned short width, hight;
	unsigned int index = 4, size;
	width = (dataArray[0] << 8) | dataArray[1];
	hight = (dataArray[2] << 8) | dataArray[3];
	size = (width / 2 + (width & 01 == 1)) * hight;

	if (flipOpt == flipY || flipOpt == flipXY)
	{
		for (int iy = (y + hight - 1); iy >= y; iy--)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						TV_DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						TV_DRAW_PIX_8BIT(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						TV_DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						TV_DRAW_PIX_8BIT(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	else
	{
		for (int iy = y; iy < (y + hight); iy++)
		{
			if (flipOpt == flipX || flipOpt == flipXY)
			{
				for (int ix = (x + width - 1); ix >= x; ix -= 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						TV_DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						TV_DRAW_PIX_8BIT(ix - 1, iy, pixelData);
					}
				}
			}
			else
			{
				for (int ix = x; ix < (x + width); ix += 2)
				{
					pixelData = dataArray[index++];
					if (((pixelData & 0x80) != 0x80) || !useSkipBit) // First pixel
					{
						TV_DRAW_PIX_8BIT(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						TV_DRAW_PIX_8BIT(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
}