
/* Copyright (c) 2020, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/
#ifndef VIDEO_OUT_h
#define VIDEO_OUT_h

#define Screen_WIDTH	384
#define Screen_HEIGHT	240

int _pal_ = 1;
bool _par9488 = false;
int _9488_lineIndex;

unsigned short _maxX = 320, _maxY = 240; // Only for 9488 parallel mode

#include "esp_types.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "esp_err.h"
#include "soc/gpio_reg.h"
#include "soc/rtc.h"
#include "soc/soc.h"
#include "soc/i2s_struct.h"
#include "soc/i2s_reg.h"
#include "soc/ledc_struct.h"
#include "soc/rtc_io_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "rom/lldesc.h"
#include "driver/periph_ctrl.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "driver/i2s.h"

//====================================================================================================
//====================================================================================================
//
// low level HW setup of DAC/DMA/APLL/PWM
//

lldesc_t _dma_desc[4] = {0};
unsigned short * dmaBuffer9488;
intr_handle_t _isr_handle;
i2s_dev_t* _dev;

uint8_t** _lines; // filled in by emulator
uint8_t** _Frame0lines;
uint8_t** _Frame1lines;
uint16_t* _palette16;

extern "C"
void IRAM_ATTR video_isr(volatile void* buf);

// simple isr
void IRAM_ATTR i2s_intr_handler_video(void *arg)
{
    if (I2S0.int_st.out_eof)
        video_isr(((lldesc_t*)I2S0.out_eof_des_addr)->buf); // get the next line of video
    I2S0.int_clr.val = I2S0.int_st.val;                     // reset the interrupt
}

#define _ESP_READ_REG(REG) (*((volatile unsigned char *)(((REG)))))
#define _I2S0_REG_BASE	0x3FF6D000 /* 0x3FF4F000 - I2S 0*/

void IRAM_ATTR i2s_intr_handler_9488(void *arg)
{
	if (I2S1.int_st.out_eof) // get the next line of video
	{
		//_9488_lineIndex - handle int to short order in memory
		//while ((!((_ESP_READ_REG(_I2S0_REG_BASE + 0xBC) & 0x1) == 0x1)));

		for (size_t i = 0; i < _maxX; i+=2)
		{
			dmaBuffer9488[i]   = _palette16[_lines[_9488_lineIndex][i+1]];
			dmaBuffer9488[i+1] = _palette16[_lines[_9488_lineIndex][i]];

			dmaBuffer9488[_maxX * 1 + i]   = _palette16[_lines[_9488_lineIndex + 1][i+1]];
			dmaBuffer9488[_maxX * 1 + i+1] = _palette16[_lines[_9488_lineIndex + 1][i]];

			dmaBuffer9488[_maxX * 2 + i]   = _palette16[_lines[_9488_lineIndex + 2][i+1]];
			dmaBuffer9488[_maxX * 2 + i+1] = _palette16[_lines[_9488_lineIndex + 2][i]];

			dmaBuffer9488[_maxX * 3 + i]   = _palette16[_lines[_9488_lineIndex + 3][i+1]];
			dmaBuffer9488[_maxX * 3 + i+1] = _palette16[_lines[_9488_lineIndex + 3][i]];

			dmaBuffer9488[_maxX * 4 + i] = _palette16[_lines[_9488_lineIndex + 4][i + 1]];
			dmaBuffer9488[_maxX * 4 + i + 1] = _palette16[_lines[_9488_lineIndex + 4][i]];

			dmaBuffer9488[_maxX * 5 + i] = _palette16[_lines[_9488_lineIndex + 5][i + 1]];
			dmaBuffer9488[_maxX * 5 + i + 1] = _palette16[_lines[_9488_lineIndex + 5][i]];
		}
		_9488_lineIndex += 6;
		if (_9488_lineIndex >= _maxY)
		{
			_9488_lineIndex = 0;
		}

		//while ((!((_ESP_READ_REG(_I2S0_REG_BASE + 0xBC) & 0x1) == 0x1)));

		_dev->out_link.stop = 1;
		_dev->out_link.start = 0;
		_dev->conf.tx_start = 0;
		//dev_reset(dev);
		unsigned int lcConf = *((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60))));
		unsigned int conf = *((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8))));
		// do rmw
		(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8)))) = conf | 0x5);
		(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8)))) = conf);
		(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60)))) = lcConf | 0x3);
		(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60)))) = lcConf);
		// Until here

		// Configure DMA burst mode
		_dev->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
		// Set address of DMA descriptor
		_dev->out_link.addr = (uint32_t)_dma_desc;
		// Start DMA operation
		_dev->out_link.start = 1;
		_dev->conf.tx_start = 1;
	}
	I2S1.int_clr.val = I2S1.int_st.val;                     // reset the interrupt
}


static esp_err_t start_dma(int line_width,int samples_per_cc, int ch = 1)
{
    periph_module_enable(PERIPH_I2S0_MODULE);

    // setup interrupt
    if (esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
        i2s_intr_handler_video, 0, &_isr_handle) != ESP_OK)
        return -1;

    // reset conf
    I2S0.conf.val = 1;
    I2S0.conf.val = 0;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_mono = (ch == 2 ? 0 : 1);

    I2S0.conf2.lcd_en = 1;
    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.sample_rate_conf.tx_bits_mod = 16;
    I2S0.conf_chan.tx_chan_mod = (ch == 2) ? 0 : 1;

    // Create TX DMA buffers
    for (int i = 0; i < 2; i++) {
        int n = line_width*2*ch;
        if (n >= 4092) {
            printf("DMA chunk too big:%s\n",n);
            return -1;
        }
        _dma_desc[i].buf = (uint8_t*)heap_caps_calloc(1, n, MALLOC_CAP_DMA);
        if (!_dma_desc[i].buf)
            return -1;
        
        _dma_desc[i].owner = 1;
        _dma_desc[i].eof = 1;
        _dma_desc[i].length = n;
        _dma_desc[i].size = n;
        _dma_desc[i].empty = (uint32_t)(i == 1 ? _dma_desc : _dma_desc+1);
    }
    I2S0.out_link.addr = (uint32_t)_dma_desc;

    //  Setup up the apll: See ref 3.2.7 Audio PLL
    //  f_xtal = (int)rtc_clk_xtal_freq_get() * 1000000;
    //  f_out = xtal_freq * (4 + sdm2 + sdm1/256 + sdm0/65536); // 250 < f_out < 500
    //  apll_freq = f_out/((o_div + 2) * 2)
    //  operating range of the f_out is 250 MHz ~ 500 MHz
    //  operating range of the apll_freq is 16 ~ 128 MHz.
    //  select sdm0,sdm1,sdm2 to produce nice multiples of colorburst frequencies

    //  see calc_freq() for math: (4+a)*10/((2 + b)*2) mhz
    //  up to 20mhz seems to work ok:
    //  rtc_clk_apll_enable(1,0x00,0x00,0x4,0);   // 20mhz for fancy DDS

    if (!_pal_) {
        switch (samples_per_cc) {
            case 3: rtc_clk_apll_enable(1,0x46,0x97,0x4,2);   break;    // 10.7386363636 3x NTSC (10.7386398315mhz)
            case 4: rtc_clk_apll_enable(1,0x46,0x97,0x4,1);   break;    // 14.3181818182 4x NTSC (14.3181864421mhz)
        }
    } else {
        rtc_clk_apll_enable(1,0x04,0xA4,0x6,1);     // 17.734476mhz ~4x PAL
    }

    I2S0.clkm_conf.clkm_div_num = 1;            // I2S clock divider’s integral value.
    I2S0.clkm_conf.clkm_div_b = 0;              // Fractional clock divider’s numerator value.
    I2S0.clkm_conf.clkm_div_a = 1;              // Fractional clock divider’s denominator value
    I2S0.sample_rate_conf.tx_bck_div_num = 1;
    I2S0.clkm_conf.clka_en = 1;                 // Set this bit to enable clk_apll.
    I2S0.fifo_conf.tx_fifo_mod = (ch == 2) ? 0 : 1; // 32-bit dual or 16-bit single channel data

    dac_output_enable(DAC_CHANNEL_1);           // DAC, video on GPIO25
    dac_i2s_enable();                           // start DAC!

    I2S0.conf.tx_start = 1;                     // start DMA!
    I2S0.int_clr.val = 0xFFFFFFFF;
    I2S0.int_ena.out_eof = 1;
    I2S0.out_link.start = 1;
    return esp_intr_enable(_isr_handle);        // start interruprs!
}

static esp_err_t start_dma_9488()
{
	_9488_lineIndex = 6;
	// setup interrupt
	if (esp_intr_alloc(ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
		i2s_intr_handler_9488, 0, &_isr_handle) != ESP_OK)
		return -1;
	// Stop all ongoing DMA operations
	_dev->out_link.stop = 1;
	_dev->out_link.start = 0;
	_dev->conf.tx_start = 0;

	unsigned int lcConf = *((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60))));
	unsigned int conf = *((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8))));
	// do rmw
	(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8)))) = conf | 0x5);
	(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x8)))) = conf);
	(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60)))) = lcConf | 0x3);
	(*((volatile unsigned char *)(((_I2S0_REG_BASE + 0x60)))) = lcConf);
	// Until here

	// Configure DMA burst mode
	_dev->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
	// Set address of DMA descriptor
	_dev->out_link.addr = (uint32_t)_dma_desc;
	// Start DMA operation
	_dev->out_link.start = 1;
	_dev->conf.tx_start = 1;

	return esp_intr_enable(_isr_handle);        // start interruprs!
}

uint32_t cpu_ticks()
{
  return xthal_get_ccount();
}

uint32_t us() 
{
    return cpu_ticks()/240;
}

// Color clock frequency is 315/88 (3.57954545455)
// DAC_MHZ is 315/11 or 8x color clock
// 455/2 color clocks per line, round up to maintain phase
// HSYNCH period is 44/315*455 or 63.55555..us
// Field period is 262*44/315*455 or 16651.5555us

#define IRE(_x)          ((uint32_t)(((_x)+40)*255/3.3/147.5) << 8)   // 3.3V DAC
#define SYNC_LEVEL       IRE(-40)
#define BLANKING_LEVEL   IRE(0)
#define BLACK_LEVEL      IRE(7.5)
#define GRAY_LEVEL       IRE(50)
#define WHITE_LEVEL      IRE(100)


#define _P0 (color >> 16)
#define _P1 (color >> 8)
#define _P2 (color)
#define _P3 (color << 8)

volatile int _line_counter = 0;
volatile int _frame_counter = 0;

int _active_lines;
int _line_count;

int _line_width;
int _samples_per_cc;
uint32_t* _palette;

unsigned char * screen = 0;

float _sample_rate;

int _hsync;
int _hsync_long;
int _hsync_short;
int _burst_start;
int _burst_width;
int _active_start;

int16_t* _burst0 = 0; // pal bursts
int16_t* _burst1 = 0;

static int usec(float us)
{
    uint32_t r = (uint32_t)(us*_sample_rate);
    return ((r + _samples_per_cc)/(_samples_per_cc << 1))*(_samples_per_cc << 1);  // multiple of color clock, word align
}

#define NTSC_COLOR_CLOCKS_PER_SCANLINE 228       // really 227.5 for NTSC but want to avoid half phase fiddling for now
#define NTSC_FREQUENCY (315000000.0/88)
#define NTSC_LINES 262

#define PAL_COLOR_CLOCKS_PER_SCANLINE 284        // really 283.75 ?
#define PAL_FREQUENCY 4433618.75
#define PAL_LINES 312

void pal_init();
//===================================================================================================
//===================================================================================================
// PAL
const uint32_t atari_4_phase_pal[] =
{
	0x18181818,0x1B1B1B1B,0x1E1E1E1E,0x21212121,0x25252525,0x28282828,0x2B2B2B2B,0x2E2E2E2E,
	0x32323232,0x35353535,0x38383838,0x3B3B3B3B,0x3F3F3F3F,0x42424242,0x45454545,0x49494949,
	0x16271A09,0x192A1D0C,0x1C2D200F,0x1F302312,0x23342716,0x26372A19,0x293A2D1C,0x2C3D301F,
	0x30413423,0x33443726,0x36473A29,0x394A3D2C,0x3D4E4130,0x40514433,0x43544736,0x47584B3A,
	0x0F24210C,0x1227240F,0x152A2712,0x182D2A15,0x1C312E19,0x1F34311C,0x2237341F,0x253A3722,
	0x293E3B26,0x2C413E29,0x2F44412C,0x3247442F,0x364B4833,0x394E4B36,0x3C514E39,0x4055523D,
	0x0B1F2511,0x0E222814,0x11252B17,0x14282E1A,0x182C321E,0x1B2F3521,0x1E323824,0x21353B27,
	0x25393F2B,0x283C422E,0x2B3F4531,0x2E424834,0x32464C38,0x35494F3B,0x384C523E,0x3C505642,
	0x09182718,0x0C1B2A1B,0x0F1E2D1E,0x12213021,0x16253425,0x19283728,0x1C2B3A2B,0x1F2E3D2E,
	0x23324132,0x26354435,0x29384738,0x2C3B4A3B,0x303F4E3F,0x33425142,0x36455445,0x3A495849,
	0x0B11251F,0x0E142822,0x11172B25,0x141A2E28,0x181E322C,0x1B21352F,0x1E243832,0x21273B35,
	0x252B3F39,0x282E423C,0x2B31453F,0x2E344842,0x32384C46,0x353B4F49,0x383E524C,0x3C425650,
	0x0F0C2124,0x120F2427,0x1512272A,0x18152A2D,0x1C192E31,0x1F1C3134,0x221F3437,0x2522373A,
	0x29263B3E,0x2C293E41,0x2F2C4144,0x322F4447,0x3633484B,0x39364B4E,0x3C394E51,0x403D5255,
	0x15091B27,0x180C1E2A,0x1B0F212D,0x1E122430,0x22162834,0x25192B37,0x281C2E3A,0x2B1F313D,
	0x2F233541,0x32263844,0x35293B47,0x382C3E4A,0x3C30424E,0x3F334551,0x42364854,0x463A4C58,
	0x1C0A1426,0x1F0D1729,0x22101A2C,0x25131D2F,0x29172133,0x2C1A2436,0x2F1D2739,0x32202A3C,
	0x36242E40,0x39273143,0x3C2A3446,0x3F2D3749,0x43313B4D,0x46343E50,0x49374153,0x4D3B4557,
	0x220D0E23,0x25101126,0x28131429,0x2B16172C,0x2F1A1B30,0x321D1E33,0x35202136,0x38232439,
	0x3C27283D,0x3F2A2B40,0x422D2E43,0x45303146,0x4934354A,0x4C37384D,0x4F3A3B50,0x533E3F54,
	0x26130A1D,0x29160D20,0x2C191023,0x2F1C1326,0x3320172A,0x36231A2D,0x39261D30,0x3C292033,
	0x402D2437,0x4330273A,0x46332A3D,0x49362D40,0x4D3A3144,0x503D3447,0x5340374A,0x57443B4E,
	0x271A0916,0x2A1D0C19,0x2D200F1C,0x3023121F,0x34271623,0x372A1926,0x3A2D1C29,0x3D301F2C,
	0x41342330,0x44372633,0x473A2936,0x4A3D2C39,0x4E41303D,0x51443340,0x54473643,0x584B3A47,
	0x24200C10,0x27230F13,0x2A261216,0x2D291519,0x312D191D,0x34301C20,0x37331F23,0x3A362226,
	0x3E3A262A,0x413D292D,0x44402C30,0x47432F33,0x4B473337,0x4E4A363A,0x514D393D,0x55513D41,
	0x1F25110B,0x2228140E,0x252B1711,0x282E1A14,0x2C321E18,0x2F35211B,0x3238241E,0x353B2721,
	0x393F2B25,0x3C422E28,0x3F45312B,0x4248342E,0x464C3832,0x494F3B35,0x4C523E38,0x5056423C,
	0x19271709,0x1C2A1A0C,0x1F2D1D0F,0x22302012,0x26342416,0x29372719,0x2C3A2A1C,0x2F3D2D1F,
	0x33413123,0x36443426,0x39473729,0x3C4A3A2C,0x404E3E30,0x43514133,0x46544436,0x4A58483A,
	0x12261E0A,0x1529210D,0x182C2410,0x1B2F2713,0x1F332B17,0x22362E1A,0x2539311D,0x283C3420,
	0x2C403824,0x2F433B27,0x32463E2A,0x3549412D,0x394D4531,0x3C504834,0x3F534B37,0x43574F3B,
	//odd
	0x18181818,0x1B1B1B1B,0x1E1E1E1E,0x21212121,0x25252525,0x28282828,0x2B2B2B2B,0x2E2E2E2E,
	0x32323232,0x35353535,0x38383838,0x3B3B3B3B,0x3F3F3F3F,0x42424242,0x45454545,0x49494949,
	0x1A271609,0x1D2A190C,0x202D1C0F,0x23301F12,0x27342316,0x2A372619,0x2D3A291C,0x303D2C1F,
	0x34413023,0x37443326,0x3A473629,0x3D4A392C,0x414E3D30,0x44514033,0x47544336,0x4B58473A,
	0x21240F0C,0x2427120F,0x272A1512,0x2A2D1815,0x2E311C19,0x31341F1C,0x3437221F,0x373A2522,
	0x3B3E2926,0x3E412C29,0x41442F2C,0x4447322F,0x484B3633,0x4B4E3936,0x4E513C39,0x5255403D,
	0x251F0B11,0x28220E14,0x2B251117,0x2E28141A,0x322C181E,0x352F1B21,0x38321E24,0x3B352127,
	0x3F39252B,0x423C282E,0x453F2B31,0x48422E34,0x4C463238,0x4F49353B,0x524C383E,0x56503C42,
	0x27180918,0x2A1B0C1B,0x2D1E0F1E,0x30211221,0x34251625,0x37281928,0x3A2B1C2B,0x3D2E1F2E,
	0x41322332,0x44352635,0x47382938,0x4A3B2C3B,0x4E3F303F,0x51423342,0x54453645,0x58493A49,
	0x25110B1F,0x28140E22,0x2B171125,0x2E1A1428,0x321E182C,0x35211B2F,0x38241E32,0x3B272135,
	0x3F2B2539,0x422E283C,0x45312B3F,0x48342E42,0x4C383246,0x4F3B3549,0x523E384C,0x56423C50,
	0x210C0F24,0x240F1227,0x2712152A,0x2A15182D,0x2E191C31,0x311C1F34,0x341F2237,0x3722253A,
	0x3B26293E,0x3E292C41,0x412C2F44,0x442F3247,0x4833364B,0x4B36394E,0x4E393C51,0x523D4055,
	0x1B091527,0x1E0C182A,0x210F1B2D,0x24121E30,0x28162234,0x2B192537,0x2E1C283A,0x311F2B3D,
	0x35232F41,0x38263244,0x3B293547,0x3E2C384A,0x42303C4E,0x45333F51,0x48364254,0x4C3A4658,
	0x140A1C26,0x170D1F29,0x1A10222C,0x1D13252F,0x21172933,0x241A2C36,0x271D2F39,0x2A20323C,
	0x2E243640,0x31273943,0x342A3C46,0x372D3F49,0x3B31434D,0x3E344650,0x41374953,0x453B4D57,
	0x0E0D2223,0x11102526,0x14132829,0x17162B2C,0x1B1A2F30,0x1E1D3233,0x21203536,0x24233839,
	0x28273C3D,0x2B2A3F40,0x2E2D4243,0x31304546,0x3534494A,0x38374C4D,0x3B3A4F50,0x3F3E5354,
	0x0A13261D,0x0D162920,0x10192C23,0x131C2F26,0x1720332A,0x1A23362D,0x1D263930,0x20293C33,
	0x242D4037,0x2730433A,0x2A33463D,0x2D364940,0x313A4D44,0x343D5047,0x3740534A,0x3B44574E,
	0x091A2716,0x0C1D2A19,0x0F202D1C,0x1223301F,0x16273423,0x192A3726,0x1C2D3A29,0x1F303D2C,
	0x23344130,0x26374433,0x293A4736,0x2C3D4A39,0x30414E3D,0x33445140,0x36475443,0x3A4B5847,
	0x0C202410,0x0F232713,0x12262A16,0x15292D19,0x192D311D,0x1C303420,0x1F333723,0x22363A26,
	0x263A3E2A,0x293D412D,0x2C404430,0x2F434733,0x33474B37,0x364A4E3A,0x394D513D,0x3D515541,
	0x11251F0B,0x1428220E,0x172B2511,0x1A2E2814,0x1E322C18,0x21352F1B,0x2438321E,0x273B3521,
	0x2B3F3925,0x2E423C28,0x31453F2B,0x3448422E,0x384C4632,0x3B4F4935,0x3E524C38,0x4256503C,
	0x17271909,0x1A2A1C0C,0x1D2D1F0F,0x20302212,0x24342616,0x27372919,0x2A3A2C1C,0x2D3D2F1F,
	0x31413323,0x34443626,0x37473929,0x3A4A3C2C,0x3E4E4030,0x41514333,0x44544636,0x48584A3A,
	0x1E26120A,0x2129150D,0x242C1810,0x272F1B13,0x2B331F17,0x2E36221A,0x3139251D,0x343C2820,
	0x38402C24,0x3B432F27,0x3E46322A,0x4149352D,0x454D3931,0x48503C34,0x4B533F37,0x4F57433B,
};
#if 0
const uint32_t atari_palette_rgb[256] =
{
	0x00000000,0x000F0F0F,0x001B1B1B,0x00272727,0x00333333,0x00414141,0x004F4F4F,0x005E5E5E,
	0x00686868,0x00787878,0x00898989,0x009A9A9A,0x00ABABAB,0x00BFBFBF,0x00D3D3D3,0x00EAEAEA,
	0x00001600,0x000F2100,0x001A2D00,0x00273900,0x00334500,0x00405300,0x004F6100,0x005D7000,
	0x00687A00,0x00778A17,0x00899B29,0x009AAC3B,0x00ABBD4C,0x00BED160,0x00D2E574,0x00E9FC8B,
	0x001C0000,0x00271300,0x00331F00,0x003F2B00,0x004B3700,0x00594500,0x00675300,0x00756100,
	0x00806C12,0x008F7C22,0x00A18D34,0x00B29E45,0x00C3AF56,0x00D6C36A,0x00EAD77E,0x00FFEE96,
	0x002F0000,0x003A0000,0x00460F00,0x00521C00,0x005E2800,0x006C3600,0x007A4416,0x00885224,
	0x00925D2F,0x00A26D3F,0x00B37E50,0x00C48F62,0x00D6A073,0x00E9B487,0x00FDC89B,0x00FFDFB2,
	0x00390000,0x00440000,0x0050000A,0x005C0F17,0x00681B23,0x00752931,0x0084373F,0x0092464E,
	0x009C5058,0x00AC6068,0x00BD7179,0x00CE838A,0x00DF949C,0x00F2A7AF,0x00FFBBC3,0x00FFD2DA,
	0x00370020,0x0043002C,0x004E0037,0x005A0044,0x00661350,0x0074215D,0x0082306C,0x00903E7A,
	0x009B4984,0x00AA5994,0x00BC6AA5,0x00CD7BB6,0x00DE8CC7,0x00F1A0DB,0x00FFB4EF,0x00FFCBFF,
	0x002B0047,0x00360052,0x0042005E,0x004E006A,0x005A1276,0x00672083,0x00762F92,0x00843DA0,
	0x008E48AA,0x009E58BA,0x00AF69CB,0x00C07ADC,0x00D18CED,0x00E59FFF,0x00F9B3FF,0x00FFCAFF,
	0x0016005F,0x0021006A,0x002D0076,0x00390C82,0x0045198D,0x0053279B,0x006135A9,0x006F44B7,
	0x007A4EC2,0x008A5ED1,0x009B6FE2,0x00AC81F3,0x00BD92FF,0x00D0A5FF,0x00E4B9FF,0x00FBD0FF,
	0x00000063,0x0000006F,0x00140C7A,0x00201886,0x002C2592,0x003A329F,0x004841AE,0x00574FBC,
	0x00615AC6,0x00716AD6,0x00827BE7,0x00948CF8,0x00A59DFF,0x00B8B1FF,0x00CCC5FF,0x00E3DCFF,
	0x00000054,0x00000F5F,0x00001B6A,0x00002776,0x00153382,0x00234190,0x0031509E,0x00405EAC,
	0x004A68B6,0x005A78C6,0x006B89D7,0x007D9BE8,0x008EACF9,0x00A1BFFF,0x00B5D3FF,0x00CCEAFF,
	0x00001332,0x00001E3E,0x00002A49,0x00003655,0x00004261,0x0012506F,0x00205E7D,0x002F6D8B,
	0x00397796,0x004987A6,0x005B98B7,0x006CA9C8,0x007DBAD9,0x0091CEEC,0x00A5E2FF,0x00BCF9FF,
	0x00001F00,0x00002A12,0x0000351E,0x0000422A,0x00004E36,0x000B5B44,0x00196A53,0x00287861,
	0x0033826B,0x0043927B,0x0054A38C,0x0065B49E,0x0077C6AF,0x008AD9C2,0x009EEDD6,0x00B5FFED,
	0x00002400,0x00003000,0x00003B00,0x00004700,0x0000530A,0x00106118,0x001E6F27,0x002D7E35,
	0x00378840,0x00479850,0x0059A961,0x006ABA72,0x007BCB84,0x008FDE97,0x00A3F2AB,0x00BAFFC2,
	0x00002300,0x00002F00,0x00003A00,0x00004600,0x00115200,0x001F6000,0x002E6E00,0x003C7C12,
	0x0047871C,0x0057972D,0x0068A83E,0x0079B94F,0x008ACA61,0x009EDD74,0x00B2F189,0x00C9FFA0,
	0x00001B00,0x00002700,0x000F3200,0x001C3E00,0x00284A00,0x00365800,0x00446600,0x00527500,
	0x005D7F00,0x006D8F19,0x007EA02B,0x008FB13D,0x00A0C24E,0x00B4D662,0x00C8EA76,0x00DFFF8D,
	0x00110E00,0x001D1A00,0x00292500,0x00353100,0x00413D00,0x004F4B00,0x005D5A00,0x006B6800,
	0x0076720B,0x0085821B,0x0097932D,0x00A8A43E,0x00B9B650,0x00CCC963,0x00E0DD77,0x00F7F48F,
};
#endif
uint32_t *atari_4_phase_pal_ram = 0;
uint32_t* pal_palette()
{
	if (!atari_4_phase_pal_ram)
	{
		atari_4_phase_pal_ram = new uint32_t[512];
		memcpy(atari_4_phase_pal_ram, atari_4_phase_pal, 512 * 4);  // copy into ram as we are tight on static mem
	}
	return atari_4_phase_pal_ram;
}

uint16_t* rgb16palette()
{
	uint16_t* tempPointer;
	tempPointer = new uint16_t[256];
	return tempPointer;
}
//uint32_t* rgb_palette() { return (uint32_t*)atari_palette_rgb; };

void pal_init()
{
	int cc_width = 4;
	_sample_rate = PAL_FREQUENCY * cc_width / 1000000.0;       // DAC rate in mhz
	_line_width = PAL_COLOR_CLOCKS_PER_SCANLINE * cc_width;
	_line_count = PAL_LINES;
	_hsync_short = usec(2);
	_hsync_long = usec(30);
	_hsync = usec(4.7);
	_burst_start = usec(5.6);
	_burst_width = (int)(10 * cc_width + 4) & 0xFFFE;
	_active_start = usec(10.4);

	// make colorburst tables for even and odd lines
	_burst0 = new int16_t[_burst_width];
	_burst1 = new int16_t[_burst_width];
	float phase = 2 * M_PI / 2;
	for (int i = 0; i < _burst_width; i++)
	{
		_burst0[i] = BLANKING_LEVEL + sin(phase + 3 * M_PI / 4) * BLANKING_LEVEL / 1.5;
		_burst1[i] = BLANKING_LEVEL + sin(phase - 3 * M_PI / 4) * BLANKING_LEVEL / 1.5;
		phase += 2 * M_PI / cc_width;
	}
}

bool video_init(bool doubleBuffer, bool par9488)
{
	if (!par9488)
	{
		_samples_per_cc = 4;
		_palette = (uint32_t*)pal_palette();
		if (doubleBuffer)
		{
			_Frame0lines = (uint8_t**)heap_caps_malloc(Screen_HEIGHT * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame0lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				free(screen);
				return false;
			}

			for (int y = 0; y < Screen_HEIGHT; y++)
			{
				_Frame0lines[y] = (unsigned char *)heap_caps_malloc(320, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame0lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}

			_Frame1lines = (uint8_t**)heap_caps_malloc(Screen_HEIGHT * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame1lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				free(screen);
				return false;
			}

			for (int y = 0; y < Screen_HEIGHT; y++)
			{
				_Frame1lines[y] = (unsigned char *)heap_caps_malloc(320, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame1lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}
		}
		else
		{
			_Frame0lines = (uint8_t**)heap_caps_malloc(Screen_HEIGHT * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame0lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				return false;
			}

			for (int y = 0; y < Screen_HEIGHT; y++)
			{
				_Frame0lines[y] = (unsigned char *)heap_caps_malloc(320, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame0lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}
		}

		_lines = _Frame0lines;
		pal_init();
		_pal_ = 1;

		_active_lines = 240;

		if (start_dma(_line_width, _samples_per_cc, 1) == -1)
			return false;
		else
			return true;
	}
	else // Parallel 9488 - 8 bit
	{
		_palette16 = (uint16_t*)rgb16palette();
		if (!_palette16)
		{
			printf("Cannot allocate screen buffer - _palette16!\n");
			return false;
		}

		dmaBuffer9488 = (uint16_t*)heap_caps_malloc(_maxX * sizeof(uint16_t) * 6, MALLOC_CAP_DMA); // 4 lines at a time
		if (!dmaBuffer9488)
		{
			printf("Cannot allocate screen buffer - dmaBuffer9488!\n");
			return false;
		}

		if (doubleBuffer)
		{
			_Frame0lines = (uint8_t**)heap_caps_malloc(_maxY * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame0lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				free(screen);
				return false;
			}

			for (int y = 0; y < _maxY; y++)
			{
				_Frame0lines[y] = (unsigned char *)heap_caps_malloc(_maxX, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame0lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}

			_Frame1lines = (uint8_t**)heap_caps_malloc(_maxY * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame1lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				free(screen);
				return false;
			}

			for (int y = 0; y < _maxY; y++)
			{
				_Frame1lines[y] = (unsigned char *)heap_caps_malloc(_maxX, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame1lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}
		}
		else // single buffer
		{
			_Frame0lines = (uint8_t**)heap_caps_malloc(_maxY * sizeof(uint8_t*), MALLOC_CAP_DMA);
			if (_Frame0lines == NULL)
			{
				printf("Cannot allocate screen buffer - lines!\n");
				free(screen);
				return false;
			}

			for (int y = 0; y < _maxY; y++)
			{
				_Frame0lines[y] = (unsigned char *)heap_caps_malloc(_maxX, MALLOC_CAP_DMA); //(uint8_t*)s;
				if (_Frame0lines[y] == NULL)
				{
					printf("Cannot allocate screen buffer - lines!\n");
					return false;
				}
			}
		}

		_lines = _Frame0lines;
		_9488_lineIndex = 0;
		
		// DMA descriptor
		_dma_desc[0].buf = (volatile uint8_t*)dmaBuffer9488;
		_dma_desc[0].owner = 1;
		_dma_desc[0].eof = 1;
		_dma_desc[0].sosf = 0;
		_dma_desc[0].length = _maxX * sizeof(uint16_t) * 6;
		_dma_desc[0].size = _maxX * sizeof(uint16_t) * 6;
		_dma_desc[0].empty = 0;
		_dma_desc[0].offset = 0;

		if (start_dma_9488() == -1)
			return false;
		else
			return true;
	}
	return true;
}

void IRAM_ATTR blit_pal(uint8_t* src, uint16_t* dst)
{
    uint32_t c,color;
    bool even = _line_counter & 1;
    const uint32_t* p = even ? _palette : _palette + 256;
    int left = 0;
    int right = 256;
    uint8_t mask = 0xFF;
    uint8_t c0,c1,c2,c3,c4;
    uint8_t y1,y2,y3;

	left = 0;
	right = 320; // only show center 336 pixels

	dst += 60;
    for (int i = left; i < right; i += 4) 
	{
        c = *((uint32_t*)(src+i));

        // make 5 colors out of 4 by interpolating y: 0000 0111 1122 2223 3333
        c0 = c;
        c1 = c >> 8;
        c3 = c >> 16;
        c4 = c >> 24;
        y1 = (((c1 & 0xF) << 1) + ((c0 + c1) & 0x1F) + 2) >> 2;    // (c0 & 0xF)*0.25 + (c1 & 0xF)*0.75;
        y2 = ((c1 + c3 + 1) >> 1) & 0xF;                           // (c1 & 0xF)*0.50 + (c2 & 0xF)*0.50;
        y3 = (((c3 & 0xF) << 1) + ((c3 + c4) & 0x1F) + 2) >> 2;    // (c2 & 0xF)*0.75 + (c3 & 0xF)*0.25;
        c1 = (c1 & 0xF0) + y1;
        c2 = (c1 & 0xF0) + y2;
        c3 = (c3 & 0xF0) + y3;

        color = p[c0];
        dst[0^1] = _P0;
        dst[1^1] = _P1;
        color = p[c1];
        dst[2^1] = _P2;
        dst[3^1] = _P3;
        color = p[c2];
        dst[4^1] = _P0;
        dst[5^1] = _P1;
        color = p[c3];
        dst[6^1] = _P2;
        dst[7^1] = _P3;
        color = p[c4];
        dst[8^1] = _P0;
        dst[9^1] = _P1;

        i += 4;
        c = *((uint32_t*)(src+i));
                
        // make 5 colors out of 4 by interpolating y: 0000 0111 1122 2223 3333
        c0 = c;
        c1 = c >> 8;
        c3 = c >> 16;
        c4 = c >> 24;
        y1 = (((c1 & 0xF) << 1) + ((c0 + c1) & 0x1F) + 2) >> 2;    // (c0 & 0xF)*0.25 + (c1 & 0xF)*0.75;
        y2 = ((c1 + c3 + 1) >> 1) & 0xF;                           // (c1 & 0xF)*0.50 + (c2 & 0xF)*0.50;
        y3 = (((c3 & 0xF) << 1) + ((c3 + c4) & 0x1F) + 2) >> 2;    // (c2 & 0xF)*0.75 + (c3 & 0xF)*0.25;
        c1 = (c1 & 0xF0) + y1;
        c2 = (c1 & 0xF0) + y2;
        c3 = (c3 & 0xF0) + y3;

        color = p[c0];
        dst[10^1] = _P2;
        dst[11^1] = _P3;
        color = p[c1];
        dst[12^1] = _P0;
        dst[13^1] = _P1;
        color = p[c2];
        dst[14^1] = _P2;
        dst[15^1] = _P3;
        color = p[c3];
        dst[16^1] = _P0;
        dst[17^1] = _P1;
        color = p[c4];
        dst[18^1] = _P2;
        dst[19^1] = _P3;
        dst += 20;
    }
}

void IRAM_ATTR burst_pal(uint16_t* line)
{
    line += _burst_start;
    int16_t* b = (_line_counter & 1) ? _burst0 : _burst1;
    for (int i = 0; i < _burst_width; i += 2) {
        line[i^1] = b[i];
        line[(i+1)^1] = b[i+1];
    }
}

void IRAM_ATTR burst(uint16_t* line)
{
	burst_pal(line);
}

void IRAM_ATTR sync(uint16_t* line, int syncwidth)
{
    for (int i = 0; i < syncwidth; i++)
        line[i] = SYNC_LEVEL;
}

void IRAM_ATTR blanking(uint16_t* line, bool vbl)
{
    int syncwidth = vbl ? _hsync_long : _hsync;
    sync(line,syncwidth);
    for (int i = syncwidth; i < _line_width; i++)
        line[i] = BLANKING_LEVEL;
    if (!vbl)
        burst(line);    // no burst during vbl
}

// Fancy pal non-interlace
// http://martin.hinner.info/vga/pal.html
void IRAM_ATTR pal_sync2(uint16_t* line, int width, int swidth)
{
    swidth = swidth ? _hsync_long : _hsync_short;
    int i;
    for (i = 0; i < swidth; i++)
        line[i] = SYNC_LEVEL;
    for (; i < width; i++)
        line[i] = BLANKING_LEVEL;
}

uint8_t DRAM_ATTR _sync_type[8] = {0,0,0,3,3,2,0,0};
void IRAM_ATTR pal_sync(uint16_t* line, int i)
{
    uint8_t t = _sync_type[i-304];
    pal_sync2(line,_line_width/2, t & 2);
    pal_sync2(line+_line_width/2,_line_width/2, t & 1);
}
// Wait for blanking before starting drawing
// avoids tearing in our unsynchonized world
#ifdef ESP_PLATFORM
void video_sync()
{
	if (!_lines)
		return;
	int n = 0;
	if (_line_counter < _active_lines)
		n = (_active_lines - _line_counter) * 1000 / 15600;
	vTaskDelay(n + 1);
}
#endif

// Workhorse ISR handles audio and video updates
extern "C"
void IRAM_ATTR video_isr(volatile void* vbuf)
{
    if (!_lines)
        return;
    
	int i = _line_counter++;
    uint16_t* buf = (uint16_t*)vbuf;
    // pal
    if (i < 32) 
	{
        blanking(buf,false);                // pre render/black 0-32
    } 
	else if (i < (_active_lines + 32)) 
	{    // active video 32-272
        sync(buf,_hsync);
        burst(buf);
		blit_pal(_lines[i - 32], buf + _active_start);
    }
	else if (i < 304) 
	{                   // post render/black 272-304
        blanking(buf,false);
    } 
	else 
	{
        pal_sync(buf,i);                    // 8 lines of sync 304-312
    }

    if (_line_counter == _line_count) 
	{
        _line_counter = 0;                      // frame is done
        _frame_counter++;
    }
}

#endif // VIDEO_OUT
