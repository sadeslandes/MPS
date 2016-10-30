// Nick Choi and Sam Deslandes
// Code for section 4 of Lab 4.
// This program utilizes the MAC to filter a signal received through the ADC, then sends the filtered signal out through the DAC.
// The digital filter acts as a notch filter, in whihch at a certain frequecy the frequency response goes to zero.
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
#define OFFSET 		1502                // Offset value = (Offset voltage/Vref)*4069

//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void ADC0_INIT(void);
void DAC0_INIT(void);
void ADC_read(void);


//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
	char i;
	signed int curr_ADC = 0; 
	signed int	past_ADC = 0;
	signed int pastest_ADC = 0;
	signed int past_DAC = 0;
	signed int result;	

    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	ADC0_INIT();
	DAC0_INIT();	
    
	SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
    printf("Test of the printf() function.\n\n\r");
    
	while(1)
    {
		SFRPAGE = ADC0_PAGE;
		ADC_read();	        			// start new conversion
		

		pastest_ADC = past_ADC; 		// update previous inputs
		past_ADC = curr_ADC;
		curr_ADC = ADC0;				// get new sample
		
		SFRPAGE = MAC0_PAGE;
		MAC0CF = 0x08;   				// clear accumulator, integer mode, multiply and accumulate

		curr_ADC -= OFFSET; 				// subtract 1.5V offset


		MAC0A = 320;					// .3125*2^10 (scaled integer)
		
		MAC0BH = curr_ADC>>8;			// load MAC0B with current sample (k)
		MAC0BL = curr_ADC;
		
		MAC0BH = pastest_ADC>>8;		// load MAC0B with 2nd previous sample (k-2)
		MAC0BL = pastest_ADC;
		
		MAC0A = 246;                    // .24038462*2^10 (scaled integer) 
		
		MAC0BH = past_ADC>>8;			// load MAC0B with previous sample (k-1)
		MAC0BL = past_ADC;
		
		MAC0A = 304;					// load MAC0A with .296875*2^10 (scaled integer)
		
		MAC0BH = past_DAC>>8;			// load MAC0B with previous output (k-1)
		MAC0BL = past_DAC;
		
		SFRPAGE = MAC0_PAGE;
		for(i=0;i<10;i++){				// shift right 10 times
			MAC0CF = 0x30;
		}
		
		SFRPAGE = MAC0_PAGE;			// dummy op, then get results
		result = MAC0ACC1<<8 | MAC0ACC0;

		result += OFFSET;                 // add back in 1.5V offset
		
		SFRPAGE = DAC0_PAGE;
		DAC0 = result;					// output result to DAC

		past_DAC = DAC0;				// update previous output
		
	}
}

void ADC_read(void){
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
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
    XBR1     = 0x04;					// /INT0 routed to port pin
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up
    P0MDOUT |= 0x01;                    // Set TX0 on P0.0 pin to push-pull
	P0       = 0x02;             		// Additionally, set P0.0=0, P0.1=1
	
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


void ADC0_INIT(void){
	char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;
	SFRPAGE = ADC0_PAGE;

	REF0CN |= 0x02;			            // turn on internal ref buffer and bias generator, vref0 is ref voltage
	ADC0CF = 0xB8;						// AD0SC = 23 for SARclk of 1Mhz, Gain = 1 		
	AD0EN = 1;  						// enable ADC0

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}

void DAC0_INIT(void){
	char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;
	SFRPAGE = DAC0_PAGE;

	REF0CN |= 0x02;			            // turn on bias generator, but not internal reference buffer. vref comes from external source
	DAC0CN = 0x80;						// enable DAC0

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}
