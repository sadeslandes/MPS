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
char timer0_flag = 0;
//-------------------------------------------------------
// Function PROTOTYPES
//-------------------------------------------------------
void main(void);
void PORT_INIT(void);
void SYSCLK_INIT(void);
void UART0_INIT(void);
void TIMER0_INIT(void);
void TIMER0_ISR(void) __interrupt 1;

//------------------------------------------------------
// Main Function 
//------------------------------------------------------

void main(void){
	unsigned int tenths = 0;

	SFRPAGE = CONFIG_PAGE;

	PORT_INIT();
	TIMER0_INIT();
	SYSCLK_INIT();
	UART0_INIT();

	SFRPAGE = LEGACY_PAGE;
	IT0 = 1;    // /INT0 triggered on negative falling edge

	printf("\033[2J");
	printf("MPS Interrupt Timer Test \n\n\r");
	
	SFRPAGE = CONFIG_PAGE;
	EX0 = 1;    // Enable external interrupt

	SFRPAGE = UART0_PAGE;
	
	
	while(1){
		if(timer0_flag == 3){ // Wait for 3 overflows, print elapsed time and reset flag
			tenths+=1;        
			printf_fastf("Elapsed Time: %.2f\n\r", tenths*0.1);
			timer0_flag = 0;
		}
	}
}
//-------------------------------------
// Interrupts 
//-------------------------------------

void TIMER0_ISR(void) __interrupt 1{
	// Reset timer0 value and increment flag
	TH0 = 0x00;
	TL0 = 0x00;
	timer0_flag += 1;
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
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page.
}

//--------------------------------------------------
// SYSCLK_Init
//--------------------------------------------------

// Initialize the system clock 22.1184Mhz

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
	
	CLKSEL  = 0x01;             // SYSCLK derived from external oscillator.
	
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
	TH1     = (unsigned char)-(EXTCLK/BAUDRATE/16); // Set Timer1 reload value for baudrate
	CKCON  |= 0x10;				// Timer1 uses SYSCLK as time base.
	TL1     = TH1;
	TR1     = 1;                // Start Timer1.
	
	SFRPAGE = UART0_PAGE;
	SCON0   = 0x50;             // Set Mode 1: 8-Bit UART
	SSTA0   = 0x10;             // UART0 baud rate divide-by-two disabled (SMOD0 = 1).
	TI0     = 1;                // Indicate TX0 ready.
	
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page
}

// Timer init
void TIMER0_INIT(void){
	char SFRPAGE_SAVE;
	SFRPAGE_SAVE = SFRPAGE;

	SFRPAGE = TIMER01_PAGE;	

	TMOD &= 0xF0; 				// Timer0, Mode 1: 16-bit counter/timer.
	TMOD |= 0x01;
	TH0 = 0x00;					// Set high byte to 0
	CKCON &= ~0x0B;				// Timer0 uses SYSCLK/12 as base
	TL0 = 0x00;					// Set low byte to 0
	TR0 = 1;					// Start timer0

	SFRPAGE = CONFIG_PAGE;
	ET0 = 1;					// Enable timer0 interrupt

	SFRPAGE = SFRPAGE_SAVE;
}