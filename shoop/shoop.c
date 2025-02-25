/**
 * shoop.c -- Demonstrate HIDAPI via commandline
 *
 * 2025, Jakub Ladman / github.com/ladmanj
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

#include "hidapi.h"


#define MAX_STR 1024  // for manufacturer, product strings
#define MAX_BUF 1024  // for buf reads & writes

// normally this is obtained from git tags and filled out by the Makefile
#ifndef SHOOP_VERSION
#define SHOOP_VERSION "v0.0"
#endif



bool msg_quiet = false;
bool msg_verbose = false;

int print_base = 16; // 16 or 10, hex or decimal
int print_width = 32; // how many characters per line
/**
 * printf that can be shut up
 */
void msg(char* fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    if(!msg_quiet) { vprintf(fmt,args); }
    va_end(args);
}
/**
 * printf that is wordy
 */
void msginfo(char* fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    if(msg_verbose) { vprintf(fmt,args); }
    va_end(args);
}

/**
 * print out a buffer of len bufsize in decimal or hex form
 */
void printbuf(uint8_t* buf, int bufsize, int base, int width)
{
    for( int i=0 ; i<bufsize; i++) {
        if( base==10 ) {
            printf(" %3d", buf[i]);
        } else if( base==16 ) {
            printf(" %02X", buf[i] );
        }
       // if (i % 16 == 15 && i < bufsize-1) printf("\n");
       if (i % width == width-1 && i < bufsize-1) printf("\n");
    }
    printf("\n");
}

/**
 *
 */
int main(int argc, char* argv[])
{
	uint8_t buf[MAX_BUF];   // data buffer for send/recv
	hid_device *dev = NULL; // HIDAPI device we will open
	int res;
	int buflen = 64;        // length of buf in use

	uint16_t vid = 0x0483;        // productId
	uint16_t pid = 0x5750;        // vendorId

	setbuf(stdout, NULL);  // turn off buffering of stdout

	memset(buf,0, MAX_BUF);   // reset buffers


	if( vid && pid) {
		msg("Opening device, vid/pid: 0x%04X/0x%04X\n",vid,pid);
		dev = hid_open(vid,pid,NULL);
	}

	if(2 == argc)
		strncpy((char*)buf,argv[1],sizeof(buf));
	else
		strncpy((char*)buf,"send",sizeof(buf));

	int parsedlen = strlen((char*)buf)+1;
	if( parsedlen<1 ) { // no bytes or error
		msg("Error: no bytes read as arg to --send...");
	}
	buflen = parsedlen;

	if( !dev ) {
		msg("Error on send: no device opened.\n");
	}
	else
	{
		msg("Writing output report of %d-bytes...",buflen);
		res = hid_write(dev, buf, buflen);

		msg("wrote %d bytes:\n", res);
		if(!msg_quiet) { printbuf(buf,buflen, print_base, print_width); }

		if(dev) {
			msg("Closing device\n");
			hid_close(dev);
		}
	}
	res = hid_exit();

} // main
