#ifndef FONTS_H
#define FONTS_H

#include <pgmspace.h>

typedef struct
{ // Data stored PER font
	unsigned short	dataOffset;     // Pointer into data
	unsigned short	width;    // Bitmap dimensions in pixels
	unsigned short	height;
	unsigned short	xAdvance;         // Distance to advance cursor (x axis)
	short			xOffset;
	short			yOffset;
} fontInfo;

typedef struct 
{ // Data stored for FONT AS A WHOLE:
	const unsigned char *fontsData;  // Fonts raw data
	const fontInfo *	fontsInfoArray; // Fonts  array
	unsigned char		first, last; 
	unsigned char		fontHight;   
} customFont;

enum fontType
{
	ORBITRON_LIGHT24,
	ORBITRON_LIGHT32,
	MONO_BOLD18,
	OBLIQUE18,
	SANS_OBLIQUE56
};

extern customFont orbitronLight32Font;
extern customFont orbitronLight24Font;
extern customFont monoBold18Font;
extern customFont oblique18Font;
extern customFont sansOblique56Font;

#endif // FONTS_H
