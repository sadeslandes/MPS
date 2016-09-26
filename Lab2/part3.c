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
//char reset_flag;

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
//void resetPress_ISR (void) __interrupt 2;

//------------------------------------------------------
// Main Function 
//------------------------------------------------------

void main(void){
	// Declare local variables 
	char choice;
	unsigned int rand_;
	unsigned char tenths = 0;
	float reactions = 0;
	unsigned char trials = 0;
	SFRPAGE = CONFIG_PAGE;
	
	PORT_INIT();
	TIMER0_INIT();
	SYSCLK_INIT();
	UART0_INIT();

	SFRPAGE = LEGACY_PAGE;
	IT0 = 1; 
	
	// Display the set up information
	printf("\033[2J");
	printf("MPS Reaction Game \n\n\r");
	printf("Ground /INT0 on P0.2 to generate an interrupt. \n\n\r");

	SFRPAGE = CONFIG_PAGE;
	EX0 = 1; 

	SFRPAGE = UART0_PAGE;
	// Seed the random number generator
	srand(78);
	while(1){
		//Generate random number 
		rand_ = rand()%10;
		TR0 = 1; 								//Start Timer0
		// Wait for the random delay to elapse
		while(timer0_flag/2 != rand_){

		}
		// Tell user to press the button and start keeping track of reaction time
		printf("PRESS NOW\n\n\r"); 
		timer0_flag = 0;
		// Wait for the user to press the reaction button
		while(!react_flag){

		}
		// Determine how long their response took in tenths of a second (truncates)
		tenths = timer0_flag/2;
		// Increment the number of trials and add the reaction time to the running total
		trials += 1;
		reactions += tenths*.1;
		// If they respond in under .2s the output text is green
		if(tenths < 2){
			printf("\033[1;32m");
		}
		// If they respond in under .5s the output text is yellow
		else if(tenths < 5){
			printf("\033[1;33m");
		}
		// Otherwise, the output text is red
		else{ 
			printf("\033[1;31m");
		}
		// Display the user's response time and average response time 
		printf_fast_f("Your response time was: %.2f seconds\n\r", tenths*0.1);
		printf_fast_f("Your average response time is: %.2f\n\n\r", reactions/trials);
		printf("\033[1;37m");
		
		// Provide the user with the option to reset the program every 5 trials
		if(trials % 5 == 0){
			printf("Do you want to continue? Press Y or N\n\n\r");
			while(1){
				choice = getchar();
				if (choice == 'n'){
					return;
				} else if(choice == 'y'){
					break;
				}	
			}
		}
		// Reset the variables 
		TR0 = 0;
		TH0 = 0x35;
		TL0 = 0x80;
		timer0_flag = 0;
		react_flag = 0;
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

// Set the flag for the reaction button if it is pressed. Uses software debouncing
void reactPress_ISR (void) __interrupt 0{
	if(timer0_flag > 0){
		react_flag = 1;
	}	 
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