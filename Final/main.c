//------------------------------------------------------------------------------------
// Final
//------------------------------------------------------------------------------------
// Nick Choi, Samuel Deslandes
// This program is an extension of that of Lab6-1 in which program output is printed
// to an LCD screen.
//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f120.h>
#include <stdio.h>
#include <stdlib.h>
#include "putget.h"
#include "LCD.h"
#include "LCD.c"
//------------------------------------------------------------------------------------
// Global Constants
//------------------------------------------------------------------------------------
#define EXTCLK      22118400            // External oscillator frequency in Hz
#define SYSCLK      49766400            // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200              // UART baud rate in bps
int ADCx;
int ADCy;
	
//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void TIMER0_INIT(void);
void ADC0_INIT(void);
void get_XY(void);
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
    unsigned int i;
	WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	TIMER0_INIT();						// Initialize TIMER0
	ADC0_INIT();
	lcd_init();							// Initialize LCD

    SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
	printf("Exit command is: <ESC>  \n\n\r");
	
	while(1)
    {
		get_XY();
		if(ADCx > 3200){//move left
			printf("\033[1D");
		}
		if(ADCx < 500 ){//move right
			printf("\033[1C");
		}
		if(ADCy < 150){//move up
			printf("\033[1A");
		}
		if(ADCy > 3200){//move down
			printf("\033[1B");
		}
		
		for(i=0;i<65535;i++);
    }
	
}

void get_XY(){
	char i;
	AMX0SL = 0x00;
	for(i=0;i<255;i++);
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
	ADCy = ADC0; //ADC0H;
	
	
	AMX0SL = 0x01;
	for(i=0;i<255;i++);
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
	ADCx = ADC0; //ADC0H;
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
    char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;             // Save Current SFR page

    SFRPAGE  = CONFIG_PAGE;
    XBR0     = 0x04;                    // Enable UART0
    XBR1     = 0x00;
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up
    P0MDOUT |= 0x01;                    // Set TX0 on P0.0 pin to push-pull
    P1MDOUT |= 0x40;                    // Set green LED output P1.6 to push-pull

    SFRPAGE  = SFRPAGE_SAVE;            // Restore SFR page
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

	SFRPAGE = SFRPAGE_SAVE;
}

void ADC0_INIT(void){
	char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;
	SFRPAGE = ADC0_PAGE;

	REF0CN |= 0x03;			            // turn on internal ref buffer and bias generator, vref0 is ref voltage
	ADC0CF = 0xBF;						// AD0SC = 23 for SARclk of 1Mhz, Gain = 1 		
	AD0EN = 1;  						// enable ADC0
	
	AMX0CF = 0x00;

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}