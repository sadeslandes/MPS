// Nick Choi and Sam Deslandes
// Code for section 1 of Lab 4.
// This program acts as a digital voltmeter using the ADC, displaying the voltage in decimal and hex to the terminal.  
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
char butpress = 0;

//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void ADC0_INIT(void);
void display(int* start, char div);
void BUTPRESS_ISR(void) __interrupt 0;

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
    int ADCdata;
	int dataArray[16] = {0};
	char num_entries = 0;
	char size = 0;
	int i;

    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	ADC0_INIT();

	SFRPAGE = LEGACY_PAGE;
	IT0 = 1;  							// /INT0 triggered on negative falling edge
    
	SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
    printf("Test of the printf() function.\n\n\r");
    
	while(1)
    {
        if(butpress){
			butpress = 0;
			//printf("button pressed\n\r");			
			
			AD0INT = 0; 				// Clear conversion interrupt flag
			AD0BUSY = 1;   				// Start conversion
			while(!AD0INT);				// Wait for conversion to end

			ADCdata = ADC0; 			// Get result of conversion (using 16bit register)

			printf_fast_f("Result: %.6f\t0x%x\n\r",(ADCdata/4096.)*2.43,ADCdata);
			
			dataArray[num_entries%16] = ADCdata; 
			num_entries += 1;

			// algorithm for # of relevant entries in array (for averaging)
			size = num_entries;
			if(num_entries > 15){
				size = 16;
			}

			display(dataArray,size);
			
		}
    }
}

void display(int* start, char div){
	float sum=0;
	char i;
	int min = 4095;
	int max = 0;

	for(i = 0; i<div; i++){
		// Get min value in array
		if(*(start+i) < min){
			min = *(start+i);
		}
		// Get max value in array
		if(*(start+i) > max){
			max = *(start+i);
		}
		// Sum entries in array
		sum += *(start+i);
	}
	
	printf_fast_f("Min is %.6f\t0x%x\n\r",(min/4096.)*2.43,min);
	printf_fast_f("Max is %.6f\t0x%x\n\r",(max/4096.)*2.43,max);
	printf_fast_f("Average is %.6f\t0x%x\n\n\r",((sum/div)/4096.)*2.43,  sum/div);
}

void BUTPRESS_ISR(void) __interrupt 0{
	butpress = 1;
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
	P0       = 0x06;             		// Additionally, set P0.0=0, P0.1=1, and P0.2=1.
	
	EA 		 = 1;						// Enable global interrupts
	EX0 	 = 1;						// Enable external interrupts
	
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

//------------------------------------------------------------------------------------
// ADC0_Init
//------------------------------------------------------------------------------------
void ADC0_INIT(void){
	char SFRPAGE_SAVE;

    SFRPAGE_SAVE = SFRPAGE;
	SFRPAGE = ADC0_PAGE;

	REF0CN |= 0x03;			            // turn on internal ref buffer and bias generator, vref0 is ref voltage
	ADC0CF = 0xB8;						// AD0SC = 23 for SARclk of 1Mhz, Gain = 1 		
	AD0EN = 1;  						// enable ADC0

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}