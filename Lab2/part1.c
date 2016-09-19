//---------------------------------------------------
// Includes
//---------------------------------------------------
#include <c8051f120.h>  
#include <stdio.h>
#include "putget.h"     
        
//---------------------------------------------------------
// Global CONSTANTS
//---------------------------------------------------------

#define EXTCLK      22118400    // External oscillator frequency in Hz
#define SYSCLK      49766400    // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200      // UART baud rate in bps
//#define BAUDRATE  19200       // UART baud rate in bps
//__bit SW2press = 0;
char butpress;

//-------------------------------------------------------
// Function PROTOTYPES
//-------------------------------------------------------
void main(void);
void PORT_INIT(void);
void SYSCLK_INIT(void);
void UART0_INIT(void);
void SW2_ISR (void) __interrupt 0;

//------------------------------------------------------
// Main Function 
//------------------------------------------------------

void main(void){
	__bit restart = 0;

	SFRPAGE = CONFIG_PAGE;

	PORT_INIT();
	SYSCLK_INIT();
	UART0_INIT();

	SFRPAGE = LEGACY_PAGE;
	IT0 = 1; 

	printf("\033[2J");
	printf("MPS Interrupt Switch Test \n\n\r");
	printf("Ground /INT0 on P0.2 to generate an interrupt. \n\n\r");

	SFRPAGE = CONFIG_PAGE;
	EX0 = 1; 

	SFRPAGE = UART0_PAGE;
	
	butpress = 0;
	while(1){
		if(butpress == 1){
			printf("/INT0 grounded! \n\n\r");
			butpress = 0;
		}
	}
}
//-------------------------------------
// Interrupts 
//-------------------------------------

void SW2_ISR (void) __interrupt 0{
	butpress = 1;	 
}

//------------------------------------------------------
// PORT_Init
//------------------------------------------------------
// Configure the Crossbar and GPIO ports

void PORT_INIT(void){
	char SFRPAGE_SAVE;

	SFRPAGE_SAVE = SFRPAGE;     // Save Current SFR page.

	SFRPAGE = CONFIG_PAGE;
	WDTCN   = 0xDE;             // Disable watchdog timer.
	WDTCN   = 0xAD;
	EA      = 1;                // Enable interrupts as selected.

	XBR0    = 0x04;             // Enable UART0.
	XBR1    = 0x04;             // /INT0 routed to port pin.
	XBR2    = 0x40;             // Enable Crossbar and weak pull-ups.
	P0MDOUT = 0x01;             // P0.0 (TX0) is configured as Push-Pull for output
	// P0.1 (RX0) is configure as Open-Drain input.
	// P0.2 (SW2 through jumper wire) is configured as Open_Drain for input.
	P0      = 0x06;             // Additionally, set P0.0=0, P0.1=1, and P0.2=1.
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page.
}

//--------------------------------------------------
// SYSCLK_Init
//--------------------------------------------------

// Initialize the system clock

void SYSCLK_INIT(void){
	int i;
	
	char SFRPAGE_SAVE;
	
	SFRPAGE_SAVE = SFRPAGE;     // Save Current SFR page.
	
	SFRPAGE = CONFIG_PAGE;
	OSCXCN  = 0x67;             // Start external oscillator
	for(i=0; i < 256; i++);     // Wait for the oscillator to start up.
	while(!(OSCXCN & 0x80));    // Check to see if the Crystal Oscillator Valid Flag is set.
	CLKSEL  = 0x01;             // SYSCLK derived from the External Oscillator circuit.
	OSCICN  = 0x00;             // Disable the internal oscillator.
	
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
	CLKSEL  = 0x02;             // SYSCLK derived from the PLL.
	
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page.
}

//-----------------------------------------------------
// UART0_Init
//-----------------------------------------------------

// Configure the UART0 using Timer1, for <baudrate> and 8-N-1.

void UART0_INIT(void){
	char SFRPAGE_SAVE;
	
	SFRPAGE_SAVE = SFRPAGE;     // Save Current SFR page.
	
	SFRPAGE = TIMER01_PAGE;
	TMOD   &= ~0xF0;
	TMOD   |=  0x20;            // Timer1, Mode 2: 8-bit counter/timer with auto-reload.
	TH1     = (unsigned char)-(SYSCLK/BAUDRATE/16); // Set Timer1 reload value for baudrate
	CKCON  |= 0x10;				// Timer1 uses SYSCLK as time base.
	TL1     = TH1;
	TR1     = 1;                // Start Timer1.
	
	SFRPAGE = UART0_PAGE;
	SCON0   = 0x50;             // Set Mode 1: 8-Bit UART
	SSTA0   = 0x10;             // UART0 baud rate divide-by-two disabled (SMOD0 = 1).
	TI0     = 1;                // Indicate TX0 ready.
	
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page
}