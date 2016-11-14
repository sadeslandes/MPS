// Nick Choi and Samuel Deslandes
// Code for Part 2 of Lab 5
// This program first writes 0xAA to external memory addresses 0x2800-0x2FFF,
// then overwrites these addresses with 0x55. Any addresses that do not read back 0x55
// are added to an array which is printed at the end of the program.
//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f120.h>
#include <stdio.h>
#include "putget.h"
//------------------------------------------------------------------------------------
// Global Constants
//------------------------------------------------------------------------------------
#define EXTCLK      22118400            // External oscillator frequency in Hz
#define SYSCLK      49766400            // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200              // UART baud rate in bps

//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
	__xdata unsigned int *error_ptr;
	__xdata unsigned char *ext_ram;
	unsigned static int __xdata count[512];
	unsigned int size = 0;
	unsigned int memAddr;
	unsigned char c;
	unsigned char i;	

	WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	
	SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
    printf("Test of the printf() function.\n\n\r");
	
	c = 0xAA;
	for(memAddr = 0x2800; memAddr < 0x3000; memAddr++)
    {
		ext_ram = (__xdata unsigned char *)(memAddr);
		*ext_ram = c;

		for(i=0;i<100;i++);
		printf("Value on external xram: 0x%x\t0x%4x\n\n\r",*ext_ram,memAddr);		// read back value 
	}
	error_ptr = count;
	c = 0x55;
	for(memAddr = 0x2800; memAddr < 0x3000; memAddr++)
    {
		ext_ram = (__xdata unsigned char *)(memAddr);
		*ext_ram = c;

		for(i=0;i<100;i++);
		printf("Value on external xram: 0x%x\t0x%4x\n\n\r",*ext_ram,memAddr);		// read back value
		if(*ext_ram != 0x55){						// if expected value is not read back add faulty address to array
			*error_ptr = memAddr;
			error_ptr += 1;
			size += 1;
		} 
		if(size == 512){							// if error array is full empty contents to terminal
			for(size;size!=0;size--){
				pritnf("0x%4x\n\r",count[size-1]);	
				error_ptr = count;
			}
		}
	}
	
	for(size;size!=0;size--){  						// print error array 
		printf(pritnf("0x%4x\n\r",count[size-1]););
	}
	while(1);
}


//------------------------------------------------------------------------------------
// SYSCLK_Init
//------------------------------------------------------------------------------------
//
// Initialize the system clock to use a 22.1184MHz crystal as its clock source
//
void SYSCLK_INIT(void)
{
    int i;
    char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;             // Save Current SFR page

    SFRPAGE = CONFIG_PAGE;
    OSCXCN  = 0x67;                     // Start ext osc with 22.1184MHz crystal
    for(i=0; i < 256; i++);             // Wait for the oscillator to start up
    while(!(OSCXCN & 0x80));
    CLKSEL  = 0x01;
    OSCICN  = 0x00;

    SFRPAGE = CONFIG_PAGE;
    PLL0CN  = 0x04;
    SFRPAGE = LEGACY_PAGE;
    FLSCL   = 0x10;
    SFRPAGE = CONFIG_PAGE;
    PLL0CN |= 0x01;
    PLL0DIV = 0x04;
    PLL0FLT = 0x01;
    PLL0MUL = 0x09;
    for(i=0; i < 256; i++);
    PLL0CN |= 0x02;
    while(!(PLL0CN & 0x10));
    CLKSEL  = 0x02;

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

//------------------------------------------------------------------------------------
// PORT_Init
//------------------------------------------------------------------------------------
//
// Configure the Crossbar and GPIO ports
//
void PORT_INIT(void)
{    
    char SFRPAGE_SAVE = SFRPAGE; 	// Save Current SFR page
	
	SFRPAGE = CONFIG_PAGE;
	XBR0 = 0x04;    				// Enable UART0
	XBR1 = 0x00;
	XBR2 = 0x40; 					// Enable Crossbar and weak pull-up
	P0MDOUT |= 0x01; 				// Set TX0 pin to push-pull
	P4MDOUT = 0xFF; 				// Output configuration for P4 all pushpull
	P5MDOUT = 0xFF; 				// Output configuration for P5 pushpull EM addr
	P6MDOUT = 0xFF; 				// Output configuration for P6 pushpull EM addr
	P7MDOUT = 0xFF; 				// Output configuration for P7 pushpull EM data
	
	// EMI_Init, split mode with banking
	SFRPAGE = EMI0_PAGE;
	EMI0CF = 0x3b; 
	EMI0TC = 0xFF;
	
	SFRPAGE = SFRPAGE_SAVE; 		// Restore SFR page
}

//------------------------------------------------------------------------------------
// UART0_Init
//------------------------------------------------------------------------------------
//
// Configure the UART0 using Timer1, for <baudrate> and 8-N-1
//
void UART0_INIT(void)
{
    char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;             // Save Current SFR page

    SFRPAGE = TIMER01_PAGE;
    TMOD   &= ~0xF0;
    TMOD   |=  0x20;                    // Timer1, Mode 2, 8-bit reload
    TH1     = -(SYSCLK/BAUDRATE/16);    // Set Timer1 reload baudrate value T1 Hi Byte
    CKCON  |= 0x10;                     // Timer1 uses SYSCLK as time base
    TL1     = TH1;
    TR1     = 1;                        // Start Timer1

    SFRPAGE = UART0_PAGE;
    SCON0   = 0x50;                     // Mode 1, 8-bit UART, enable RX
    SSTA0   = 0x10;                     // SMOD0 = 1
    TI0     = 1;                        // Indicate TX0 ready

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}