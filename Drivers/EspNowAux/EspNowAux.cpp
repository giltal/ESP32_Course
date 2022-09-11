#include "EspNowAux.h"
#include <String.h>
#include <WiFi.h>
#include <Arduino.h>

unsigned int	EspNowAux::dataSent = 0;
unsigned int	EspNowAux::dropCounter = 0;
unsigned int	EspNowAux::error = 0;
espNowMessage	EspNowAux::messageFifo[] = { {0,0,0} };

bool EspNowAux::init()
{
	for (size_t i = 0; i < ESP_NOW_FIFO_SIZE; i++)
	{
		messageFifo[i].data = (char *)malloc(ESP_NOW_MAX_USER_DATA_SIZE);
		messageFifo[i].dataSize = 0;
		messageFifo[i].messageIndex = 0;
		if (messageFifo[i].data == NULL )
		{
			for (size_t j = 0; j < i; j++)
			{
				free(messageFifo[j].data);
			}
			return false;
		}
	}
	return true;
}

bool EspNowAux::on(bool on, bool encrypt)
{
	if (on)
	{
		// WiFi Part
		WiFi.mode(WIFI_STA);
		// Init ESP-NOW
		if (esp_now_init() != ESP_OK)
		{
			return false;
		}
		/* Callback Functions */
		// On Tx
		esp_now_register_send_cb(EspNowAux::OnDataSent);
		// On Rx
		esp_now_register_recv_cb(EspNowAux::OnDataRecv);

		/* Register peer */
		memset(&peerInfo, 0, sizeof(peerInfo));
		peerInfo.channel = 0;
		peerInfo.encrypt = encrypt;
		dataSent = 0;
		onFlag = true;
	}
	else
	{
		// Init ESP-NOW
		esp_now_deinit();
		WiFi.mode(WIFI_OFF);
		onFlag = false;
	}
	return true;
}

bool EspNowAux::sendDataToPeer(uint8_t * macAddress, char * dataToSend, unsigned int retryCounter)
{
	unsigned int timeOut = 0;
	error = 0;
	
	if (strlen(dataToSend) >= ESP_NOW_MAX_USER_DATA_SIZE)
	{
		error = 1;
		return false;
	}

	memcpy(peerInfo.peer_addr, macAddress, _MAC_LEN);
	if (esp_now_add_peer(&peerInfo) != ESP_OK)
	{
		error = 2;
		return false;
	}

	for (size_t i = 0; i < retryCounter; i++)
	{
		EspNowAux::dataSent = 0;
		timeOut = 0;
		if ((error = esp_now_send(macAddress, (const unsigned char*)dataToSend, strlen(dataToSend))) != ESP_OK)
		{
			break;
		}
		while ((EspNowAux::dataSent == 0) && (timeOut < 300))
		{
			delay(10);
			timeOut++;
		}
		if (EspNowAux::dataSent == 1)
		{
			esp_now_del_peer(macAddress);
			return true;
		}
		if (timeOut == 300)
		{
			esp_now_del_peer(macAddress);
			error = 3;
			return false;
		}
	}
	esp_now_del_peer(macAddress);

	return false;
}

void EspNowAux::OnDataSent(const uint8_t * mac_addr, esp_now_send_status_t status)
{
	if (status == ESP_NOW_SEND_SUCCESS)
	{
		EspNowAux::dataSent = 1;
	}
	else
	{
		EspNowAux::dataSent = 2;
	}
}

void EspNowAux::OnDataRecv(const uint8_t * mac_addr, const uint8_t * incomingData, int len)
{
	unsigned int i = 0;
	int messageIndex = - 1, freeIndex = -1;
	if (len > ESP_NOW_MAX_USER_DATA_SIZE)
	{
		len = ESP_NOW_MAX_USER_DATA_SIZE - 1;
	}
	// Look for an empty entry
	for (i = 0; i < ESP_NOW_FIFO_SIZE; i++)
	{
		if (messageFifo[i].messageIndex == 0)
		{
			freeIndex = i;
		}
		if (messageFifo[i].messageIndex > messageIndex)
		{
			messageIndex = messageFifo[i].messageIndex;
		}
	}
	if (freeIndex == -1)
	{
		// No empty entry, drop the message
		dropCounter++;
		return;
	}
	messageIndex++;

	for (i = 0; i < len; i++)
	{
		EspNowAux::messageFifo[freeIndex].data[i] = incomingData[i];
	}
	EspNowAux::messageFifo[freeIndex].data[i] = '\0';
	memcpy(EspNowAux::messageFifo[freeIndex].senderMAC, mac_addr, _MAC_LEN);
	EspNowAux::messageFifo[freeIndex].dataSize = len;
	EspNowAux::messageFifo[freeIndex].messageIndex = messageIndex;
}

unsigned int EspNowAux::getNumberOfAvailableMessages(uint8_t * macAddress)
{
	unsigned int count = 0, i;
	for (i = 0; i < ESP_NOW_FIFO_SIZE; i++)
	{
		if (EspNowAux::messageFifo[i].messageIndex > 0)
		{
			if (macAddress == NULL)
			{
				count++;
			}
			else
			{
				if (memcmp((const char *)EspNowAux::messageFifo[i].senderMAC, (const char *)macAddress, _MAC_LEN) == 0)
				{
					count++;
				}
			}
		}
	}
	return count;
}

bool EspNowAux::popMessage(espNowMessage * userMessage, uint8_t * macAddress)
{
	unsigned int i;
	int messageIndex = 0x7fffffff, popIndex = -1;
	if (macAddress == NULL)
	{
		for (i = 0; i < ESP_NOW_FIFO_SIZE; i++)
		{
			if ((EspNowAux::messageFifo[i].messageIndex < messageIndex) && (EspNowAux::messageFifo[i].messageIndex > 0))
			{
				messageIndex = EspNowAux::messageFifo[i].messageIndex;
				popIndex = i;
			}
		}
		if (popIndex == -1)
		{
			// No Messages
			userMessage->data = NULL;
			userMessage->dataSize = 0;
			return false;
		}
		strcpy(userMessage->data, EspNowAux::messageFifo[popIndex].data);
		memcpy(userMessage->senderMAC, EspNowAux::messageFifo[popIndex].senderMAC, _MAC_LEN);
		userMessage->dataSize = EspNowAux::messageFifo[popIndex].dataSize;
		EspNowAux::messageFifo[popIndex].messageIndex = 0; // Free the entry
		return true;
	}
	// Look for message with specific MAC, oldest first
	messageIndex = 0x7fffffff, popIndex = -1;
	for (i = 0; i < ESP_NOW_FIFO_SIZE; i++)
	{
		if ((messageFifo[i].messageIndex < messageIndex) && (messageFifo[i].messageIndex > 0) && (memcmp((const char *)messageFifo[i].senderMAC, (const char *)macAddress, _MAC_LEN) == 0))
		{
			messageIndex = messageFifo[i].messageIndex;
			popIndex = i;
		}
	}
	if (popIndex == -1)
	{
		// No Messages
		userMessage->data = NULL;
		userMessage->dataSize = 0;
		return false;
	}
	strcpy(userMessage->data, messageFifo[popIndex].data);
	memcpy(userMessage->senderMAC, messageFifo[popIndex].senderMAC, _MAC_LEN);
	userMessage->dataSize = messageFifo[popIndex].dataSize;
	messageFifo[popIndex].messageIndex = 0; // Free the entry

	return true;
}

bool EspNowAux::extractMAC(String * rawMAC, uint8_t * MACarray)
{
	String tmpStr;
	unsigned int mac;
	char * ptr;

	// Verfiy rawMAC is OK
	if (rawMAC->length() < 17)
	{
		return false;
	}
	for (size_t i = 0, index = 2; i < 6; i++, index += 3)
	{
		if ((rawMAC->charAt(index) != ':') && i < 5)
		{
			return false;
		}
		tmpStr = rawMAC->substring(index - 2, index);
		MACarray[i] = strtol(tmpStr.c_str(), &ptr, 16); // If there other chars which are not hex number ptr will point to this srting thus its sign will be > 0
		if (strlen(ptr) > 0)
		{
			return false;
		}
	}
	return true;
}

bool EspNowAux::waitForDataFromPeer(unsigned int timeOutMiliSec, uint8_t * peerMAC)
{
	for (size_t i = 0; i < timeOutMiliSec; i++)
	{
		delay(1);
		if (peerMAC == NULL)
		{
			if (ESP_NOW_CLASS.getNumberOfAvailableMessages() > 0)
			{
				return true;
			}
		}
		else
		{
			if (ESP_NOW_CLASS.getNumberOfAvailableMessages(peerMAC) > 0)
			{
				return true;
			}
		}
	}
	return false;
}

EspNowAux ESP_NOW_CLASS;

