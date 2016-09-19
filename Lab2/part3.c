//---------------------------------------------------
// Includes
//---------------------------------------------------
#include <c8051f120.h>  
#include <stdio.h>
#include <stdlib.h>
//#include <time.h>
#include "putget.h"     
        
//---------------------------------------------------------
// Global CONSTANTS
//---------------------------------------------------------

#define EXTCLK      22118400    // External oscillator frequency in Hz
#define SYSCLK      49766400    // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200      // UART baud rate in bps
//#define BAUDRATE  19200       // UART baud rate in bps
char timer0_flag = 0;
__bit reactPress = 0;
__bit resetPress = 0;
char react_flag;
char reset_flag;

//-------------------------------------------------------
// Function PROTOTYPES
//-------------------------------------------------------
void main(void);
void PORT_INIT(void);
void SYSCLK_INIT(void);
void UART0_INIT(void);
void TIMER0_INIT(void);
void TIMER0_ISR(void) __interrupt 1;
void reactPress_ISR (void) __interrupt 0;
void resetPress_ISR (void) __interrupt 2;

//------------------------------------------------------
// Main Function 
//------------------------------------------------------

void main(void){
	unsigned int rand_;
	unsigned char tenths = 0;
	unsigned int reactions = 0;
	unsigned char trials = 0;
	SFRPAGE = CONFIG_PAGE;

	PORT_INIT();
	TIMER0_INIT();
	SYSCLK_INIT();
	UART0_INIT();

	SFRPAGE = LEGACY_PAGE;
	IT0 = 1; 
	IT1 = 1;

	printf("\033[2J");
	printf("MPS Reaction Game \n\n\r");
	printf("Ground /INT0 on P0.2 to generate an interrupt. \n\n\r");

	SFRPAGE = CONFIG_PAGE;
	EX0 = 1; 

	SFRPAGE = UART0_PAGE;
	
	reactPress = 0;
	srand(78);
	while(1){
		//generate random number
		rand_ = rand()%10;
		printf("%i\n\r", rand_);
		TR0 = 1; //Start Timer0
		while(timer0_flag/2 != rand_){

		}
		printf("PRESS NOW"); 
		timer0_flag = 0;
		while(!react_flag){

		}
		tenths = timer0_flag/2;
		trials += 1;
		reactions += 1;
		printf("Your response time was: %u tenths\n\n\r", tenths);
		printf("Your average response time is: %f\n\n\r", (float)reactions/trials);
		
		
		
		TR0 = 0;
		TH0 = 0;
		TL0 = 0;
		timer0_flag = 0;
		react_flag = 0;
		if(reset_flag){
			reset_flag = 0;
			return;
		}
	}
}
//-------------------------------------
// Interrupts 
//-------------------------------------

void TIMER0_ISR(void) __interrupt 1{
	TH0 = 0x35;
	TL0 = 0x80;
	timer0_flag += 1;
}

void reactPress_ISR (void) __interrupt 0{
	react_flag = 1;	 
}

void resetPress_ISR (void) __interrupt 0{
	reset_flag = 1;	 
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
	XBR1    = 0x14;             // /INT0 and /INT1 routed to port pins P0.2 and P0.3 respectively.
	XBR2    = 0x40;             // Enable Crossbar and weak pull-ups.
	P0MDOUT = 0x01;             // P0.0 (TX0) is configured as Push-Pull for output
	// P0.1 (RX0) is configure as Open-Drain input.
	// P0.2 (recatPress button through jumper wire) is configured as Open_Drain for input.
	// P0.3 (recatPress button through jumper wire) is configured as Open_Drain for input.
	P0      = 0x0E;             // Additionally, set P0.0=0, P0.1=1, P0.2=1, and P0.3=1
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

// Timer init
void TIMER0_INIT(void){
	char SFRPAGE_SAVE;
	SFRPAGE_SAVE = SFRPAGE;

	SFRPAGE = TIMER01_PAGE;	

	TMOD &= 0xF0;
	TMOD |= 0x01;
	TH0 = 0x35;
	CKCON &= ~0x09;
	CKCON |= 0x02;
	TL0 = 0x80;

	SFRPAGE = CONFIG_PAGE;
	ET0 = 1;

	SFRPAGE = SFRPAGE_SAVE;
}