/**
 * In recognition of the original code from the r0ket project, this is
 * one big ugly mess
 */

#include <stdint.h>
#include "core/usbcdc/usb.h"
#include "core/usbcdc/usbcore.h"
#include "core/usbcdc/usbhw.h"
#include "core/usbcdc/cdcuser.h"
#include "core/usbcdc/cdc_buf.h"
#include <sysinit.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "lcd/print.h"
#include "lcd/allfonts.h"


unsigned char hexdigit(int n) {
	static unsigned char digits[17]="0123456789abcdef-";

	if(n<0) n=16;
	if(n>15) n=16;

	return digits[n];
}

volatile unsigned int lastTick;

int puts(const char * str) {
	// There must be at least 1ms between USB frames (of up to 64 bytes)
	// This buffers all data and writes it out from the buffer one frame
	// and one millisecond at a time
	if (USB_Configuration) {
		while (*str)
			cdcBufferWrite(*str++);
		// Check if we can flush the buffer now or if we need to wait
		unsigned int currentTick = systickGetTicks();
		if (currentTick != lastTick) {
			uint8_t frame[64];
			uint32_t bytesRead = 0;
			while (cdcBufferDataPending()) {
				// Read up to 64 bytes as long as possible
				bytesRead = cdcBufferReadLen(frame, 64);
				USB_WriteEP(CDC_DEP_IN, frame, bytesRead);
				systickDelay(1);
			}
			lastTick = currentTick;
		}
	}
	return 0;
}

/**
 * \brief	Get string from serial.
 *
 * \param buf	pointer to the character buffer
 * \param len	max bytes to read
 * \return bytes read, or negative on error
 */
int getstr(uint8_t *buf, int len)
{
	int idx = 0;
	int available = 0;
	int r = 0;
	int read_bytes = 1;
	uint8_t read_buf[2] = {0,0};
	unsigned char dbg[3] = {0,0,0};

	CDC_OutBufAvailChar(&available);

	if(0 == available)
		return -1;

	for(idx=0;idx<len;idx++)
		buf[idx]=0;

	idx=0;

	while(idx<len) {
		//DoString(0,2,"r");
		//lcdDisplay();
		CDC_OutBufAvailChar(&available);
		//DoString(0,2,"R");
		//lcdDisplay();
		if(0 < available) {
			for(r=0;r<available;r++) {
				read_bytes = 1;
				//DoString(0,3,"rr");
				//lcdDisplay();
				CDC_RdOutBuf(read_buf, &read_bytes);
				//DoString(0,3,"rR");
				//lcdDisplay();

				//dbg[0]=hexdigit((read_buf[0]>>4) & 0xf);
				//dbg[1]=hexdigit((read_buf[0]) & 0xf);
				//lcdPrintln(dbg);
				//lcdRefresh();

				buf[idx]=read_buf[0];
				idx++;
				if(10==read_buf[0] || 13==read_buf[0])
					return idx;
				if(idx==len)
					return idx;
			}
		}
	}

	return idx;
}

void main_thermo(void) {
	int dx = 0;
	uint8_t buf[128] = { 0, 0 };
	uint8_t setting[128] = { 0 };
	uint8_t temp[128] = { 0 };
	int l;
	uint32_t usbTimeout = 0;
	int refresh = 0;
	uint8_t button = BTN_NONE;

	//lastTick = systickGetTicks();   // Used to control output/printf timing

	lcdPrintln("Init USB");
	lcdRefresh();

	CDC_Init();                     // Initialise VCOM
	USB_Init();                     // USB Initialization
	USB_Connect(TRUE);              // USB Connect

	// Wait until USB is configured or timeout occurs
	while (usbTimeout < CFG_USBCDC_INITTIMEOUT / 10) {
		if (USB_Configuration)
			break;
		delayms(10);             // Wait 10ms
		usbTimeout++;
	}
	lcdPrintln("Done");
	lcdRefresh();

	lcdPrintln("Init thermo");
	lcdRefresh();

	while (1) {
		button = getInput();

		if(BTN_UP == button) {
			puts("Bup\n");
		}
		if(BTN_DOWN == button) {
			puts("Bdn\n");
		}

		l = getstr(buf, 128);

		if(0>l) {
			puts(buf);

			refresh = 0;

			if(strlen(buf)>1) {
				if('T'==buf[0]) {
					memcpy(temp, buf+1, 127);
					refresh = 1;
				}
				if('S'==buf[0]) {
					memcpy(setting, buf+1, 127);
					refresh = 1;
				}
			}

			if(0 != refresh) {
				lcdFill(0);
				DoString(20,30,"t");
				DoString(20,50,"set");
				DoString(45,30,temp);
				DoString(45,50,setting);
				lcdDisplay();
			}
		}
	}
}
