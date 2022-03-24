/*
 General graphics lib to be inherited
*/

#ifndef GRAPHICS_h
#define GRAPHICS_h

#include "Fonts/fonts.h"
#include <sys/types.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <rom/gpio.h>
#include <soc/gpio_sig_map.h>
#include <driver/i2s.h>
#include <rom/lldesc.h>
#include <cstring>

enum flipOption { noflip = 0, flipX = 1, flipY = 2, flipXY = 3 };

typedef void(*_drawPixel)(short x, short y);
typedef void(*_fillScr)(unsigned char r, unsigned char g, unsigned char b);
typedef void(*_drawHLine)(short x, short y, int l);
typedef void(*_drawVLine)(short x, short y, int l);
typedef void(*_drawCompressed24bitBitmap)(short x, short y, const unsigned int * dataArray);
typedef void(*_draw8C_Pixel)(short x, short y, unsigned char pixelData);

class graphics
{
	protected:
		static bool collision;
		static unsigned char	fgColor[3], bgColor[3];
		static int				_fgColor, _bgColor;
		static unsigned short	fg565, bg565;
		static short			maxX, maxY;
		static customFont *	currentFonts;
		static fontType		fontTypeLoaded;
		static _draw8C_Pixel draw8C_Pixel;

		void	swap(short * a, short * b);
		static unsigned short colorTable565[8];
		static unsigned int colorTableRGB[8];

	private:
		static void Dummy_drawPixel(short x, short y) {};
		static void Dummy_fillScr(unsigned char r, unsigned char g, unsigned char b) {};
		static void Dummy_drawHLine(short x, short y, int l) {};
		static void Dummy_drawVLine(short x, short y, int l) {};
		static void Dummy_setColor(unsigned char r, unsigned char g, unsigned char b) {};
		static void Dummy_setBackColor(unsigned char r, unsigned char g, unsigned char b) {};

	public:
		graphics(short maxX, short maxY);
		~graphics() {};
		
		static _drawPixel drawPixel;
		static _fillScr fillScr;
		static _drawHLine drawHLine;
		static _drawVLine drawVLine;

		virtual void setColor(unsigned char r, unsigned char g, unsigned char b);
		virtual void setBackColor(unsigned char r, unsigned char g, unsigned char b);

		virtual void drawLine(short x1, short y1, short x2, short y2);

		static unsigned short rgbTo565(unsigned char r, unsigned char g, unsigned char b);
		unsigned int rgbToInt(unsigned char r, unsigned char g, unsigned char b);
		
		virtual int	getXSize() { return maxX; }
		virtual int	getYSize() { return maxY; }

		virtual void drawRect(short x1, short y1, short x2, short y2,bool fill = false);
		void	drawRoundRect(short x1, short y1, short x2, short y2,bool fill = false);
		void	drawCircle(short x, short y, int radius, bool fill = false);
		void	drawTriangle(short x0, short y0, short x1, short y1, short x2, short y2, bool fill = false);
		void	loadFonts(fontType fontsToLoad);
		short	getFontHieght();
		short	getPrintWidth(char * string);
		void	print(char * string, short x, short y,bool center = false);
		bool	draw8bBitMap(short x, short y, const unsigned char * dataArray, bool useSkipBit, flipOption flipOpt = noflip);
		static _drawCompressed24bitBitmap drawCompressed24bitBitmap;

};

#define LCD_COMMAND	0
#define LCD_DATA	1

#define SELECT_DATA_COMM(X)		digitalWrite(_cmdDataPin, X);
#define LCD_WRITE_DATA8(VAL)	SPI.writeBYTE((char)VAL);
#define LCD_WRITE_DATA16(X)		SPI.writeShort(X);
#define LCD_WRITE_DATA32(X)		SPI.writeRGB(X);

#define LCD_WRITE_COM(VAL)\
			SELECT_DATA_COMM(LCD_COMMAND);\
			LCD_WRITE_DATA8((char)VAL);\
			SELECT_DATA_COMM(LCD_DATA);

#define ST7789_X_OFFSET				52
#define ST7789_Y_OFFSET				40

class lcdHwAccessor
{
public:
	lcdHwAccessor() {};
	~lcdHwAccessor() {};
	virtual void setup() = 0;
	virtual void reset() = 0;
	virtual void assertCS() = 0;
	virtual void deAssertCS() = 0;
	virtual void backLightOn() = 0;
	virtual void backLightOff() = 0;
};
// Implementation example:
/*
class lcdAccessor : public lcdHwAccessor
{
public:
	lcdAccessor() {};
	~lcdAccessor() {};
	void setup()
	{
		pcf8574.pinMode(P0, OUTPUT); // Chip Select
		pcf8574.pinMode(P1, OUTPUT); // reset
	}
	void reset()
	{
		pcf8574.digitalWrite(P1, LOW);
		delay(250);
		pcf8574.digitalWrite(P1, HIGH);
		delay(250);
	};
	void assertCS()
	{
		pcf8574.digitalWrite(P0, LOW);
	}
	void deAssertCS()
	{
		pcf8574.digitalWrite(P0, HIGH);
	}
	void backLightOn()
	{

	}
	void backLightOff()
	{

	}
} myLCD_Accessor;*/

enum lcdControllerType { ili9488_480x320x24 = 0, ili9481_480x320x24 = 1, ili9341_320x240x16 = 2 ,st7789_135x240x16 = 3, st7789_240x135x16 = 4, st7796S_480x320x16 = 5,
						 st7789_240x240x16 = 6};

typedef void(*_setXY2)(short x1, short y1, short x2, short y2);
typedef void(*_setXY)(short x, short y);

class BASE_SPI_LCD : public graphics
{
protected:
	static unsigned short	Xoffset, Yoffset;
	static void setXY2gen(short x1, short y1, short x2, short y2);
	static void setXY2wOffset(short x1, short y1, short x2, short y2);
	static void setXYgen(short x, short y);
	static void setXYwOffset(short x, short y);
	static void drawPixel24(short x, short y);
	static void drawPixel16(short x, short y);
	static void fillScr16(unsigned char r, unsigned char g, unsigned char b);
	static void fillScr24(unsigned char r, unsigned char g, unsigned char b);
	static void drawHLine16(short x, short y, int l);
	static void drawHLine24(short x, short y, int l);
	static void drawVLine16(short x, short y, int l);
	static void drawVLine24(short x, short y, int l);
	static void draw8C_Pixel16(short x, short y, unsigned char pixelData);
	static void draw8C_Pixel24(short x, short y, unsigned char pixelData);
public:
	BASE_SPI_LCD() : graphics(0,0) {};
	~BASE_SPI_LCD() {};
	static _setXY2 setXY2;
	static _setXY setXY;
};

class SPI_LCD : public BASE_SPI_LCD
{
protected:
	static void drawCompressed24bitBitmap_16(short x, short y, const unsigned int * dataArray);
	static void drawCompressed24bitBitmap_24(short x, short y, const unsigned int * dataArray);
public:
	bool init(lcdControllerType cntrlType, lcdHwAccessor * lcdHwAcc, unsigned char cmdDataPin, unsigned char mosiPin, unsigned char clockPin, unsigned int freq = 40000000L);
};

enum lcdControllerTypeFB { st7789_240x135x16_FB = 0, ili9488_480x320x3_FB = 1, ili9481_480x320x3_FB = 2};

class SPI_LCD_FrameBuffer : public BASE_SPI_LCD
{
public:
	SPI_LCD_FrameBuffer() {};
	~SPI_LCD_FrameBuffer() {};
	bool init(lcdControllerTypeFB cntrlType, lcdHwAccessor * lcdHwAcc, unsigned char cmdDataPin, unsigned char mosiPin, unsigned char clockPin, unsigned int freq = 40000000L,
		      bool usePSRAM = false);
	void flushFrameBuffer();
	void setColor(unsigned char r, unsigned char g, unsigned char b);
	void setBackColor(unsigned char r, unsigned char g, unsigned char b);
	void setFGbitOn() { FGbitOn = true; }; // All Pixels being drawn will be marked as foreground 
	void setFGbitOff() { FGbitOn = false; };
	static inline bool isFGbitSet(short x, short y);
	static _drawCompressed24bitBitmap drawCompressed24bitBitmap;
private:
	unsigned int TotalNumOfBytes = 0;
	static inline void _drawPixel8C(short x, short y, unsigned char color);
	static inline void _drawPixel16b(short x, short y, unsigned char color);
	static void * frameBuffer;
	static unsigned char fgColorH, fgColorL, bgColorH, bgColorL, fgColorHL;
	static bool FGbitOn;
	static void drawPixel8C(short x, short y);
	static void drawPixel16b(short x, short y);	
	static void fillScr8C(unsigned char r, unsigned char g, unsigned char b);
	static void fillScr16b(unsigned char r, unsigned char g, unsigned char b);
	static void drawHLine8C(short x, short y, int l);
	static void drawHLine16b(short x, short y, int l);
	static void drawVLine8C(short x, short y, int l);
	static void drawVLine16b(short x, short y, int l);
	static void drawCompressed24bitBitmapFB(short x, short y, const unsigned int * dataArray);
};

#define I2S0_REG_BASE	0x3FF4F000
#define I2S1_REG_BASE	0x3FF6D000
class TVoutBase : public graphics
{
protected:
	// I2S related
	void fifo_reset(i2s_dev_t* dev);
	void dev_reset(i2s_dev_t* dev);
	i2s_port_t port;
	i2s_dev_t* I2S[I2S_NUM_MAX] = { &I2S0, &I2S1 };
	unsigned int i2sRegBase = I2S0_REG_BASE;
	i2s_dev_t* dev;
	static unsigned char colorTableIndex[8];
	static unsigned char **displayFrame; // uint8_t * *
	static unsigned char paletteIndex;
	unsigned char bgPaletteIndex = 0;
	static unsigned char computeIndex(unsigned char r, unsigned char g, unsigned char b);
	static void drawPixelTV(short x, short y);
	static void drawHLineTV(short x, short y, int l);
	static void drawVLineTV(short x, short y, int l);
	static void fillScrTV(unsigned char r, unsigned char g, unsigned char b);
	static void drawCompressed24bitBitmapTV(short x, short y, const unsigned int * dataArray);
public:
	TVoutBase() : graphics(320, 240) {};
	~TVoutBase() {};
	void setColor(unsigned char r, unsigned char g, unsigned char b);
	void setPaletteIndex(unsigned char index);
	void setBackColor(unsigned char r, unsigned char g, unsigned char b);
	virtual void updatePalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
	unsigned char getRGB323paletteIndex(unsigned char r, unsigned char g, unsigned char b);
	virtual void generateRGB323palette();
	unsigned char getIndex(short x, short y);
	unsigned char * getLineBufferAddress(unsigned short Y);
};

enum TVoutMaxY {maxY_240 = 0, maxY_269 = 1};

class TVout : public TVoutBase
{
private:
	static void draw8CpixelTV(short x, short y, unsigned char pixelData);
public:
	TVout(){};
	~TVout() {};
	bool init(TVoutMaxY tvMaxY = maxY_240, bool(*copyLine)(unsigned char *) = NULL); // True =  TV Out
	float getFPS();
};

////////// Parallel LCD ///////////
#define ESP_WRITE_REG(REG,DATA) (*((volatile unsigned int *)(((REG)))) = DATA)
#define ESP_READ_REG(REG) (*((volatile unsigned int *)(((REG)))))

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

enum lcdControllerTypePar { ili9488_480x320x256C_PAR = 0, ili9488_320x240x256C_PAR = 1};

enum ParaBusFreq { _8MHz = 10, _10MHz = 8, _12_5MHz = 6, _16MHz = 5, _20MHz = 4, _26MHz = 3 };

typedef struct _I2Ssetup
{
	unsigned int	port;
	unsigned char	dataPins[8]; // For now use only: D0-7 = 4,5,12,13,14,15,18,19
	unsigned char	clockPin; // 2, D\C = 0
	ParaBusFreq		freq;
} I2Ssetup;

class PARALLEL_LCD : public TVoutBase
{
protected:
	static void draw8CpixelPar(short x, short y, unsigned char pixelData);
	unsigned char pinList[9];
	volatile int iomux_signal_base;
	volatile int iomux_clock;
public:
	bool init(lcdControllerTypePar cntrlType, lcdHwAccessor * lcdHwAcc, I2Ssetup * i2sSetup, unsigned char cmdDataPin, bool usePSRAM = false);
	void updatePalette(unsigned char index, unsigned short val);
	void updatePalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
	void generateRGB323palette(); // 256 colors
	bool flushFB();
};

#endif