#include "soundBlasterESP32.h"
#include "FreeRTOS.h"
#include "driver/i2s.h"
#include "esp32-hal-timer.h"
#include "soc/ledc_struct.h"
#include "FS.h"
#include "SD.h"

#include<stdio.h>
#include<string.h>

#define SB_DEBUG 1

hw_timer_t * timer = NULL;

unsigned char *	_globalMusicData;
dacNumber		_DACpin;
unsigned int	_musicCounter = 0;
unsigned int	_globalNumOfSamples = 0, _globalNumOfBlocks = 0;
bool			_musicIsPlaying = false;

unsigned int	_currentIndexInNotesArray, _numberOfNoteElements;
unsigned char	_IOpin = 5, _PWMchannel, _timerNum = 0;
bool			_repeatFlagArray, _playing, _notesPlayerIsSet = false;
NoteElement	*	_NoteElementsArray;

soundBlaster::soundBlaster(soundPlayMode playMode, dacNumber dacPinNumber, unsigned char i2sPort, unsigned char timerNum, unsigned char ioPin, unsigned char pwmChannel)
{
	if (timerNum < 4)
	{
		this->timerNum = timerNum;
	}
	else
	{
		this->timerNum = 0;
	}
	_timerNum = this->timerNum;

	if (ioPin != 0xff && pwmChannel != 0xff)
	{
		pinMode(ioPin, OUTPUT);
		//ledcSetup(pwmChannel,2000, 16);
		//ledcAttachPin(ioPin, pwmChannel);
		ledcSetup(pwmChannel, 2000000, 7);    // 625000 khz is as fast as we go w 7 bits
		ledcAttachPin(ioPin, pwmChannel);
		ledcWrite(pwmChannel, 0);
		_IOpin = ioPin;
		this->pwmChannel = pwmChannel;
		this->ioPin = ioPin;
		_PWMchannel = pwmChannel;
		timer = timerBegin(timerNum, 80, true);
		_playing = false;
		_notesPlayerIsSet = true;
	}
	
	WAVinfoLoaded = false;
	WAVhandler = new WAVheader();
	_playMode = playMode;
	_DACpin = _dacNumber = dacPinNumber;

	if (i2sPort == 1)
	{
		_I2Sport = 1;
	}
	else
	{
		_I2Sport = 0;
	}
}

soundBlaster::~soundBlaster()
{
	delete WAVhandler;
	if (_notesPlayerIsSet)
	{
		stop();
	}
}

bool soundBlaster::loadWAV(unsigned char * WAVdata)
{
	memcpy((void *)WAVhandler, (const void *)WAVdata, WAV_HEADER_SIZE);
	if (!(WAVhandler->ChunkID[3] == 'F' && WAVhandler->ChunkID[2] == 'F' && WAVhandler->ChunkID[1] == 'I' && WAVhandler->ChunkID[0] == 'R'))
	{
		return false;
	}
	if (!(WAVhandler->AudioFormat == 1)) // PCM
	{
		return false; // Un supported format
	}
#if (SB_DEBUG == 1)
	printf("Number Of Channels: %d\n", WAVhandler->NumChannels);
	printf("Sample rate: %d\n", WAVhandler->SampleRate);
	printf("Bit per sample: %d\n", WAVhandler->BitsPerSample);
	printf("Data Size: %d\n", WAVhandler->Subchunk2Size);
#endif
	WAVhandler->data = (char *)&WAVdata[44];
	WAVinfoLoaded = true;
	return true;
}
#define NOTES_LIMIT	1100
#define DIR_UP		0
#define DIR_DOWN	1
#define DIR_FLAT	0
int freq = 0, numOfNotes = 0, lastVal = 0, directionFlag, lastDirection = DIR_FLAT;
void IRAM_ATTR onTimer()
{
	switch (_DACpin)
	{
	case USE_DAC25:
		dacWrite(25, _globalMusicData[_musicCounter]);
		break;
	case USE_DAC26:
		dacWrite(26, _globalMusicData[_musicCounter]);
		break;
	case USE_BOTH_DACS:
		dacWrite(25, _globalMusicData[_musicCounter]);
		dacWrite(26, _globalMusicData[_musicCounter]);
		break;
	case USE_PWM:
		numOfNotes++;
#if 0
		if (_globalMusicData[_musicCounter] - lastVal > 0)
			directionFlag = DIR_UP;
		if (_globalMusicData[_musicCounter] - lastVal < 0)
			directionFlag = DIR_DOWN;
		if (_globalMusicData[_musicCounter] - lastVal == 0)
			directionFlag = DIR_FLAT;

		if (directionFlag != lastDirection)
		{
			if ((directionFlag == DIR_DOWN && lastDirection == DIR_UP) /*|| (directionFlag == DIR_DOWN && lastDirection == DIR_FLAT)*/)
			{
				freq++;
			}
		}
		
		lastDirection = directionFlag;
		if (numOfNotes == NOTES_LIMIT)
		{
			numOfNotes = 0;
			ledcWriteTone(_PWMchannel, freq*10);
				freq = 0;
		}
		lastVal = _globalMusicData[_musicCounter];
#endif
		//auto& reg = LEDC.channel_group[0].channel[0];
		LEDC.channel_group[0].channel[_PWMchannel].duty.duty = _globalMusicData[_musicCounter] << 4; // 25 bit (21.4)
		LEDC.channel_group[0].channel[_PWMchannel].conf0.sig_out_en = 1; // This is the output enable control bit for channel
		LEDC.channel_group[0].channel[_PWMchannel].conf1.duty_start = 1; // When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware
		LEDC.channel_group[0].channel[_PWMchannel].conf0.clk_en = 1;

		dacWrite(25, _globalMusicData[_musicCounter]);
		break;
	default:
		break;
	}
		
	_musicCounter++;
	if (_musicCounter == _globalNumOfSamples)
	{
		timerAlarmDisable(timer);
		timerDetachInterrupt(timer);
		timerEnd(timer);
		_musicIsPlaying = false;
		switch (_DACpin)
		{
		case USE_DAC25:
			dacWrite(25, 0);
			break;
		case USE_DAC26:
			dacWrite(26, 0);
			break;
		case USE_BOTH_DACS:
			dacWrite(25, 0);
			dacWrite(26, 0);
			break;
		case USE_PWM:
			ledcWrite(_PWMchannel, 0);
			break;
		default:
			break;
		}

	}
}

bool soundBlaster::playWAV()
{
	if (!WAVinfoLoaded)
	{
		return false;
	}
	if (_playMode == CPU_INT_MODE)
	{
		if (_musicIsPlaying)
		{
			timerAlarmDisable(timer);
			timerDetachInterrupt(timer);
			timerEnd(timer);
			switch (_DACpin)
			{
			case USE_DAC25:
				dacWrite(25, 0);
				break;
			case USE_DAC26:
				dacWrite(26, 0);
				break;
			case USE_BOTH_DACS:
				dacWrite(25, 0);
				dacWrite(26, 0);
				break;
			case USE_PWM:
				ledcWrite(_PWMchannel, 0);
				break;
			default:
				break;
			}			
		}
		_musicIsPlaying = true;

		_globalNumOfSamples = WAVhandler->Subchunk2Size;
		_globalMusicData = (unsigned char*)WAVhandler->data;
		timer = timerBegin(timerNum, 8, true); // Set to run @ 10MHz
		timerAttachInterrupt(timer, &onTimer, true);
		timerAlarmWrite(timer, 10000000 / WAVhandler->SampleRate, true);
		timerAlarmEnable(timer);
		return true;
	}
	else
	{
		unsigned int currentChunck, dataWritten;
		unsigned int leftData = WAVhandler->Subchunk2Size;
		char * tempBuf;
		unsigned long cycles;
		i2s_config_t i2s_config;
		i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN); // Only TX
		i2s_config.sample_rate = WAVhandler->SampleRate;
		i2s_config.bits_per_sample = (i2s_bits_per_sample_t)16;
		i2s_config.channel_format = (i2s_channel_fmt_t)I2S_CHANNEL_FMT_ONLY_RIGHT;
		i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S_MSB);
		i2s_config.dma_buf_count = 2;
		i2s_config.dma_buf_len = 256;
		i2s_config.use_apll = false;
		i2s_config.intr_alloc_flags = ESP_INTR_FLAG_INTRDISABLED;

		i2s_driver_install((i2s_port_t)_I2Sport, &i2s_config, 0, NULL);

		switch (_DACpin)
		{
		case USE_DAC25:
			i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
			break;
		case USE_DAC26:
			i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
			break;
		case USE_BOTH_DACS:
			i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
			break;
		default:
			break;
		}

		tempBuf = WAVhandler->data;

		while (leftData)
		{
			if (leftData > WAV_PLAYER_BUF_SIZE)
			{
				currentChunck = WAV_PLAYER_BUF_SIZE;
				leftData -= WAV_PLAYER_BUF_SIZE;
			}
			else
			{
				currentChunck = leftData;
				leftData = 0;
			}

			for (size_t i = 0; i < currentChunck; i++)
			{
				if (WAVhandler->BitsPerSample == 8)
				{
					wavTempBuf[i] = (*tempBuf) << 8;
					tempBuf++;
				}
			}
			i2s_write((i2s_port_t)_I2Sport, (const char *)wavTempBuf, currentChunck * 2, &dataWritten, portMAX_DELAY);
		}
	}
}

bool soundBlaster::playWAV(char * file)
{
	File fileHandler;
	fileHandler = SD.open(file);
	if (!fileHandler)
	{
		return false;
	}
	unsigned char wavHeader[WAV_HEADER_SIZE];
	if (fileHandler.read(wavHeader, WAV_HEADER_SIZE) != WAV_HEADER_SIZE)
	{
		fileHandler.close();
		return false;
	}
	memcpy((void *)WAVhandler, (const void *)wavHeader, WAV_HEADER_SIZE);
	if (!(WAVhandler->ChunkID[3] == 'F' && WAVhandler->ChunkID[2] == 'F' && WAVhandler->ChunkID[1] == 'I' && WAVhandler->ChunkID[0] == 'R'))
	{
		return false;
	}
	if (!(WAVhandler->AudioFormat == 1)) // PCM
	{
		return false; // Un supported format
	}
#if (SB_DEBUG == 1)
	printf("Number Of Channels: %d\n", WAVhandler->NumChannels);
	printf("Sample rate: %d\n", WAVhandler->SampleRate);
	printf("Bit per sample: %d\n", WAVhandler->BitsPerSample);
	printf("Data Size: %d\n", WAVhandler->Subchunk2Size);
#endif
	//WAVhandler->data = (char *)&WAVdata[44];
	_globalNumOfBlocks = WAVhandler->Subchunk2Size / WAS_SUB_BLOCK_SIZE;
	WAVinfoLoaded = true;
}
/////// Notes Player Section ///////

void _restartNotesInt(unsigned short durationInMiliSec);

void IRAM_ATTR onTimerNotesPlayer()
{
	if (_currentIndexInNotesArray == _numberOfNoteElements) // End of array
	{
		if (_repeatFlagArray)
		{
			_currentIndexInNotesArray = 0;
			_restartNotesInt(_NoteElementsArray[0].duration);
			ledcWriteTone(_PWMchannel, _NoteElementsArray[0].note);
		}
		else
		{
			ledcWriteTone(_PWMchannel, 0);
			timerAlarmDisable(timer);
			timerDetachInterrupt(timer);
			timerEnd(timer);
			_playing = false;
		}
	}
	else
	{
		_currentIndexInNotesArray++;
		_restartNotesInt(_NoteElementsArray[_currentIndexInNotesArray].duration);
		ledcWriteTone(_PWMchannel, _NoteElementsArray[_currentIndexInNotesArray].note);
	}
}

void _restartNotesInt(unsigned short durationInMiliSec)
{
	if (_playing == false) // First time
	{
		timerAlarmDisable(timer);
		timerDetachInterrupt(timer);
		timerEnd(timer);
		timer = timerBegin(_timerNum, 80, true);
		timerAttachInterrupt(timer, &onTimerNotesPlayer, true);
		timerAlarmWrite(timer, durationInMiliSec * 1000, false);
		timerAlarmEnable(timer);
	}
	else
	{
		_playing = true;
		timerAlarmWrite(timer, durationInMiliSec * 1000, false);
		timerRestart(timer);
	}
}

void soundBlaster::playNotesArray(NoteElement * notesArray, unsigned int numberOfNotes, bool repeat)
{
	_NoteElementsArray = notesArray;
	_numberOfNoteElements = numberOfNotes;
	_currentIndexInNotesArray = 0;
	_repeatFlagArray = repeat;
	_playing = false;
	_restartNotesInt(_NoteElementsArray[_currentIndexInNotesArray].duration);
	ledcWriteTone(_PWMchannel, _NoteElementsArray[_currentIndexInNotesArray].note);
}

void soundBlaster::stop()
{
	if (_notesPlayerIsSet)
	{
		ledcWriteTone(_PWMchannel, 0);
		timerAlarmDisable(timer);
		timerDetachInterrupt(timer);
		timerEnd(timer);
	}
}


