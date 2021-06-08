#ifndef _WIIMOTE_H_
#define _WIIMOTE_H_

#include <cstdint>

typedef void (* wiimote_callback_t)(uint8_t number, uint8_t *, size_t);

typedef struct
{
	bool b1, b2, bPlus, bMinus, bHome, bA, bB, nC, nZ;
	bool aUp, aDown, aLeft, aRight;
	bool nunDataAvailable;
	unsigned char nX, nY;
} WII_KEYS;

class Wiimote 
{
public:
	static void init(bool usePSRAM = false);
    static void handle();
	static bool readKeys(WII_KEYS * wiiKeysP);
	static bool isConnected();
};

#endif
