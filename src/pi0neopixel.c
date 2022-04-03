#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <fcntl.h>

#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <sched.h>
#include <math.h>

// define if NeoPixel hardware is WS2812B
#define WS2812B

// The number of NeoPixel elements
#define NbLeds 8

// define the nibble values when converting color bits into the NeoPixel signal
// values in the T0H-T0L and T1H-T1L format, may require adjustment for SPI speed
#define T0MSN 0x80 // Most significant nibble for 0 bit
#define T1MSN 0xE0 // Most significant nibble for 1 bit
#define T0LSN 0x08 // Least significant nibble for 0 bit
#define T1LSN 0x0E // Least significant nibble for 1 bit

// the RGBW pixel color value will be streched out into a NeoPixel data signal with more bytes
#define RGBW_SIGNALBYTES 16 // bytes per pixel to handle NeoPixel signal with 4 bytes per color channel (R, G, B, W)

// declaore methods
void refreshDisplay(void);
void clearLeds();


// declare SPI variables
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 4000000;
static uint16_t delay;


// data structure to hold the color bytes for a single LED
typedef struct{
	unsigned char R,G,B,W; // W will not be used on ws2812b
} ledStruct;


// use union so RGBW can be accessed as a single unsigned long named dw
union ledUnion {
	unsigned long dw;
	ledStruct RGBW;
};

// array to hold the data for the LED strip
union ledUnion leds[NbLeds];

// SPI file descriptor
int fd;


// abort the application with an error message
static void pabort(const char* s)
{
	perror(s);
	abort();
}

// write an array of bytes to the SPI interface
void writeSPI(unsigned char* array, int length)
{
	int ret;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long) array,
		.rx_buf = (unsigned long) array,
		.len = length,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if(ret < 1) pabort("Can't send spi message");
}

// close the SPI interface
void closeSPI(void)
{
	if (fd >= 0)
	{
		clearLeds();
		refreshDisplay();
		close(fd);
		fd = -1;
	}
}

// handle user hitting Control C to exit application
void Ctrl_C_Handler(int value)
{
	closeSPI();
	clearLeds();
	exit(0);
}

// clear the LED array in memory
void clearLeds ()
{
       memset(leds ,0 ,sizeof(leds));
}

// fill the buffer at the provided pointer with the NeoPixel T0H-T0L/T1H-T1L signal for the given RGBW byte
unsigned char* fillColor(unsigned char* colorPt, int RGBWvalue)
{
	int loop;
	unsigned char _temp;

	// loop through 4 sets of 2 bits for the given RGBW byte
	for (loop = 0; loop < 4; loop++)
	{
		// take 2 most significant bits and spread them across one byte
		// into ws2812B signal format for T0H-T0L and T1H-T1L
		_temp = (RGBWvalue & 0x80) ? T1MSN : T0MSN;
		_temp |= (RGBWvalue & 0x40) ? T1LSN : T0LSN;
		// store signal byte in buffer
		*(colorPt++) = _temp;
		// shift to the next two bits
		RGBWvalue <<= 2;
	}

	// return the incremented pointer for potential reuse
	return colorPt;
}

// refresh the hardware display
void refreshDisplay(void)
{
	int loop;
	if(fd < 0) return;

	// set the reset 50 us minimum then at 3Mhz we will used 20 ~60us
	int ResetCount = 50;

	unsigned char* bufferSPI;
	unsigned char* bufferPtr;
	bufferSPI = malloc(ResetCount + NbLeds * RGBW_SIGNALBYTES); // RGBW_SIGNALBYTES bytes per pixel to handle NeoPixel signal with 4 bytes per color channel (R, G, B, W)

	memset(bufferSPI, 0, ResetCount);
	bufferPtr = &bufferSPI[ResetCount];
	for (loop = 0; loop < NbLeds; loop++)
	{
		bufferPtr = fillColor(bufferPtr, leds[loop].RGBW.G);
		bufferPtr = fillColor(bufferPtr, leds[loop].RGBW.R);
		bufferPtr = fillColor(bufferPtr, leds[loop].RGBW.B);
// if not WS2812B then include W channel in signal
#ifndef WS2812B
		bufferPtr = fillColor(bufferPtr, leds[loop].RGBW.W);
#endif
	}

	writeSPI(bufferSPI, ResetCount + NbLeds * RGBW_SIGNALBYTES);
	free(bufferSPI);
}

// create a random RGB color value
unsigned long  randomLed(void)
{
	int r, g, b, w;
	union ledUnion Rled;

	// set all color channels to 0
	Rled.dw = 0;
	// random color strength
	unsigned char level = (rand() % 16) + 1;

	// randomize mixed or pure color
	switch (rand() % 2)
	{
		case 0 : // mixed color
		Rled.RGBW.R = rand() % (int)((float)level / 3 + 1);
		Rled.RGBW.G = rand() % (int)((float)level / 3 + 1);
		Rled.RGBW.B = rand() % (int)((float)level / 3 + 1);
		break;

		default: // pure color R,G,B,or W
		switch (rand() % 3)
		{
			case 0 : // RED
			Rled.RGBW.R = level;
			break;

			case 1 : // GREEN
			Rled.RGBW.G = level;
			break;

			case 2 : // BLUE
			Rled.RGBW.B = level;
			break;
		}
	}

	return Rled.dw;
}


void set_max_priority(void) {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void set_default_priority(void) {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}

// initialize the SPI interface
void initSpi() {
	int ret = 0;

	fd = open("/dev/spidev0.0",O_RDWR);
	if(fd <0) pabort("Can't open device\n");

	mode = 0;

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) pabort("can't get spi mode");

	// bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) pabort("can't get bits per word");

	// max speed hz
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) pabort("can't get max speed hz");

	printf("speed = %d\n",speed);
	printf("bits  = %d\n",bits);
	printf("mode  = %d\n",mode);
}


// cylon demo
void cylon()
{
	int loop, tloop;
	int Table[NbLeds * 4];
	// create table of led pixel index incrementing then decrementing
	for (loop = 0; loop < NbLeds * 2; loop++)
	{
		Table[loop] = loop - NbLeds;
		Table[loop + NbLeds * 2] = NbLeds - loop;
	}
	for (tloop = 0; tloop < NbLeds * 4; tloop++)
	{
		int offset = Table[tloop];
		clearLeds();
		for (loop = 0; loop < NbLeds / 2; loop++)
		{
			int l = 1 * (int)pow((double)(1 + 1.25), (double)loop);
			if (offset + loop < NbLeds && offset + loop >= 0) leds[offset + loop].RGBW.R = l;
			if (offset + NbLeds - loop - 1 < NbLeds && offset + NbLeds - loop - 1 >= 0) leds[offset + NbLeds - loop - 1].RGBW.R = l;
		}
		refreshDisplay();
		usleep(100000);
	}
}

// droplets demo
void droplets(int drops)
{
	int dloop, loop, i, f, dr;

	do
	{
		// random droplet
		i = rand() % NbLeds;
		leds[i].RGBW.R = rand() % 128;
		leds[i].RGBW.G = rand() % 128;
		leds[i].RGBW.B = rand() % 128;

		dr = rand() % 10 + 1;
		for (dloop = 0; dloop < dr; dloop++)
		{
			// fade drops
			for (loop = 0; loop < NbLeds; loop++)
			{
				if (leds[loop].RGBW.R > 0)
				{
					f = rand() % ((leds[loop].RGBW.R / 4) + 1) + 1;
					leds[loop].RGBW.R -= f;
				}
				if (leds[loop].RGBW.G > 0)
				{
					f = rand() % ((leds[loop].RGBW.G / 4) + 1) + 1;
					leds[loop].RGBW.G -= f;
				}
				if (leds[loop].RGBW.B > 0)
				{
					f = rand() % ((leds[loop].RGBW.B / 4) + 1) + 1;
					leds[loop].RGBW.B -= f;
				}
			}

			refreshDisplay();
			usleep(100000);
		}
	} while (drops--);
}



int main(int argc, char *argv[])
{
	// initialize random
	time_t t;
	srand((unsigned) time(&t));

	clearLeds();

	initSpi();

	atexit(closeSPI);
	signal(SIGINT, Ctrl_C_Handler);

	set_max_priority();

	int rc, loop;
	while(1)
	{
		rc = rand() % 15;
		for (loop = 0; loop < rc; loop++) cylon();
		rc = rand() % 5;
		for (loop = 0; loop < rc; loop++) droplets(rand() % 10 + 10);
	}
	return 0;
}
