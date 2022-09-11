#ifndef ESP_NOW_AUX
#define ESP_NOW_AUX

#include "esp_now.h"
#include <Arduino.h>

#define ESP_NOW_MAX_USER_DATA_SIZE	250
#define ESP_NOW_FIFO_SIZE 4
#define _MAC_LEN 6

struct espNowMessage
{
	char * data;
	int	messageIndex; // 0 - free for driver to store message, > 0 - data available for user, when data taken, user should free the entry by setting back to 0
	unsigned short	dataSize;
	unsigned char	senderMAC[_MAC_LEN];
};

class EspNowAux
{
private:
	bool				onFlag;
	esp_now_peer_info_t peerInfo;
public:
	static unsigned int		dataSent, error, dropCounter;
	static espNowMessage	messageFifo[ESP_NOW_FIFO_SIZE];

	EspNowAux()
	{
		onFlag = false;
		for (size_t i = 0; i < ESP_NOW_FIFO_SIZE; i++)
		{
			messageFifo[i].data = NULL;
			messageFifo[i].dataSize = 0;
		}
		dropCounter = 0;
	}
	~EspNowAux()
	{
		for (size_t i = 0; i < ESP_NOW_FIFO_SIZE; i++)
		{
			if (messageFifo[i].data != NULL)
			{
				free(messageFifo[i].data);
			}
		}
	}
	bool init();
	bool on(bool on, bool encrypt = false); // false to turn off
	bool isOn() { return onFlag; }
	bool sendDataToPeer(uint8_t * macAddress, char * dataToSend, unsigned int retryCounter);
	bool popMessage(espNowMessage * userMessage, uint8_t * macAddress = NULL); // Pop message with this MAC, NULL will just pop the last recived message
	bool extractMAC(String * rawMAC, uint8_t * MACarray); // rawMAC = 24:62:AB:F1:A5:24, MACarray must be of size 6, function checks for validity of the string as well
	unsigned int getNumberOfAvailableMessages(uint8_t * macAddress = NULL);
	unsigned int getNumberOfdroppedMessages() { return dropCounter; };
	static unsigned int getError() { return error; }
	bool waitForDataFromPeer(unsigned int timeOutMiliSec, uint8_t * peerMAC = NULL);

	// declaration of callback functions
	static void OnDataSent(const uint8_t * mac_addr, esp_now_send_status_t status);
	static void OnDataRecv(const uint8_t * mac_addr, const uint8_t * incomingData, int len);
};

extern EspNowAux ESP_NOW_CLASS;

#endif