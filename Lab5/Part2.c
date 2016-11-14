
//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f120.h>
#include <stdio.h>
#include <ctype.h>
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
	__xdata unsigned char *int_ram;
	__xdata unsigned char *ext_ram;
	unsigned int memAddr;
	unsigned char c,r;
	unsigned char i;	
	unsigned long wait;

	WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	
	SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
    printf("Test of the printf() function.\n\n\r");
	
	
	while(1){
		printf("Enter a memory address (1FF0 - 1FFF) and a hex value (0-f) to input\n\r0x");
		memAddr = 0;
		for(i=0;i<4;i++){
			c = getchar();
			putchar(c);
			if(isdigit(c)){
				memAddr = memAddr*16 + (c-'0');
			}
			else if(c < 'g' && c > '`'){
				memAddr = memAddr*16 + (c-'W');
			}
			else if(c < 'G' && c > '@'){
				memAddr = memAddr*16 + (c-'7');
			}
			else{
				printf("\n\rBad input. Restart program.");
				while(1);
			}
		}
		printf(", 0x");
		c = getchar();
		putchar(c);
		
		if(isdigit(c)){
			c= (c-'0');
		}
		else if(c < 'g' && c > '`'){
			c= (c-'W');
		}
		else if(c < 'G' && c > '@'){
			c= (c-'7');
		}
		else{
			printf("\n\rBad input. Restart program.");
			while(1);
		}

		int_ram = (__xdata unsigned char *)(memAddr);
		*int_ram = c;
		r = *int_ram;
		
		printf("\n\rValue '0x%x' written to memory address 0x%4x.\n\r",*int_ram,memAddr);
		printf("Press a key to write '0x%x' to all external memory...\n\r", *int_ram);
		getchar();
		
		for(memAddr = 0x2000; memAddr < 0x4400; memAddr++)
	    {

			ext_ram = (__xdata unsigned char *)(memAddr);
			*ext_ram = r;
			
			for(i=0;i<100;i++);
			printf("Value at address 0x%4x: 0x%x\n\r",memAddr,*ext_ram);
		}
		
		while(1);
	}
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