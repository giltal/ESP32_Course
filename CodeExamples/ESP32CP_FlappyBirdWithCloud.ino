/*
 Name:		ESP32CP_FlappyBird.ino
 Created:	28-Jun-21 7:57:34 AM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"

PCF8574 pcf8574(0x21);
ST77XX_FB lcd;

#define SPEAKER_PIN 27

// (we can change the size of the game easily that way)
#define TFTW				240     // screen width
#define TFTH				135     // screen height
#define TFTW2				120     // half screen width
#define TFTH2				67     // half screen height
// game constant
#define SPEED             2

//#define GRAVITY         9.8
//#define JUMP_FORCE		2.15
#define GRAVITY         6.0
#define JUMP_FORCE		1.0

#define SKIP_TICKS		22.0     // 1000 / 50fps
#define MAX_FRAMESKIP    22
// bird size
#define BIRDW             20     // bird width
#define BIRDH             20     // bird height
#define BIRDW2            10     // half width
#define BIRDH2            10     // half height
// pipe size
#define PIPEW            18     // pipe width
#define GAPHEIGHT        48     // pipe gap height
// floor size
#define FLOORH           10     // floor height (from bottom of the screen)
// grass size
#define GRASSH            4     // grass height (inside floor, starts at floor y)

// background
#define BCKGRDCOL	50,50,0xff
// pipe
#define PIPECOL		0,0xff,0
// pipe highlight
#define PIPEHIGHCOL 0xff,0xff,0
// pipe seam
#define PIPESEAMCOL 0,0xff,0xff
// floor
#define FLOORCOL    0xff,0xff,0xff
// grass (col2 is the stripe color)
#define GRASSCOL    0xff,0,0xff
#define GRASSCOL2   0,0xff,0xff

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
const unsigned char angryBird[] = {
0x00,0x15,0x00,0x14,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x99,0x04,0x49,0x99,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x90,0x04,0x44,0x99,0x99,0x99,0x99,
0x90,0x99,0x99,0x99,0x44,0x44,0x44,0x49,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x90,0x44,0x44,0x40,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x44,0x44,0x44,0x44,0x49,0x99,
0x99,0x90,0x99,0x99,0x94,0x44,0x44,0x44,0x44,0x44,0x99,0x99,0x90,0x99,0x99,0x04,0x44,0x44,0x44,0x44,0x44,0x49,0x99,0x90,0x99,0x09,0x44,0x44,0x44,0x04,0x44,0x44,
0x49,0x99,0x90,0x90,0x00,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x99,0x90,0x99,0x00,0x44,0x44,0x44,0x77,0x07,0x07,0x44,0x99,0x90,0x99,0x90,0x44,0x44,0x44,0x07,0x76,
0x67,0x04,0x99,0x90,0x99,0x90,0x44,0x44,0x44,0x40,0x66,0x60,0x40,0x99,0x90,0x99,0x90,0x44,0x44,0x47,0x70,0x66,0x66,0x40,0x99,0x90,0x99,0x99,0x04,0x47,0x77,0x77,
0x46,0x64,0x09,0x99,0x90,0x99,0x99,0x90,0x77,0x77,0x77,0x74,0x74,0x09,0x99,0x90,0x99,0x99,0x99,0x00,0x67,0x77,0x76,0x09,0x99,0x99,0x90,0x99,0x99,0x99,0x99,0x90,
0x00,0x09,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90 };

const unsigned char FlappyBird[] =
{
0x00,0x28,0x00,0x28,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x00,
0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x44,0x40,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x44,0x44,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x44,0x44,0x40,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x00,0x00,0x00,0x04,0x44,0x44,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x04,
0x44,0x44,0x44,0x00,0x44,0x44,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x04,0x44,0x44,0x44,0x40,0x04,0x44,0x40,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x04,0x44,0x44,0x44,0x00,0x44,0x40,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x04,0x44,0x44,
0x44,0x44,0x44,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x00,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x90,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
0x44,0x09,0x99,0x99,0x99,0x99,0x90,0x00,0x09,0x99,0x04,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x99,0x99,0x99,0x99,0x00,0x00,0x09,0x90,0x44,0x44,
0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x99,0x99,0x99,0x99,0x90,0x00,0x99,0x04,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x00,0x99,
0x99,0x00,0x90,0x00,0x90,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x99,0x90,0x00,0x09,0x00,0x00,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x99,0x90,0x00,0x09,0x00,0x04,0x44,0x44,0x44,0x44,0x44,0x40,0x00,0x44,0x44,0x44,0x44,0x44,0x00,0x44,0x09,0x99,0x00,0x00,0x00,
0x04,0x44,0x44,0x44,0x44,0x44,0x00,0x00,0x04,0x44,0x44,0x44,0x00,0x00,0x04,0x09,0x99,0x99,0x00,0x00,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x00,0x00,0x04,0x44,0x40,
0x00,0x00,0x04,0x09,0x99,0x99,0x00,0x00,0x44,0x44,0x44,0x44,0x40,0x44,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x44,0x09,0x99,0x00,0x00,0x04,0x44,0x44,0x44,0x44,
0x44,0x04,0x44,0x07,0x70,0x00,0x00,0x00,0x07,0x74,0x44,0x09,0x99,0x00,0x09,0x04,0x44,0x44,0x40,0x44,0x44,0x04,0x40,0x77,0x77,0x77,0x00,0x07,0x77,0x70,0x44,0x09,
0x99,0x09,0x99,0x04,0x44,0x44,0x44,0x44,0x40,0x44,0x40,0x77,0x70,0x07,0x70,0x70,0x77,0x70,0x44,0x09,0x99,0x99,0x99,0x04,0x44,0x44,0x44,0x04,0x44,0x44,0x40,0x77,
0x77,0x77,0x70,0x00,0x00,0x00,0x44,0x09,0x99,0x99,0x99,0x94,0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x00,0x00,0x00,0x46,0x40,0x04,0x44,0x44,0x09,0x99,0x99,0x99,0x90,
0x44,0x44,0x44,0x44,0x44,0x44,0x40,0x44,0x44,0x06,0x66,0x66,0x00,0x40,0x44,0x09,0x99,0x99,0x99,0x90,0x44,0x44,0x44,0x77,0x77,0x77,0x77,0x74,0x40,0x06,0x66,0x66,
0x66,0x04,0x44,0x09,0x99,0x99,0x99,0x90,0x44,0x47,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x70,0x66,0x66,0x66,0x60,0x44,0x09,0x99,0x99,0x99,0x99,0x07,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x70,0x70,0x00,0x06,0x66,0x64,0x40,0x99,0x99,0x99,0x99,0x99,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x06,0x66,0x40,0x66,0x66,0x40,0x99,
0x99,0x99,0x99,0x99,0x90,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x06,0x66,0x60,0x04,0x66,0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x07,0x77,0x77,0x77,0x77,0x77,0x77,
0x77,0x70,0x66,0x07,0x77,0x44,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x77,0x77,0x00,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x07,0x77,0x77,0x77,0x77,0x77,0x74,0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x90,0x00,0x04,0x77,0x74,0x00,0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x00,0x00,0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x99
};

const unsigned char cloud[] = {
0x00,0x32,0x00,0x25,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x77,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x00,0x00,0x07,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x00,0x00,0x00,0x00,0x07,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x70,0x00,0x00,0x77,0x77,0x77,0x00,0x00,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x97,0x00,0x07,0x77,0x77,0x77,0x77,0x77,
0x00,0x00,0x79,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x09,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x00,0x00,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x00,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,
0x00,0x00,0x00,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x79,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x00,0x00,0x77,0x77,0x07,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x09,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x90,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,
0x07,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x07,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x70,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x00,0x07,0x99,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x07,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x00,0x09,0x99,0x99,0x99,0x99,0x90,0x00,0x00,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x00,0x00,0x99,0x99,0x99,0x99,0x00,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,
0x09,0x99,0x99,0x90,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x07,0x99,0x99,0x70,0x07,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x00,0x99,0x99,0x70,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x99,0x99,0x00,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x00,0x99,0x99,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x99,0x99,
0x00,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x99,0x99,0x70,0x07,0x77,0x77,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x07,0x99,0x99,0x70,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x09,0x99,0x99,0x90,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
0x00,0x79,0x99,0x99,0x99,0x00,0x00,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x00,0x00,0x99,0x99,0x99,0x99,0x90,0x00,
0x00,0x00,0x07,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x70,0x00,0x00,0x00,0x79,0x99,0x99,0x99,0x99,0x99,0x70,0x00,0x00,0x00,0x07,0x77,0x77,0x77,
0x77,0x77,0x77,0x70,0x77,0x77,0x77,0x00,0x00,0x00,0x77,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x00,0x00,0x07,0x77,0x77,0x77,0x77,0x70,0x00,0x00,0x00,0x00,
0x00,0x79,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x97,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x79,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x79,0x77,0x77,0x79,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x97,0x70,0x00,0x00,0x77,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99 };
// bird structure
typedef struct _bird {
	int x, y, old_y;
	int col;
	float vel_y;
} BIRD;

// pipe structure
typedef struct _pipeF {
	int x, gap_y;
	int col;
} PIPEF;

// score
static short score;
// temporary x and y var
static short tmpx, tmpy;

BIRD bird;
PIPEF pipef;

void setup()
{
	Serial.begin(115200);
	pcf8574.begin();
	// Setup the speaker
	pinMode(SPEAKER_PIN, OUTPUT);
	pcf8574.pinMode(3, INPUT);
	pcf8574.pinMode(2, INPUT);
	pcf8574.pinMode(1, INPUT);
	ledcSetup(0, 2000, 8);
	ledcAttachPin(SPEAKER_PIN, 0);

	Serial.begin(115200);
	// Setup the LCD screen
	if (!lcd.init(40000000L))
	{
		printf("Cannot initialize LCD!");
		while (1);
	}
	// Setup the touch pannel
}

// ---------------
// main loop
// ---------------
void loop()
{
	game_start();
	game_loop();
	game_over();
}

// ---------------
// game loop
// ---------------
void game_loop()
{
	char tempStr[10];
	float speed = 2.0;
	// ===============
	// prepare game variables
	// ===============
	// instead of calculating the distance of the floor from the screen height each time store it in a variable
	unsigned short GAMEH = TFTH - FLOORH;
	// game loop time variables
	float delta, old_time, current_time;
	current_time = millis();

	// passed pipef flag to count score
	bool passed_pipe = false;
	pipef.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
	while (1)
	{
		lcd.fillScr(BCKGRDCOL);
		lcd.draw8bBitMap(100, 30, cloud, true);

		lcd.setColor(0, 0, 0);
		lcd.drawHLine(0, GAMEH, TFTW);
		lcd.setColor(0, 0, 0);
		lcd.drawHLine(0, GAMEH + GRASSH, TFTW);

		// ===============
		// input
		// ===============
		if (pcf8574.digitalRead(3) == HIGH)
		{
			// if the bird is not too close to the top of the screen apply jump force
			if (bird.y > BIRDH2*0.5) bird.vel_y = -JUMP_FORCE;
			// else zero velocity
			else bird.vel_y = 0;
		}

		// ===============
		// update
		// ===============
		// calculate delta time
		// ---------------
		old_time = current_time;
		current_time = millis();
		delta = (current_time - old_time) / 1000.0;

		// bird
		// ---------------
		bird.vel_y += GRAVITY * delta;
		bird.y += bird.vel_y;

		// pipef
		// ---------------
		pipef.x -= speed;
		// if pipef reached edge of the screen reset its position and gap
		if (pipef.x < 0)
		{
			pipef.x = TFTW - PIPEW;
			pipef.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
		}

		// ===============
		// draw
		// ===============
		// pipef
		// ---------------
		// we save cycles if we avoid drawing the pipef when outside the screen
		if (pipef.x >= 0 && pipef.x < TFTW)
		{
			// pipef color
			lcd.setColor(PIPECOL);
			lcd.drawRect(pipef.x, 0, pipef.x + PIPEW, pipef.gap_y, true);
			lcd.drawRect(pipef.x, pipef.gap_y + GAPHEIGHT + 1, pipef.x + PIPEW, GAMEH, true);
		}

		// bird
		lcd.draw8bBitMap(bird.x, bird.y, angryBird, true);
		// save position to erase bird on next draw
		//bird.old_y = bird.y;


		// ===============
		// collision
		// ===============
		// if the bird hit the ground game over
		if (bird.y > (GAMEH - BIRDH))
			break;
		// checking for bird collision with pipef
		//if ((bird.x + BIRDW) >= (pipef.x - BIRDW2) && bird.x <= (pipef.x + PIPEW - BIRDW)) 
		if ((bird.x + BIRDW) >= (pipef.x) && bird.x <= (pipef.x + PIPEW))
		{
			// bird entered a pipef, check for collision
			if (bird.y < pipef.gap_y || (bird.y + BIRDH) >(pipef.gap_y + GAPHEIGHT))
				break;
			else
				passed_pipe = true;
		}
		// if bird has passed the pipef increase score
		else if (bird.x > pipef.x + PIPEW - BIRDW && passed_pipe)
		{
			passed_pipe = false;
			score++;
		}

		// update score
		lcd.setColor(0xff, 0xff, 0xff);
		sprintf(tempStr, "%d", score);
		//lcd.drawString(tempStr, TFTW2, 4, 20);
		lcd.loadFonts(OBLIQUE18);
		lcd.print(tempStr, TFTW2, 4);
		lcd.flushFB();
	}

	// add a small delay to show how the player lost
	delay(1000);
}

// ---------------
// game start
// ---------------
void game_start()
{
	lcd.fillScr(0, 0, 0);
	lcd.setColor(0xff, 0xff, 0xff);
	lcd.loadFonts(ORBITRON_LIGHT32);
	lcd.print("Flappy Bird", 0, 10, true);
	lcd.print("Press B2", 0, 50, true);
	lcd.draw8bBitMap(90, 100, FlappyBird, true);

	lcd.flushFB();

	//sound(523, 500);
	//sound(587, 500);
	//sound(659, 500);

	while (pcf8574.digitalRead(2) == LOW);
	// init game settings
	game_init();
}

// ---------------
// game init
// ---------------
void game_init()
{
	// clear screen
	lcd.fillScr(BCKGRDCOL);
	// reset score
	score = 0;
	// init bird
	bird.x = 60;
	bird.y = bird.old_y = TFTH2 - BIRDH;
	bird.vel_y = -JUMP_FORCE;
	tmpx = tmpy = 0;
	// generate new random seed for the pipef gape
	randomSeed(ESP.getCycleCount());
	// init pipef
	pipef.x = TFTW;
}

// ---------------
// game over
// ---------------
void game_over()
{
	char tempStr[50];
	lcd.fillScr(0, 0, 0);
	lcd.setColor(0xff, 0xff, 0xff);
	lcd.loadFonts(ORBITRON_LIGHT32);
	lcd.print("Game Over", 0, 10, true);
	sprintf(tempStr, "Score %d", score);
	lcd.setColor(0, 0xff, 0);
	lcd.print(tempStr, 0, 50, true);
	lcd.setColor(0, 0, 0xff);
	lcd.print("Press B2", 0, 100, true);
	lcd.flushFB();
	while (pcf8574.digitalRead(2) == LOW);
	while (pcf8574.digitalRead(2) == HIGH);
}
