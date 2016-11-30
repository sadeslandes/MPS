//------------------------------------------------------------------------------------
// Lab6-2
//------------------------------------------------------------------------------------
// Nick Choi, Samuel Deslandes
// This program is an extension of that of Lab6-1 in which program output is printed
// to an LCD screen.
//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f120.h>
#include <stdio.h>
#include "putget.h"
#include "LCD.h"
#include "LCD.c"
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
void TIMER0_INIT(void);

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
    char low;
	char high;
	char choice;
	const char* days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	char out[2];

    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	TIMER0_INIT();						// Initialize TIMER0
	lcd_init();							// Initialize LCD

    SFRPAGE = UART0_PAGE;               // Direct output to UART0

    printf("\033[2J");                  // Erase screen & move cursor to home position
	printf("Exit command is: <ESC>  \n\n\r");
	
	while(1)
    {
		printf("Select an option:\n\r1. Yes/No\n\r2. True/False\n\r3. Day of the week\n\r4. Number within range\n\n\r");
		choice = getchar();
		// Switch/Case block to handle each question category
		// TL0 is the value of Timer0, which is used to generate 
		// pseudorandom numbers
		switch(choice){
			// Yes/No
			case '1':
				lcd_clear();
				if(TL0%2 == 1){
					printf("Yes.\n\r");
					lcd_puts((char *) &"Yes");
				} else {
					printf("No.\n\r");
					lcd_puts((char *) &"No");
				}
				break;
			// True/False
			case '2':
				lcd_clear();
				if(TL0%2 == 1){
						printf("True.\n\r");
						lcd_puts((char *) &"True");
					} else {
						printf("False. \n\r");
						lcd_puts((char *) &"False");
					}
				break;
			// Day of the week
			case '3':
				lcd_clear();
				choice = TL0%7;						// get random index for array containing days of the week
				printf("%s\n\r",days[choice]);
				lcd_puts(days[choice]);				
				break;
			// Random number
			case '4': 
				printf("Enter min value followed by max value: ");
				low = getchar();
				printf("%c, ",low);
				high = getchar();
				putchar(high);
				
				//Array to store output. Last element is null char which signifies end of string.
				out[0] = low+TL0%(high-low+1);		// Get random number within range (inclusive)
				out[1] = '\0';
				
				printf("\n\r%c\n\r", out[0]);
				lcd_clear();
				lcd_puts(out);
				break;
			// Invalid input case
			default:
				lcd_clear();
				printf("Invalid input\n\r");
				lcd_puts((char*)&"Invalid input");
		}
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