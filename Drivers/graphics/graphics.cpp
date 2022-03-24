#include "graphics.h"
#include "SPI.h"
#include "Fonts/fonts.h"
#include "Images.c" // Heb Fonts / Bitmaps + numbers
#include "arduino.h"
#define ESP32

/*
 General graphics lib to be inherited
*/
unsigned char _cmdDataPin;

unsigned char graphics::fgColor[3] = { 0,0,0 };
unsigned char graphics::bgColor[3];
int	graphics::_fgColor = 0, graphics::_bgColor = 0;
unsigned short graphics::fg565 = 0, graphics::bg565 = 0;
short graphics::maxX = 0, graphics::maxY = 0;
customFont * graphics::currentFonts = NULL;
fontType graphics::fontTypeLoaded = ORBITRON_LIGHT24;
_drawPixel graphics::drawPixel = NULL;
_fillScr graphics::fillScr = NULL;
_drawHLine graphics::drawHLine = NULL;
_drawVLine graphics::drawVLine = NULL;
_draw8C_Pixel graphics::draw8C_Pixel = NULL;
_drawCompressed24bitBitmap graphics::drawCompressed24bitBitmap = NULL;
unsigned short graphics::colorTable565[8];
unsigned int graphics::colorTableRGB[8];
bool graphics::collision;

graphics::graphics(short maxX, short maxY)
{	
	this->maxX = maxX;
	this->maxY = maxY;
	// Load default fonts
	fontTypeLoaded = ORBITRON_LIGHT24;
	currentFonts = &orbitronLight24Font;
	drawPixel = Dummy_drawPixel;
	fillScr = Dummy_fillScr;
	drawHLine = Dummy_drawHLine;
	drawVLine = Dummy_drawVLine;
	_fgColor = 0, _bgColor = 0;
	fg565 = 0, bg565 = 0;
	colorTable565[0] = rgbTo565(0, 0, 0);
	colorTable565[1] = rgbTo565(0, 0, 0xff);
	colorTable565[2] = rgbTo565(0, 0xff, 0);
	colorTable565[3] = rgbTo565(0, 0xff, 0xff);
	colorTable565[4] = rgbTo565(0xff, 0, 0);
	colorTable565[5] = rgbTo565(0xff, 0, 0xff);
	colorTable565[6] = rgbTo565(0xff, 0xff, 0);
	colorTable565[7] = rgbTo565(0xff, 0xff, 0xff);
	colorTableRGB[0] = rgbToInt(0, 0, 0);
	colorTableRGB[1] = rgbToInt(0, 0, 0xff);
	colorTableRGB[2] = rgbToInt(0, 0xff, 0);
	colorTableRGB[3] = rgbToInt(0, 0xff, 0xff);
	colorTableRGB[4] = rgbToInt(0xff, 0, 0);
	colorTableRGB[5] = rgbToInt(0xff, 0, 0xff);
	colorTableRGB[6] = rgbToInt(0xff, 0xff, 0);
	colorTableRGB[7] = rgbToInt(0xff, 0xff, 0xff);
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
	if (x2 >= maxX)
	{
		x2 = maxX - 1;
	}
	if (y2 >= maxY)
	{
		y2 = maxY - 1;
	}
	if (fill)
	{
		for (int i = 0; i < ((y2 - y1) / 2) + 1; i++)
		{
			drawHLine(x1, y1 + i, x2 - x1 + 1);
			drawHLine(x1, y2 - i, x2 - x1 + 1);
		}
	}
	else
	{
		drawHLine(x1, y1, x2 - x1 + 1);
		drawHLine(x1, y2, x2 - x1 + 1);
		drawVLine(x1, y1, y2 - y1 + 1);
		drawVLine(x2, y1, y2 - y1 + 1);
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

	if (x2 >= maxX)
	{
		x2 = maxX - 1;
	}
	if (y2 >= maxY)
	{
		y2 = maxY - 1;
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

void graphics::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	fgColor[2] = b;
	fgColor[1] = g;
	fgColor[0] = r;
	_fgColor = ((int)b << 16) | ((int)g << 8) | r;
	fg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}
unsigned int graphics::rgbToInt(unsigned char r, unsigned char g, unsigned char b)
{
	return ((int)b << 16) | ((int)g << 8) | r;
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
bool graphics::draw8bBitMap(short x, short y, const unsigned char * dataArray, bool useSkipBit, flipOption flipOpt)
{
	unsigned char pixelData;
	unsigned short width, hight;
	unsigned int index = 4, size;
	width = (dataArray[0] << 8) | dataArray[1];
	hight = (dataArray[2] << 8) | dataArray[3];
	size = (width / 2 + (width & 01 == 1)) * hight;
	collision = false;
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
						draw8C_Pixel(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						draw8C_Pixel(ix - 1, iy, pixelData);
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
						draw8C_Pixel(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						draw8C_Pixel(ix + 1, iy, pixelData);
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
						draw8C_Pixel(ix, iy, pixelData >> 4);
					}
					if (((pixelData & 0x08) != 0x08) && (ix < (x + width)) || !useSkipBit) // Second Pixel
					{
						draw8C_Pixel(ix - 1, iy, pixelData);
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
						draw8C_Pixel(ix, iy, pixelData >> 4);
					}
					if ((((pixelData & 0x08) != 0x08) && ((ix + 1) < (x + width))) || !useSkipBit) // Second Pixel
					{
						draw8C_Pixel(ix + 1, iy, pixelData);
					}
				}
			}
		}
	}
	return collision;
}


//////////////////////////////////// SPI LCD ////////////////////////////////////
_setXY2 BASE_SPI_LCD::setXY2 = NULL;
_setXY BASE_SPI_LCD::setXY = NULL;
unsigned short	BASE_SPI_LCD::Xoffset, BASE_SPI_LCD::Yoffset;

// Common functions
void BASE_SPI_LCD::setXY2gen(short x1, short y1, short x2, short y2)
{
	unsigned int ex = x2 | (x1 << 16);
	unsigned int ey = y2 | (y1 << 16);

	LCD_WRITE_COM(0x2a);
	SPI.writeUINT(ex);
	LCD_WRITE_COM(0x2b);
	SPI.writeUINT(ey);
	LCD_WRITE_COM(0x2c);
}
void BASE_SPI_LCD::setXY2wOffset(short x1, short y1, short x2, short y2)
{
	x1 += Xoffset;
	y1 += Yoffset;
	x2 += Xoffset;
	y2 += Yoffset;

	int ex = x2 | (x1 << 16);
	int ey = y2 | (y1 << 16);

	LCD_WRITE_COM(0x2a);
	SPI.writeUINT(ex);
	LCD_WRITE_COM(0x2b);
	SPI.writeUINT(ey);
	LCD_WRITE_COM(0x2c);
}
void BASE_SPI_LCD::setXYgen(short x, short y)
{
	static unsigned short lastX = 1000, lastY = 1000;
	if (lastX != x)
	{
		unsigned int ex = x | (x << 16);
		LCD_WRITE_COM(0x2a);
		SPI.writeUINT(ex); // Need to optimize !
	}
	if (lastY != y)
	{
		unsigned int ey = y | (y << 16);
		LCD_WRITE_COM(0x2b);
		SPI.writeUINT(ey);
	}
	LCD_WRITE_COM(0x2c);
	lastX = x;
	lastY = y;
}
void BASE_SPI_LCD::setXYwOffset(short x, short y)
{
	static short lastX = 1000, lastY = 1000;
	x += Xoffset;
	y += Yoffset;
	if (lastX != x)
	{
		int ex = x | (x << 16);
		LCD_WRITE_COM(0x2a);
		SPI.writeUINT(ex); // Need to optimize !
	}
	if (lastY != y)
	{
		int ey = y | (y << 16);
		LCD_WRITE_COM(0x2b);
		SPI.writeUINT(ey);
	}
	LCD_WRITE_COM(0x2c);
	lastX = x;
	lastY = y;
}

void BASE_SPI_LCD::drawPixel24(short x, short y)
{
	setXY(x, y);
	LCD_WRITE_DATA32(_fgColor);
}
void BASE_SPI_LCD::drawPixel16(short x, short y)
{
	setXY(x, y);
	LCD_WRITE_DATA16(fg565);
}
void BASE_SPI_LCD::fillScr16(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short color = (((b >> 3) | (g << 5)) << 8) | ((r & 0xF8) | (g >> 5));
	unsigned int tempC = (color << 16) | color;

	setXY2(0, 0, maxX - 1, maxY - 1);
	SPI.writeUINT(tempC, maxX * maxY >> 1);
}
void BASE_SPI_LCD::fillScr24(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int color = ((unsigned int)b << 16) | ((unsigned int)g << 8) | (unsigned int)r;

	setXY2(0, 0, maxX - 1, maxY - 1);
	SPI.writeRGB(color, maxX * maxY);
}
#define DATA_START_OFFSET 2
void BASE_SPI_LCD::draw8C_Pixel16(short x, short y, unsigned char pixelData)
{
	if (x < maxX && y < maxY)
	{
		setXY(x, y);
		LCD_WRITE_DATA16(colorTable565[(pixelData) & 0x7]);
	}
}
void BASE_SPI_LCD::draw8C_Pixel24(short x, short y, unsigned char pixelData)
{
	if (x < maxX && y < maxY)
	{
		setXY(x, y);
		LCD_WRITE_DATA32(colorTableRGB[(pixelData) & 0x7]);
	}
}
void BASE_SPI_LCD::drawHLine16(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		x -= l;
	}
	setXY2(x, y, x + l, y);
	SPI.writeShort(fg565, l);
}
void BASE_SPI_LCD::drawHLine24(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		x -= l;
	}
	setXY2(x, y, x + l, y);
	SPI.writePattern(fgColor, 3, l);
}
void BASE_SPI_LCD::drawVLine16(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	setXY2(x, y, x, y + l);
	SPI.writeShort(fg565, l);
}
void BASE_SPI_LCD::drawVLine24(short x, short y, int l)
{
	if (l < 0)
	{
		l = -l;
		y -= l;
	}
	setXY2(x, y, x, y + l);
	SPI.writePattern(fgColor, 3, l);
}

void SPI_LCD::drawCompressed24bitBitmap_16(short x, short y, const unsigned int * dataArray)
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

	setXY2(x, y, x + width - 1, y + hight - 1);
	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		rgb565 = rgbTo565((buffer & 0x000000ff), (buffer & 0x0000ffff) >> 8, (buffer & 0x00ffffff) >> 16);
		SPI.writeShort(rgb565, copies);
		counter += copies;
	}
}
void SPI_LCD::drawCompressed24bitBitmap_24(short x, short y, const unsigned int * dataArray)
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

	setXY2(x, y, x + width - 1, y + hight - 1);
	for (i = DATA_START_OFFSET; counter < dataArraySize; i++)
	{
		buffer = dataArray[index];
		index++;
		copies = (buffer >> 24);
		SPI.writeRGB(buffer & 0x00ffffff, copies);
		counter += copies;
	}
}
bool SPI_LCD::init(lcdControllerType cntrlType, lcdHwAccessor * lcdHwAcc, unsigned char cmdDataPin, unsigned char mosiPin, unsigned char clockPin, unsigned int freq)
{
	_cmdDataPin = cmdDataPin;
	pinMode(cmdDataPin, OUTPUT);

	SPI.begin((char)clockPin, (char)-1, (char)mosiPin, (char)-1);
	SPI.setBitOrder(MSBFIRST);
	SPI.setFrequency(freq);
	SPI.setDataMode(3);
#ifdef ESP32
	ESP32_SPI_DIS_MOSI_MISO_FULL_DUPLEX(ESP32_VSPI);
#endif

	lcdHwAcc->setup();
	SELECT_DATA_COMM(LCD_DATA);
	lcdHwAcc->assertCS();
	lcdHwAcc->reset();
	lcdHwAcc->backLightOn();

	if (cntrlType == ili9488_480x320x24)
	{
		maxX = 480;
		maxY = 320;
		drawPixel = drawPixel24;
		fillScr = fillScr24;
		drawHLine = drawHLine24;
		drawVLine = drawVLine24;
		setXY = setXYgen;
		setXY2 = setXY2gen;
		draw8C_Pixel = draw8C_Pixel24;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_24;

		LCD_WRITE_COM(0xE0);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x03);
		LCD_WRITE_DATA8(0x09);
		LCD_WRITE_DATA8(0x08);
		LCD_WRITE_DATA8(0x16);
		LCD_WRITE_DATA8(0x0A);
		LCD_WRITE_DATA8(0x3F);
		LCD_WRITE_DATA8(0x78);
		LCD_WRITE_DATA8(0x4C);
		LCD_WRITE_DATA8(0x09);
		LCD_WRITE_DATA8(0x0A);
		LCD_WRITE_DATA8(0x08);
		LCD_WRITE_DATA8(0x16);
		LCD_WRITE_DATA8(0x1A);
		LCD_WRITE_DATA8(0x0F);

		LCD_WRITE_COM(0XE1);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x16);
		LCD_WRITE_DATA8(0x19);
		LCD_WRITE_DATA8(0x03);
		LCD_WRITE_DATA8(0x0F);
		LCD_WRITE_DATA8(0x05);
		LCD_WRITE_DATA8(0x32);
		LCD_WRITE_DATA8(0x45);
		LCD_WRITE_DATA8(0x46);
		LCD_WRITE_DATA8(0x04);
		LCD_WRITE_DATA8(0x0E);
		LCD_WRITE_DATA8(0x0D);
		LCD_WRITE_DATA8(0x35);
		LCD_WRITE_DATA8(0x37);
		LCD_WRITE_DATA8(0x0F);

		LCD_WRITE_COM(0XC0);     //Power Control 1
		LCD_WRITE_DATA8(0x17);    //Vreg1out
		LCD_WRITE_DATA8(0x15);    //Verg2out

		LCD_WRITE_COM(0xC1);     //Power Control 2
		LCD_WRITE_DATA8(0x41);    //VGH,VGL

		LCD_WRITE_COM(0xC5);     //Power Control 3
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x12);    //Vcom
		LCD_WRITE_DATA8(0x80);

		LCD_WRITE_COM(0x36);      //Memory Access
		//LCD_WRITE_DATA8(0x4A);
								 ////76543210
		LCD_WRITE_DATA8(0b11101010);

		LCD_WRITE_COM(0x3A);      // Interface Pixel Format

		/*if (mode == rgb666)
		{
			LCD_WRITE_DATA8(0x66); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits is not supported in SPI mode!
		}
		else
		{
			LCD_WRITE_DATA8(0x11); 	  // 0x11 = 3 bits (3 bits per pixel)
		}*/
		LCD_WRITE_DATA8(0x66); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits is not supported in SPI mode! 

		LCD_WRITE_COM(0XB0);      // Interface Mode Control
		LCD_WRITE_DATA8(0x80);     //SDO NOT USE

		LCD_WRITE_COM(0xB1);      //Frame rate
		LCD_WRITE_DATA8(0xA0);    //60Hz

		LCD_WRITE_COM(0xB4);      //Display Inversion Control
		LCD_WRITE_DATA8(0x02);    //2-dot

		LCD_WRITE_COM(0XB6);     //Display Function Control  RGB/MCU Interface Control

		LCD_WRITE_DATA8(0x02);    //MCU
		LCD_WRITE_DATA8(0x02);    //Source,Gate scan direction

		LCD_WRITE_COM(0XE9);     // Set Image Function
		LCD_WRITE_DATA8(0x00);    // Disable 24 bit data

		LCD_WRITE_COM(0xF7);      // Adjust Control
		LCD_WRITE_DATA8(0xA9);
		LCD_WRITE_DATA8(0x51);
		LCD_WRITE_DATA8(0x2C);
		LCD_WRITE_DATA8(0x82);    // D7 stream, loose

		LCD_WRITE_COM(0x11);    //Exit Sleep
		delay(120);
		LCD_WRITE_COM(0x29);    //Display on
	}
	if (cntrlType == ili9481_480x320x24)
	{
		maxX = 480;
		maxY = 320;
		drawPixel = drawPixel24;
		fillScr = fillScr24;
		drawHLine = drawHLine24;
		drawVLine = drawVLine24;
		setXY = setXYgen;
		setXY2 = setXY2gen;
		draw8C_Pixel = draw8C_Pixel24;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_24;

		LCD_WRITE_COM(0xD0); // power settings
		LCD_WRITE_DATA8(0x07);
		LCD_WRITE_DATA8(0x42);
		LCD_WRITE_DATA8(0x18);

		LCD_WRITE_COM(0xD1); // vcom control
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x07);
		LCD_WRITE_DATA8(0x10);

		LCD_WRITE_COM(0xD2); // power settings for normal mode
		LCD_WRITE_DATA8(0x01);
		LCD_WRITE_DATA8(0x02);

		LCD_WRITE_COM(0xC0); // panel driving setting
		LCD_WRITE_DATA8(0x10);
		LCD_WRITE_DATA8(0x3B);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x02);
		LCD_WRITE_DATA8(0x11);

		LCD_WRITE_COM(0xC5); // frame rate & inversion control
		LCD_WRITE_DATA8(0x03); // 72Hz

		LCD_WRITE_COM(0xC8);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x32);
		LCD_WRITE_DATA8(0x36);
		LCD_WRITE_DATA8(0x45);
		LCD_WRITE_DATA8(0x06);
		LCD_WRITE_DATA8(0x16);
		LCD_WRITE_DATA8(0x37);
		LCD_WRITE_DATA8(0x75);
		LCD_WRITE_DATA8(0x77);
		LCD_WRITE_DATA8(0x54);
		LCD_WRITE_DATA8(0x0C);
		LCD_WRITE_DATA8(0x00);

		LCD_WRITE_COM(0x36); // set address mode
		//LCD_WRITE_DATA8(0x0A); // page-address-order | page/column-selection |  horizontal flip
		LCD_WRITE_DATA8(0b00101011);

		LCD_WRITE_COM(0x3A); // set pixel format
		LCD_WRITE_DATA8(0x66);

		LCD_WRITE_COM(0x21); // INVON

		LCD_WRITE_COM(0x2a);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x01);
		LCD_WRITE_DATA8(0x3F);

		LCD_WRITE_COM(0x2b);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x01);
		LCD_WRITE_DATA8(0xDF);

		LCD_WRITE_COM(0x11);    //Exit Sleep
		delay(120);
		LCD_WRITE_COM(0x29);    //Display on
	}
	if (cntrlType == ili9341_320x240x16)
	{
		maxX = 320;
		maxY = 240;
		drawPixel = drawPixel16;
		fillScr = fillScr16;
		drawHLine = drawHLine16;
		drawVLine = drawVLine16;
		setXY = setXYgen;
		setXY2 = setXY2gen;
		draw8C_Pixel = draw8C_Pixel16;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_16;

		LCD_WRITE_COM(0x11);//sleep out 
		delay(20);
		LCD_WRITE_COM(0x28); //display off
		delay(5);
		LCD_WRITE_COM(0xCF); //power control b
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x83); //83 81 AA
		LCD_WRITE_DATA8(0x30);
		LCD_WRITE_COM(0xED); //power on seq control
		LCD_WRITE_DATA8(0x64); //64 67
		LCD_WRITE_DATA8(0x03);
		LCD_WRITE_DATA8(0x12);
		LCD_WRITE_DATA8(0x81);
		LCD_WRITE_COM(0xE8); //timing control a
		LCD_WRITE_DATA8(0x85);
		LCD_WRITE_DATA8(0x01);
		LCD_WRITE_DATA8(0x79); //79 78
		LCD_WRITE_COM(0xCB); //power control a
		LCD_WRITE_DATA8(0x39);
		LCD_WRITE_DATA8(0X2C);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x34);
		LCD_WRITE_DATA8(0x02);
		LCD_WRITE_COM(0xF7); //pump ratio control
		LCD_WRITE_DATA8(0x20);
		LCD_WRITE_COM(0xEA); //timing control b
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_COM(0xC0); //power control 2
		LCD_WRITE_DATA8(0x26); //26 25
		LCD_WRITE_COM(0xC1); //power control 2
		LCD_WRITE_DATA8(0x11);
		LCD_WRITE_COM(0xC5); //vcom control 1
		LCD_WRITE_DATA8(0x35);
		LCD_WRITE_DATA8(0x3E);
		LCD_WRITE_COM(0xC7); //vcom control 2
		LCD_WRITE_DATA8(0xBE); //BE 94
		LCD_WRITE_COM(0xB1); //frame control
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x1B); //1B 70
		LCD_WRITE_COM(0xB6); //display control
		LCD_WRITE_DATA8(0x0A);
		LCD_WRITE_DATA8(0x82);
		LCD_WRITE_DATA8(0x27);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_COM(0xB7); //emtry mode
		LCD_WRITE_DATA8(0x07);
		LCD_WRITE_COM(0x3A); //pixel format
		LCD_WRITE_DATA8(0x55); //16bit
		LCD_WRITE_COM(0x36); //  5: Memory access ctrl (directions), 1 arg:
		LCD_WRITE_DATA8(0b11111100);// Row addr/col addr, bottom to top refresh
		LCD_WRITE_COM(0x29); //display on
		delay(5);
		LCD_WRITE_COM(0x11);    //Exit Sleep
		delay(120);
		LCD_WRITE_COM(0x29);    //Display on
	}
	if (cntrlType == st7789_135x240x16 || cntrlType == st7789_240x135x16) // ST7789-TTGO
	{
		if (cntrlType == st7789_135x240x16)
		{
			maxX = 135;
			maxY = 240;
			Xoffset = ST7789_X_OFFSET;
			Yoffset = ST7789_Y_OFFSET;
		}
		else
		{
			maxX = 240;
			maxY = 135;
			Xoffset = ST7789_Y_OFFSET;
			Yoffset = ST7789_X_OFFSET;
		}
		drawPixel = drawPixel16;
		fillScr = fillScr16;
		drawHLine = drawHLine16;
		drawVLine = drawVLine16;
		setXY = setXYwOffset;
		setXY2 = setXY2wOffset;
		draw8C_Pixel = draw8C_Pixel16;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_16;

		LCD_WRITE_COM(0xE0);
		LCD_WRITE_DATA8(0x00);

		LCD_WRITE_COM(0x01);	//Power Control 1
		delay(150);

		LCD_WRITE_COM(0x11);	//Power Control 2
		delay(255);

		LCD_WRITE_COM(0x3A);	// VCOM Control 1
		LCD_WRITE_DATA8(0x55);
		delay(10);

		LCD_WRITE_COM(0x36);	// MADCTL : Memory Data Access Control
		if (cntrlType == st7789_135x240x16)
		{
			LCD_WRITE_DATA8(0x00);
		}
		else
		{
			LCD_WRITE_DATA8(0xB0);
		}

		LCD_WRITE_COM(0x2A);	//Memory Access Control
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x2B);	//Pixel Format Set
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x21);	//Display Inversion OFF
		delay(10);

		LCD_WRITE_COM(0x13);	//Frame Rate Control
		delay(10);

		LCD_WRITE_COM(0x29);	//Display ON
		delay(255);
	}
	if (cntrlType == st7796S_480x320x16)
	{
		maxX = 480;
		maxY = 320;
		drawPixel = drawPixel16;
		fillScr = fillScr16;
		drawHLine = drawHLine16;
		drawVLine = drawVLine16;
		setXY = setXYgen;
		setXY2 = setXY2gen;
		draw8C_Pixel = draw8C_Pixel16;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_16;

		LCD_WRITE_COM(0x01); //Software reset
		delay(120);

		LCD_WRITE_COM(0x11); //Sleep exit                                            
		delay(120);

		LCD_WRITE_COM(0xF0); //Command Set control                                 
		LCD_WRITE_DATA8(0xC3);    //Enable extension command 2 partI

		LCD_WRITE_COM(0xF0); //Command Set control                                 
		LCD_WRITE_DATA8(0x96);    //Enable extension command 2 partII

		LCD_WRITE_COM(0x36); //Memory Data Access Control MX, MY, RGB mode                                    
		//LCD_WRITE_DATA8(0x48);    //X-Mirror, Top-Left to right-Buttom, RGB  
		LCD_WRITE_DATA8(0b11101011);

		LCD_WRITE_COM(0x3A);	//Interface Pixel Format                                    
		LCD_WRITE_DATA8(0x55);   //Control interface color format set to 16


		LCD_WRITE_COM(0xB4); //Column inversion 
		LCD_WRITE_DATA8(0x01);    //1-dot inversion

		LCD_WRITE_COM(0xB6); //Display Function Control
		LCD_WRITE_DATA8(0x80);    //Bypass
		LCD_WRITE_DATA8(0x02);    //Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
		LCD_WRITE_DATA8(0x3B);    //LCD Drive Line=8*(59+1)


		LCD_WRITE_COM(0xE8); //Display Output Ctrl Adjust
		LCD_WRITE_DATA8(0x40);
		LCD_WRITE_DATA8(0x8A);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x29);    //Source eqaulizing period time= 22.5 us
		LCD_WRITE_DATA8(0x19);    //Timing for "Gate start"=25 (Tclk)
		LCD_WRITE_DATA8(0xA5);    //Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
		LCD_WRITE_DATA8(0x33);

		LCD_WRITE_COM(0xC1); //Power control2                          
		LCD_WRITE_DATA8(0x06);    //VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset)

		LCD_WRITE_COM(0xC2); //Power control 3                                      
		LCD_WRITE_DATA8(0xA7);    //Source driving current level=low, Gamma driving current level=High

		LCD_WRITE_COM(0xC5); //VCOM Control
		LCD_WRITE_DATA8(0x18);    //VCOM=0.9

		delay(120);

		//ST7796 Gamma Sequence
		LCD_WRITE_COM(0xE0); //Gamma"+"                                             
		LCD_WRITE_DATA8(0xF0);
		LCD_WRITE_DATA8(0x09);
		LCD_WRITE_DATA8(0x0b);
		LCD_WRITE_DATA8(0x06);
		LCD_WRITE_DATA8(0x04);
		LCD_WRITE_DATA8(0x15);
		LCD_WRITE_DATA8(0x2F);
		LCD_WRITE_DATA8(0x54);
		LCD_WRITE_DATA8(0x42);
		LCD_WRITE_DATA8(0x3C);
		LCD_WRITE_DATA8(0x17);
		LCD_WRITE_DATA8(0x14);
		LCD_WRITE_DATA8(0x18);
		LCD_WRITE_DATA8(0x1B);

		LCD_WRITE_COM(0xE1); //Gamma"-"                                             
		LCD_WRITE_DATA8(0xE0);
		LCD_WRITE_DATA8(0x09);
		LCD_WRITE_DATA8(0x0B);
		LCD_WRITE_DATA8(0x06);
		LCD_WRITE_DATA8(0x04);
		LCD_WRITE_DATA8(0x03);
		LCD_WRITE_DATA8(0x2B);
		LCD_WRITE_DATA8(0x43);
		LCD_WRITE_DATA8(0x42);
		LCD_WRITE_DATA8(0x3B);
		LCD_WRITE_DATA8(0x16);
		LCD_WRITE_DATA8(0x14);
		LCD_WRITE_DATA8(0x17);
		LCD_WRITE_DATA8(0x1B);

		delay(120);

		LCD_WRITE_COM(0xF0); //Command Set control                                 
		LCD_WRITE_DATA8(0x3C);    //Disable extension command 2 partI

		LCD_WRITE_COM(0xF0); //Command Set control                                 
		LCD_WRITE_DATA8(0x69);    //Disable extension command 2 partII 	
		delay(120);
		LCD_WRITE_COM(0x29); //Display on  
	}
	if (cntrlType == st7789_240x240x16)
	{
		maxX = 240;
		maxY = 240;
		drawPixel = drawPixel16;
		fillScr = fillScr16;
		drawHLine = drawHLine16;
		drawVLine = drawVLine16;
		setXY = setXYgen;
		setXY2 = setXY2gen;
		draw8C_Pixel = draw8C_Pixel16;
		drawCompressed24bitBitmap = drawCompressed24bitBitmap_16;

		LCD_WRITE_COM(0xE0);
		LCD_WRITE_DATA8(0x00);

		LCD_WRITE_COM(0x01);	//Power Control 1
		delay(150);

		LCD_WRITE_COM(0x11);	//Power Control 2
		delay(255);

		LCD_WRITE_COM(0x3A);	// VCOM Control 1
		LCD_WRITE_DATA8(0x55);
		delay(10);

		LCD_WRITE_COM(0x36);	// MADCTL : Memory Data Access Control
		LCD_WRITE_DATA8(0x00);
		//LCD_WRITE_DATA8(0xB0);

		LCD_WRITE_COM(0x2A);	//Memory Access Control
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x2B);	//Pixel Format Set
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x21);	//Display Inversion OFF
		delay(10);

		LCD_WRITE_COM(0x13);	//Frame Rate Control
		delay(10);

		LCD_WRITE_COM(0x29);	//Display ON
		delay(255);
	}
	return true;
}

/////////////////// Frame Buffer LCD //////////////////////
void * SPI_LCD_FrameBuffer::frameBuffer;

unsigned char SPI_LCD_FrameBuffer::fgColorH, SPI_LCD_FrameBuffer::fgColorL, SPI_LCD_FrameBuffer::bgColorH, SPI_LCD_FrameBuffer::bgColorL, SPI_LCD_FrameBuffer::fgColorHL;
_drawCompressed24bitBitmap SPI_LCD_FrameBuffer::drawCompressed24bitBitmap;
bool SPI_LCD_FrameBuffer::FGbitOn;
void SPI_LCD_FrameBuffer::drawPixel8C(short x, short y)
{
	unsigned int index = (y*maxX + x) >> 1;

	if (FGbitOn)
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x38 | fgColorL | 0x40;
		}
		else
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x07 | fgColorH | 0x80;
		}
	}
	else
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x38 | fgColorL;
		}
		else
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x07 | fgColorH;
		}
	}
}
void SPI_LCD_FrameBuffer::drawPixel16b(short x, short y)
{
	if (x < maxX && y < maxY)
	{
		((unsigned short*)frameBuffer)[y * maxX + x] = fg565;
	}
}
void SPI_LCD_FrameBuffer::fillScr8C(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char tempColorL = ((r != 0) << 2) | ((g != 0) << 1) | (b != 0);
	unsigned char tempColor = (tempColorL << 3) | tempColorL;
	if (FGbitOn)
	{
		tempColorL = tempColorL | 0xc0;
	}
	unsigned int tempC = (tempColor << 24) | (tempColor << 16) | (tempColor << 8 | tempColor);
	unsigned int *tempBuffer = (unsigned int *)frameBuffer;
	for (size_t i = 0; i < (maxX*maxY / 4); i++)
	{
		tempBuffer[i] = tempC;
	}
}
void SPI_LCD_FrameBuffer::fillScr16b(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short color = (((b >> 3) | (g << 5)) << 8) | ((r & 0xF8) | (g >> 5));
	unsigned int tempC = (color << 16) | color;
	unsigned int * tempPointer = (unsigned int *)(frameBuffer);

	for (size_t i = 0; i < maxX * maxY / 2; i++)
	{
		tempPointer[i] = tempC;
	}
}
void SPI_LCD_FrameBuffer::drawHLine8C(short x, short y, int l)
{
	size_t i = 0, lo = l;
	unsigned int index;

	if (x & 0x1 == 1) // x%2 == 1
	{
		drawPixel(x, y);
		i++;
		lo -= 1;
	}
	while (i <= l) // We are alighned to 2
	{
		if (lo >= 2)
		{
			index = (y*maxX + x + i) >> 1;
			if (index >= maxX * maxY / 2)
				return;
			if (FGbitOn)
			{
				((unsigned char*)frameBuffer)[index] = fgColorHL | 0xC0;
			}
			else
			{
				((unsigned char*)frameBuffer)[index] = fgColorHL;
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
void SPI_LCD_FrameBuffer::drawHLine16b(short x, short y, int l)
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
		((unsigned short*)frameBuffer)[maxX * y + i] = fg565;
	}
}
void SPI_LCD_FrameBuffer::drawVLine8C(short x, short y, int l)
{
	// Need to optimize
	if ((y + l) > maxY)
	{
		for (size_t i = y; i < maxY; i++)
		{
			drawPixel(x, i);
		}
	}
	else
	{
		for (size_t i = y; i < (y + l); i++)
		{
			drawPixel(x, i);
		}
	}
}
void SPI_LCD_FrameBuffer::drawVLine16b(short x, short y, int l)
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
		((unsigned short*)frameBuffer)[maxX * i + x] = fg565;
	}
}
void SPI_LCD_FrameBuffer::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	fgColorL = ((r != 0) << 2) | ((g != 0) << 1) | (b != 0);
	fgColorH = fgColorL << 3;
	fgColorHL = fgColorL | fgColorH;
	fg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}
void SPI_LCD_FrameBuffer::setBackColor(unsigned char r, unsigned char g, unsigned char b)
{
	bgColorL = ((r != 0) << 2) | ((g != 0) << 1) | (b != 0);
	bgColorH = bgColorL << 3;
	bg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));
}
void SPI_LCD_FrameBuffer::flushFrameBuffer()
{
	SPI.writeBuffer((unsigned int *)frameBuffer, TotalNumOfBytes);
}
inline void SPI_LCD_FrameBuffer::_drawPixel8C(short x, short y, unsigned char color)
{
	if (x < 0 || x > (maxX-1) || y < 0 || y > (maxY-1))
	{
		return;
	}
	if (isFGbitSet(x, y))
	{
		collision = true;
		return;
	}
	unsigned int index = (y*maxX + x) >> 1;

	if (FGbitOn)
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x38 | color | 0x40;
		}
		else
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x07 | (color << 3) | 0x80;
		}
	}
	else
	{
		if (x & 0x1 == 1) // x%2 == 1
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x38 | color;
		}
		else
		{
			((unsigned char*)frameBuffer)[index] = ((unsigned char*)frameBuffer)[index] & 0x07 | (color << 3);
		}
	}
}
inline void SPI_LCD_FrameBuffer::_drawPixel16b(short x, short y, unsigned char color)
{
	if (x < 0 || x >(maxX - 1) || y < 0 || y >(maxY - 1))
	{
		return;
	}
	((unsigned short*)frameBuffer)[y * maxX + x] = colorTable565[color];
}
inline bool SPI_LCD_FrameBuffer::isFGbitSet(short x, short y)
{
	unsigned int index = (y*maxX + x) >> 1;
	if (x & 0x1 == 1) // x%2 == 1
	{
		return ((((unsigned char*)frameBuffer)[index] & 0x40) == 0x40);
	}
	else
	{
		return ((((unsigned char*)frameBuffer)[index] & 0x80) == 0x80);
	}
}
void SPI_LCD_FrameBuffer::drawCompressed24bitBitmapFB(short x, short y, const unsigned int * dataArray)
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
		r = buffer & 0x000000ff;
		g = (buffer & 0x0000ffff) >> 8;
		b = (buffer & 0x00ffffff) >> 16;
		fgColorL = ((r > 128) << 2) | ((g > 128) << 1) | (b > 128);
		fgColorH = fgColorL << 3;
		fg565 = (((b >> 3) | ((g << 3) & 0xE0)) << 8) | ((r & 0xF8) | (g >> 5));

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

#define DRAW_PIX(X,Y,PIX_DATA)\
		if (X < maxX && Y < maxY) {\
		if (!isFGbitSet(X, Y)) _drawPixel(X, Y, (PIX_DATA) & 0x7); else collision = true;}

bool SPI_LCD_FrameBuffer::init(lcdControllerTypeFB cntrlType, lcdHwAccessor * lcdHwAcc, unsigned char cmdDataPin, unsigned char mosiPin, unsigned char clockPin, unsigned int freq
							   , bool usePSRAM)
{
	_cmdDataPin = cmdDataPin;
	pinMode(cmdDataPin, OUTPUT);

	SPI.begin((char)clockPin, (char)-1, (char)mosiPin, (char)-1);
	SPI.setBitOrder(MSBFIRST);
	SPI.setFrequency(freq);
	SPI.setDataMode(3);
#ifdef ESP32
	ESP32_SPI_DIS_MOSI_MISO_FULL_DUPLEX(ESP32_VSPI);
#endif

	lcdHwAcc->setup();
	SELECT_DATA_COMM(LCD_DATA);
	lcdHwAcc->assertCS();
	lcdHwAcc->reset();
	lcdHwAcc->backLightOn();

	if ((cntrlType == ili9488_480x320x3_FB) || (cntrlType == ili9481_480x320x3_FB))
	{
		FGbitOn = false;
		maxX = 480;
		maxY = 320;
		TotalNumOfBytes = maxX * maxY / 8;
		if (usePSRAM)
		{
			frameBuffer = ps_malloc(maxX*maxY / 2);
			if (!frameBuffer)
			{
				return false;
			}
		}
		else
		{
			frameBuffer = malloc(maxX*maxY / 2);
			if (!frameBuffer)
			{
				return false;
			}
		}
		setXY = setXYgen;
		setXY2 = setXY2gen;
		drawPixel = drawPixel8C;
		fillScr = fillScr8C;
		drawHLine = drawHLine8C;
		drawVLine = drawVLine8C;
		draw8C_Pixel = _drawPixel8C;
		drawCompressed24bitBitmap = drawCompressed24bitBitmapFB;
		if (cntrlType == ili9488_480x320x3_FB)
		{
			LCD_WRITE_COM(0xE0);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x03);
			LCD_WRITE_DATA8(0x09);
			LCD_WRITE_DATA8(0x08);
			LCD_WRITE_DATA8(0x16);
			LCD_WRITE_DATA8(0x0A);
			LCD_WRITE_DATA8(0x3F);
			LCD_WRITE_DATA8(0x78);
			LCD_WRITE_DATA8(0x4C);
			LCD_WRITE_DATA8(0x09);
			LCD_WRITE_DATA8(0x0A);
			LCD_WRITE_DATA8(0x08);
			LCD_WRITE_DATA8(0x16);
			LCD_WRITE_DATA8(0x1A);
			LCD_WRITE_DATA8(0x0F);

			LCD_WRITE_COM(0XE1);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x16);
			LCD_WRITE_DATA8(0x19);
			LCD_WRITE_DATA8(0x03);
			LCD_WRITE_DATA8(0x0F);
			LCD_WRITE_DATA8(0x05);
			LCD_WRITE_DATA8(0x32);
			LCD_WRITE_DATA8(0x45);
			LCD_WRITE_DATA8(0x46);
			LCD_WRITE_DATA8(0x04);
			LCD_WRITE_DATA8(0x0E);
			LCD_WRITE_DATA8(0x0D);
			LCD_WRITE_DATA8(0x35);
			LCD_WRITE_DATA8(0x37);
			LCD_WRITE_DATA8(0x0F);

			LCD_WRITE_COM(0XC0);     //Power Control 1
			LCD_WRITE_DATA8(0x17);    //Vreg1out
			LCD_WRITE_DATA8(0x15);    //Verg2out

			LCD_WRITE_COM(0xC1);     //Power Control 2
			LCD_WRITE_DATA8(0x41);    //VGH,VGL

			LCD_WRITE_COM(0xC5);     //Power Control 3
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x12);    //Vcom
			LCD_WRITE_DATA8(0x80);

			LCD_WRITE_COM(0x36);      //Memory Access
			//LCD_WRITE_DATA8(0x4A);
									 ////76543210
			LCD_WRITE_DATA8(0b11101010);

			LCD_WRITE_COM(0x3A);      // Interface Pixel Format

			LCD_WRITE_DATA8(0x11); 	  // 0x66 = 18 bits (3 bytes per pixel), 0x55 = 16 bits is not supported in SPI mode! 

			LCD_WRITE_COM(0XB0);      // Interface Mode Control
			LCD_WRITE_DATA8(0x80);     //SDO NOT USE

			LCD_WRITE_COM(0xB1);      //Frame rate
			LCD_WRITE_DATA8(0xA0);    //60Hz

			LCD_WRITE_COM(0xB4);      //Display Inversion Control
			LCD_WRITE_DATA8(0x02);    //2-dot

			LCD_WRITE_COM(0XB6);     //Display Function Control  RGB/MCU Interface Control

			LCD_WRITE_DATA8(0x02);    //MCU
			LCD_WRITE_DATA8(0x02);    //Source,Gate scan direction

			LCD_WRITE_COM(0XE9);     // Set Image Function
			LCD_WRITE_DATA8(0x00);    // Disable 24 bit data

			LCD_WRITE_COM(0xF7);      // Adjust Control
			LCD_WRITE_DATA8(0xA9);
			LCD_WRITE_DATA8(0x51);
			LCD_WRITE_DATA8(0x2C);
			LCD_WRITE_DATA8(0x82);    // D7 stream, loose

			LCD_WRITE_COM(0x11);    //Exit Sleep
			delay(120);
			LCD_WRITE_COM(0x29);    //Display on
			delay(10);
			setXY2(0, 0, maxX, maxY);
		}
		else
		{
			LCD_WRITE_COM(0xD0); // power settings
			LCD_WRITE_DATA8(0x07);
			LCD_WRITE_DATA8(0x42);
			LCD_WRITE_DATA8(0x18);

			LCD_WRITE_COM(0xD1); // vcom control
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x07);
			LCD_WRITE_DATA8(0x10);

			LCD_WRITE_COM(0xD2); // power settings for normal mode
			LCD_WRITE_DATA8(0x01);
			LCD_WRITE_DATA8(0x02);

			LCD_WRITE_COM(0xC0); // panel driving setting
			LCD_WRITE_DATA8(0x10);
			LCD_WRITE_DATA8(0x3B);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x02);
			LCD_WRITE_DATA8(0x11);

			LCD_WRITE_COM(0xC5); // frame rate & inversion control
			LCD_WRITE_DATA8(0x03); // 72Hz

			LCD_WRITE_COM(0xC8);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x32);
			LCD_WRITE_DATA8(0x36);
			LCD_WRITE_DATA8(0x45);
			LCD_WRITE_DATA8(0x06);
			LCD_WRITE_DATA8(0x16);
			LCD_WRITE_DATA8(0x37);
			LCD_WRITE_DATA8(0x75);
			LCD_WRITE_DATA8(0x77);
			LCD_WRITE_DATA8(0x54);
			LCD_WRITE_DATA8(0x0C);
			LCD_WRITE_DATA8(0x00);

			LCD_WRITE_COM(0x36); // set address mode
			//LCD_WRITE_DATA8(0x0A); // page-address-order | page/column-selection |  horizontal flip
			LCD_WRITE_DATA8(0b00101011);

			LCD_WRITE_COM(0x3A); // set pixel format
			LCD_WRITE_DATA8(0x11);

			LCD_WRITE_COM(0x21); // INVON

			LCD_WRITE_COM(0x2a);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x01);
			LCD_WRITE_DATA8(0x3F);

			LCD_WRITE_COM(0x2b);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x00);
			LCD_WRITE_DATA8(0x01);
			LCD_WRITE_DATA8(0xDF);

			LCD_WRITE_COM(0x11);    //Exit Sleep
			delay(120);
			LCD_WRITE_COM(0x29);    //Display on
			delay(10);
			setXY2(0, 0, maxX - 1, maxY - 1);

		}
	}
	if (cntrlType == st7789_240x135x16_FB)
	{
		FGbitOn = false;
		maxX = 240;
		maxY = 135;
		TotalNumOfBytes = (maxX) * (maxY) / 2;
		if (usePSRAM)
		{
			frameBuffer = ps_malloc(maxX*maxY*2);
			if (!frameBuffer)
			{
				return false;
			}
		}
		else
		{
			frameBuffer = malloc(maxX*maxY*2);
			if (!frameBuffer)
			{
				return false;
			}
		}
		Xoffset = ST7789_Y_OFFSET;
		Yoffset = ST7789_X_OFFSET;
		setXY = setXYwOffset;
		setXY2 = setXY2wOffset;
		drawPixel = drawPixel16b;
		fillScr = fillScr16b;
		drawHLine = drawHLine16b;
		drawVLine = drawVLine16b;
		draw8C_Pixel = _drawPixel16b;
		drawCompressed24bitBitmap = drawCompressed24bitBitmapFB;

		LCD_WRITE_COM(0xE0);
		LCD_WRITE_DATA8(0x00);

		LCD_WRITE_COM(0x01);	//Power Control 1
		delay(150);

		LCD_WRITE_COM(0x11);	//Power Control 2
		delay(255);

		LCD_WRITE_COM(0x3A);	// VCOM Control 1
		LCD_WRITE_DATA8(0x55);
		delay(10);

		LCD_WRITE_COM(0x36);	// MADCTL : Memory Data Access Control
		LCD_WRITE_DATA8(0xB0);

		LCD_WRITE_COM(0x2A);	//Memory Access Control
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x2B);	//Pixel Format Set
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0x00);
		LCD_WRITE_DATA8(0xF0);

		LCD_WRITE_COM(0x21);	//Display Inversion OFF
		delay(10);

		LCD_WRITE_COM(0x13);	//Frame Rate Control
		delay(10);

		LCD_WRITE_COM(0x29);	//Display ON
		delay(255);
		delay(10);
		setXY2(0, 0, maxX - 1, maxY - 1);
	}
	return true;
}

////// TV OUT //////
#include "videoOut.h"
unsigned char ** TVoutBase::displayFrame; // uint8_t * *
unsigned char TVoutBase::paletteIndex = 0;
unsigned char TVoutBase::colorTableIndex[8];

unsigned char redTable[8] = { 0,4,8,16,32,64,128,255 };
unsigned char greenTable[4] = { 0,64,128,255 };
unsigned char blueTable[8] = { 0,4,8,16,32,64,128,255 };

void TVoutBase::setPaletteIndex(unsigned char index)
{
	paletteIndex = index;
}
void TVoutBase::fifo_reset(i2s_dev_t * dev)
{
	dev->conf.rx_fifo_reset = 1;
	//  while(dev->state.rx_fifo_reset_back);
	dev->conf.rx_fifo_reset = 0;
	dev->conf.tx_fifo_reset = 1;
	// while(dev->state.tx_fifo_reset_back);
	dev->conf.tx_fifo_reset = 0;
}
void TVoutBase::dev_reset(i2s_dev_t * dev)
{
	unsigned int lcConf = *((volatile unsigned char *)(((i2sRegBase + 0x60))));
	unsigned int conf = *((volatile unsigned char *)(((i2sRegBase + 0x8))));
	// do rmw
	(*((volatile unsigned char *)(((i2sRegBase + 0x8)))) = conf | 0x5);
	(*((volatile unsigned char *)(((i2sRegBase + 0x8)))) = conf);
	(*((volatile unsigned char *)(((i2sRegBase + 0x60)))) = lcConf | 0x3);
	(*((volatile unsigned char *)(((i2sRegBase + 0x60)))) = lcConf);
}
unsigned char TVoutBase::getIndex(short x, short y)
{
	return displayFrame[y][x];
}
void TVoutBase::generateRGB323palette()
{
	// 8 bit RGB R3 G2 B3
	unsigned int index = 0;
	for (size_t r = 0; r < 8; r++)
	{
		for (size_t g = 0; g < 4; g++)
		{
			for (size_t b = 0; b < 8; b++)
			{
				//updatePalette((r << 5) | (g << 3) | b, r << 5, g << 6, b << 5);
				updatePalette(computeIndex(redTable[r], greenTable[g], blueTable[b]), redTable[r], greenTable[g], blueTable[b]);
				index++;
			}
		}
	}
	printf("Index = %d\n", index);
}
unsigned char TVoutBase::computeIndex(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char hashRed, hashGreen, hashBlue;
	// 0,4,8,16,32,64,128,255;
	if (r <= 2)
	{
		hashRed = 0;
	}
	else if (r <= 6)
	{
		hashRed = 1;
	}
	else if (r <= 12)
	{
		hashRed = 2;
	}
	else if (r <= 24)
	{
		hashRed = 3;
	}
	else if (r <= 48)
	{
		hashRed = 4;
	}
	else if (r <= 96)
	{
		hashRed = 5;
	}
	else if (r <= 192)
	{
		hashRed = 6;
	}
	else
	{
		hashRed = 7;
	}
	// 0,64,128,255
	if (g <= 32)
	{
		hashGreen = 0;
	}
	else if (g <= 96)
	{
		hashGreen = 1;
	}
	else if (g <= 192)
	{
		hashGreen = 2;
	}
	else
	{
		hashGreen = 3;
	}
	// 0,4,8,16,32,64,128,255;
	if (b <= 2)
	{
		hashBlue = 0;
	}
	else if (b <= 6)
	{
		hashBlue = 1;
	}
	else if (b <= 12)
	{
		hashBlue = 2;
	}
	else if (b <= 24)
	{
		hashBlue = 3;
	}
	else if (b <= 48)
	{
		hashBlue = 4;
	}
	else if (b <= 96)
	{
		hashBlue = 5;
	}
	else if (b <= 192)
	{
		hashBlue = 6;
	}
	else
	{
		hashBlue = 7;
	}
	//printf("Comp index = %d\n", ((hashRed >> 5) << 5) | ((hashGreen >> 6) << 3) | (hashBlue >> 5));

	return (hashRed << 5) | (hashGreen << 3) | (hashBlue);
}
void TVoutBase::setColor(unsigned char r, unsigned char g, unsigned char b)
{
	paletteIndex = computeIndex(r, g, b);
}
void TVoutBase::setBackColor(unsigned char r, unsigned char g, unsigned char b)
{
	bgPaletteIndex = computeIndex(r, g, b);
}
unsigned char TVoutBase::getRGB323paletteIndex(unsigned char r, unsigned char g, unsigned char b)
{
	return computeIndex(r, g, b);
}
unsigned char * TVoutBase::getLineBufferAddress(unsigned short Y)
{
	return displayFrame[Y];
}
void TVoutBase::updatePalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	float chroma_scale = BLANKING_LEVEL / 2 / 256;
	//chroma_scale /= 127;  // looks a little washed out
	chroma_scale /= 80;

	float y = 0.299 * r + 0.587*g + 0.114 * b;
	float u = -0.147407 * r - 0.289391 * g + 0.436798 * b;
	float v = 0.614777 * r - 0.514799 * g - 0.099978 * b;
	y /= 255.0;
	y = (y*(WHITE_LEVEL - BLACK_LEVEL) + BLACK_LEVEL) / 256;

	uint32_t e = 0;
	uint32_t o = 0;
	for (int i = 0; i < 4; i++)
	{
		float p = 2 * M_PI*i / 4 + M_PI;
		float s = sin(p)*chroma_scale;
		float c = cos(p)*chroma_scale;
		uint8_t e0 = round(y + (s*u) + (c*v));
		uint8_t o0 = round(y + (s*u) - (c*v));
		e = (e << 8) | e0;
		o = (o << 8) | o0;
	}
	_palette[index] = e; //even
	_palette[index + 256] = o; //odd
}

void TVoutBase::fillScrTV(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char index = computeIndex(r, g, b); //((r >> 5) << 5) | ((g >> 6) << 3) | (b >> 5);
	unsigned int val = (index << 24) | (index << 16) | (index << 8) | index;
	unsigned int * tempPtr;
	for (size_t y = 0; y < maxY; y++)
	{
		tempPtr = (unsigned int *)displayFrame[y];
		for (size_t x = 0; x < maxX / 4; x++)
		{
			tempPtr[x] = val;
		}
	}
}
void TVoutBase::drawPixelTV(short x, short y)
{
	if (x < maxX && y < maxY)
	{
		displayFrame[y][x] = paletteIndex;
	}
}
void TVoutBase::drawHLineTV(short x, short y, int l)
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
		displayFrame[y][i] = paletteIndex;
	}
}
void TVoutBase::drawVLineTV(short x, short y, int l)
{
	unsigned int fixedL, i;
	if (y + l >= maxY)
	{
		fixedL = maxY;
	}
	else
		fixedL = (y + l);

	for (i = y; i < fixedL; i++)
	{
		displayFrame[i][x] = paletteIndex;
	}
}
void TVoutBase::drawCompressed24bitBitmapTV(short x, short y, const unsigned int * dataArray)
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
		paletteIndex = computeIndex(buffer & 0x000000ff, (buffer & 0x0000ffff) >> 8, (buffer & 0x00ffffff) >> 16);
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

bool TVout::init(TVoutMaxY tvMaxY, bool(*copyLine)(unsigned char *))
{
	unsigned short Yresolution;
	if (tvMaxY == maxY_240)
	{
		Yresolution = 240;
	}
	if (tvMaxY == maxY_269)
	{
		Yresolution = 269;
	}

	if (!video_init(320, Yresolution, false, true))
		return false;
	
	maxX = 320;
	maxY = Yresolution;

	displayFrame = _Frame0lines;
	generateRGB323palette();

	drawPixel = drawPixelTV;
	fillScr = fillScrTV;
	drawHLine = drawHLineTV;
	drawVLine = drawVLineTV;
	drawCompressed24bitBitmap = drawCompressed24bitBitmapTV;
	draw8C_Pixel = draw8CpixelTV;
	
	colorTableIndex[0] = computeIndex(0,0,0);
	colorTableIndex[1] = computeIndex(0, 0, 255);
	colorTableIndex[2] = computeIndex(0, 255, 0);
	colorTableIndex[3] = computeIndex(0, 255, 255);
	colorTableIndex[4] = computeIndex(255, 0, 0);
	colorTableIndex[5] = computeIndex(255, 0, 255);
	colorTableIndex[6] = computeIndex(255, 255, 0);
	colorTableIndex[7] = computeIndex(255, 255, 255);
	
	return true;
}
float TVout::getFPS()
{
	return _FPS;
}
void TVout::draw8CpixelTV(short x, short y, unsigned char pixelData)
{
	if (x < maxX && y < maxY)
	{
		displayFrame[y][x] = colorTableIndex[pixelData];
	}
}

//////// Parallel LCD /////////
void PARALLEL_LCD::generateRGB323palette()
{
	// 8 bit RGB R3 G2 B3
	unsigned int index = 0;
	//_palette16
	unsigned short swap;
	for (size_t r = 0; r < 8; r++)
	{
		for (size_t g = 0; g < 4; g++)
		{
			for (size_t b = 0; b < 8; b++)
			{
				swap = rgbTo565(redTable[r], greenTable[g], blueTable[b]);
				_palette16[index] = (swap << 8) | (swap >> 8);
				index++;
			}
		}
	}
}
void PARALLEL_LCD::updatePalette(unsigned char index, unsigned short val)
{
	_palette16[index] = val;
}
void PARALLEL_LCD::updatePalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short swap;
	swap = rgbTo565(redTable[r], greenTable[g], blueTable[b]);
	_palette16[index] = (swap << 8) | (swap >> 8);
}
void PARALLEL_LCD::draw8CpixelPar(short x, short y, unsigned char pixelData)
{

}
bool PARALLEL_LCD::init(lcdControllerTypePar cntrlType, lcdHwAccessor * lcdHwAcc, I2Ssetup * i2sSetup, unsigned char cmdDataPin, bool usePSRAM)
{
	drawPixel = drawPixelTV;
	fillScr = fillScrTV;
	drawHLine = drawHLineTV;
	drawVLine = drawVLineTV;
	drawCompressed24bitBitmap = drawCompressed24bitBitmapTV;
	draw8C_Pixel = draw8CpixelPar;
	
	if (cntrlType == ili9488_320x240x256C_PAR)
	{
		maxX = _maxX = 320;
		maxY = _maxY = 240; // 318
	}
	else // default
	{
		maxX = _maxX = 320;
		maxY = _maxY = 240; // 318
	}
	
	_line_count = 0;
	_cmdDataPin = cmdDataPin;
	// Setup the pinout
	pinMode(cmdDataPin, OUTPUT); // D/C
	//pinMode(2, OUTPUT); // WR/CLK
	pinMode(i2sSetup->clockPin, OUTPUT);
	for (size_t pin = 0; pin < 8; pin++)
	{
		pinMode(i2sSetup->dataPins[pin], OUTPUT);
	}
	// Data 0-7
	//pinMode(4, OUTPUT); // D0
	//pinMode(5, OUTPUT); // 1
	//pinMode(12, OUTPUT);// 2
	//pinMode(13, OUTPUT);// 3
	//pinMode(14, OUTPUT);// 4
	//pinMode(15, OUTPUT);// 5
	//pinMode(18, OUTPUT);// 6
	//pinMode(19, OUTPUT);// 7

	//ILI9488SPI_PRE_INIT();
	lcdHwAcc->setup();
	//ILI9488SPI_RESET();
	lcdHwAcc->reset();
	//ILI9488SPI_ASSERT_CS();
	lcdHwAcc->assertCS();

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

	ILI9488P_WR_DATA8(0x55); 	  // 0x66 = 18 bits , 0x55 = 16 bits  

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

	// Set the work arae
	unsigned short x1 = 0, y1 = 0, x2 = maxX - 1, y2 = maxY - 1;
	ILI9488P_WR_CMD(0x2a);
	ILI9488P_WR_DATA8(x1 >> 8);
	ILI9488P_WR_DATA8(x1);
	ILI9488P_WR_DATA8(x2 >> 8);
	ILI9488P_WR_DATA8(x2);
	ILI9488P_WR_CMD(0x2b);
	ILI9488P_WR_DATA8(y1 >> 8);
	ILI9488P_WR_DATA8(y1);
	ILI9488P_WR_DATA8(y2 >> 8);
	ILI9488P_WR_DATA8(y2);
	ILI9488P_WR_CMD(0x2b);
	ILI9488P_WR_CMD(0x2c);

	// Init the I2S part
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
	_dev = dev;
	int irq_source;
	// Initialize I2S peripheral
	if (port == I2S_NUM_0)
	{
		periph_module_reset(PERIPH_I2S0_MODULE);
		periph_module_enable(PERIPH_I2S0_MODULE);
		iomux_clock = I2S0O_WS_OUT_IDX;
		irq_source = ETS_I2S0_INTR_SOURCE;
		iomux_signal_base = I2S0O_DATA_OUT0_IDX;
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

	dev->conf.tx_right_first = 0; //????
	dev->conf.rx_right_first = 0;

	dev->lc_conf.out_eof_mode = 1;
	dev->lc_conf.check_owner = 0;
	//dev->lc_conf.out_auto_wrback = 1;

	dev->timing.val = 0;

	I2S1.int_ena.out_eof = 1;

	if (!video_init(maxX, maxY, usePSRAM, false))
		return false;

	displayFrame = _Frame0lines;

	generateRGB323palette();

	return true;
}
bool PARALLEL_LCD::flushFB()
{
	if (_9488_stopIntFlag) // DMA stopped
	{
		_dev->out_link.start = 1;
		_dev->conf.tx_start = 1;
		_9488_stopIntFlag = false;
		return true;
	}
	else
		return false;
}

