// Nick Choi and Sam Deslandes
// Code for section 2 of Lab 3.
// This program echoes characters between UART0 and UART1 via interrupts. 
// This program was designed to work with communication between two 8051 processors
// with connected UART1s. When a key is pressed on UART0 of one of the MCUs, it is 
// echoed to that of the other MCU.
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include <c8051f120.h>  
#include <stdio.h>
#include <stdlib.h>
#include "putget.h"     
        
//---------------------------------------------------------
// Global CONSTANTS
//---------------------------------------------------------

#define EXTCLK      22118400    // External oscillator frequency in Hz
#define SYSCLK      49766400    // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200      // UART baud rate in bps
//#define BAUDRATE  19200       // UART baud rate in bps
char CURR_PAGE;
char exit_flag = 0;
char UART0_flag = 0;
char UART1_flag = 0;
char choice = 0;
//-------------------------------------------------------
// Function PROTOTYPES
//-------------------------------------------------------
void main(void);
void PORT_INIT(void);
void SYSCLK_INIT(void);
void UART0_INIT(void);
void UART1_INIT(void);
void TIMER0_INIT(void);
char checkSBUF(char CUR_PAGE);
void echo(char character);
void INTERRUPT_INIT(void);
void UART0_int(void) __interrupt 4;
void UART1_int(void) __interrupt 20;

//------------------------------------------------------
// Main Function 
//------------------------------------------------------

void main(void){
	// Declare local variables 
	char message;
	unsigned long i;
	char j;

    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	UART1_INIT();                       // Initialize UART1
	INTERRUPT_INIT();

    SFRPAGE = UART0_PAGE;				// Direct output to UART1

	printf("\033[2J");                  // Erase screen & move cursor to home position
    printf("Test of the printf() function.\n\n\r");
    
    while(1)
    {
		SFRPAGE = UART0_PAGE;
		if(exit_flag){
			printf("\n\n\rStopping now...");
			SFRPAGE = UART1_PAGE;
			printf("\n\n\rStopping now...");
			for(i = 0;i<20000;i++);
			break;
		}
		
		for(j=0;j<255;j++){
			ES0 = 0;
		}
		ES0 = 1;
		
		if(UART0_flag){
			echo(choice);
			UART0_flag = 0;
		}
		if(UART1_flag){
			ES0 = 0;
			SBUF0 = choice;
			ES0 = 1;
			UART1_flag = 0;
		}
		
	}
	return;
}

void echo(char character){
	if(character == 27){
		exit_flag = 1;
		return;
	}
	ES0 = 0;
	SFRPAGE = UART0_PAGE;             
	SBUF0 = character;
	
	SFRPAGE = UART1_PAGE;
	SBUF1 = character;
}

//-------------------------------------
// Interrupts 
//-------------------------------------
void UART0_int(void) __interrupt 4{
	CURR_PAGE = SFRPAGE;
	SFRPAGE = UART0_PAGE;
	if(RI0){
		RI0 = 0;
		choice = SBUF0;
		UART0_flag = 1;
	}
	SFRPAGE = CURR_PAGE;
}

void UART1_int(void) __interrupt 20{
	CURR_PAGE = SFRPAGE;
	SFRPAGE = UART1_PAGE;
	if(RI1){
		RI1 = 0;
		choice = SBUF1;
		UART1_flag = 1;
	}
	ES0 = 1;
	SFRPAGE = CURR_PAGE;
}


//------------------------------------------------------
// PORT_Init
//------------------------------------------------------
// Configure the Crossbar and GPIO ports

void PORT_INIT(void){
	char SFRPAGE_SAVE;

	SFRPAGE_SAVE = SFRPAGE;     // Save Current SFR page.

	SFRPAGE = CONFIG_PAGE;
	XBR0    = 0x04;             // Enable UART0.
	XBR1    = 0x00;             // 
	XBR2    = 0x44;             // Enable Crossbar and weak pull-ups and UART1.
	P0MDOUT = 0x05;             // P0.0 (TX0) and P0.2 (TX1) are configured as Push-Pull for output, P0.1 (RX0) and P0.3 (RX1) are Open-drain
	P0      = 0x0A;             // Additionally, set P0.0=0, P0.1=1, P0.2=0, P0.3=1
	
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

void INTERRUPT_INIT(void){
	EA  = 1;                // Enable interrupts as selected.
	ES0 = 1;
	EIE2 |= 0x40;
}

// Configure the UART0 using Timer1, for 9600 and 8-N-1.
void UART0_INIT(void)
{
	char SFRPAGE_SAVE;
	
	SFRPAGE_SAVE = SFRPAGE;     // Save Current SFR page.
	
	SFRPAGE = TMR2_PAGE;
    TMR2CN  = 0x00;             // Auto-reload mode, use clock defined in TMR2CF,  
	TMR2CF  = 0x08;				// Timer 2 uses SYSCLK as time base

	RCAP2H  = 0xFD;             // Set timer 2 auto-reload value for 4800bps
	RCAP2L  = 0x78;

	TR2 = 1;					// Start timer 2
	
	SFRPAGE = UART0_PAGE;
	SCON0   = 0x50;             // Set Mode 1: 8-Bit UART
	SSTA0   = 0x15;             // UART0 baud rate divide-by-two disabled (SMOD0 = 1) and use TMR2.
	TI0     = 1;                // Indicate TX0 ready.
	
	SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page
}


// Configure the UART1 using Timer1, for 115200 and 8-N-1.
void UART1_INIT(void)
{
    char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;             // Save Current SFR page

    SFRPAGE = TIMER01_PAGE;
    TMOD   &= ~0xF0;
    TMOD   |=  0x20;                    // Timer1, Mode 2, 8-bit reload
    TH1 	= 40;						// Set Timer1 reload baudrate value T1 Hi Byte
	CKCON  |= 0x10;                     // Timer1 uses SYSCLK as time base
    TL1     = TH1;
    TR1     = 1;                        // Start Timer1
	
	SFRPAGE = UART1_PAGE;
    SCON1   = 0x30;                     // Mode 1, 8-bit UART, enable RX
    TI1     = 1;                        // Indicate TX1 ready
	
	SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}