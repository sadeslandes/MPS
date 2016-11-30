//------------------------------------------------------------------------------------
// Lab6-3
//------------------------------------------------------------------------------------
// Nick Choi, Samuel Deslandes
// This program expands on that of 6-2 to get user input from a keypad, rather than the keyboard.
// Output is still printed to the LCD screen.
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
#define EXTCLK      11059200            // External oscillator frequency in Hz
#define SYSCLK      11059200            // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    115200              // UART baud rate in bps
char asciichar;
char portvalue;
char keyvalue;
int i;
char keyflag = 0;
//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void TIMER0_INIT(void);
void KeypadVector(void) __interrupt 0;
char getkeychar(void);
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
	
	EX0 = 1;
	while(1)
    {
		printf("Select an option:\n\rA: Yes/No\n\rB: True/False\n\rC: Day of the week\n\rD: Number within range\n\n\r");
		
		choice = getkeychar();
		// Switch/Case block to handle each question category
		// TL0 is the value of Timer0, which is used to generate 
		// pseudorandom numbers
		switch(choice){
			// Yes/No
			case 'A':
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
			case 'B':
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
			case 'C':
				lcd_clear();
				choice = TL0%7;						// get random index for array containing days of the week
				printf("%s\n\r",days[choice]);
				lcd_puts(days[choice]);
				break;
			// Random number
			case 'D': 
				printf("Enter min value followed by max value: ");

				low = getkeychar();
				printf("%c, ",low);
				high = getkeychar();
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

// External Interrupt 0 ISR
void KeypadVector(void) __interrupt 0{
	EX0 = 0;						// Disable /INT0
	keyflag = 1;
	keyvalue = P3 & 0x0F;	
	// Try first row
	P3=0x8F; 						// check if row one (top) was active
	for(i = 0; i<400; i++); 		// wait for the output and input pins to stabilize
	portvalue = P3 & 0x0F; 		 	// read the value of the lower 4 bits
	if (portvalue == 0x0F) 			// if this row was selected then the value will be 0x0F
	{
		if (keyvalue == 0x07){ 		// look at the value of the low 4 bits
			asciichar = '1';		// return the value of the matching key
		}
		else if (keyvalue == 0x0B){
			asciichar = '2';
		}
		else if (keyvalue == 0x0D){
			asciichar = '3';
		}
		else{
			asciichar = 'A';
		}
		
		P3 = 0x0F;					// put output lines back to 0
		while (P3 != 0x0F); 		// wait while the key is still pressed
		for(i = 0; i<20000; i++);	// wait for output and input pins to stabilize after key is released
		EX0 = 1;
		return;
	} 
	// Try second row
	P3=0x4F; 						// check if row one (top) was active
	for(i = 0; i<400; i++); 		// wait for the output and input pins to stabilize
	portvalue = P3 & 0x0F; 			// read the value of the lower 4 bits
	if (portvalue == 0x0F) 			// if this row was selected then the value will be 0x0F
	{
		if (keyvalue == 0x07){ 		// look at the value of the low 4 bits
			asciichar = '4';		// return the value of the matching key
		}
		else if (keyvalue == 0x0B){
			asciichar = '5';
		}
		else if (keyvalue == 0x0D){
			asciichar = '6';
		}
		else{
			asciichar = 'B';
		}

		P3 = 0x0F;					// put output lines back to 0
		while (P3 != 0x0F); 		// wait while the key is still pressed
		for(i = 0; i<20000; i++);	// wait for output and input pins to stabilize after key is released
		EX0 = 1;
		return;
	}
	// Try third row
	P3=0x2F; 						// check if row one (top) was active
	for(i = 0; i<400; i++); 		// wait for the output and input pins to stabilize
	portvalue = P3 & 0x0F; 			// read the value of the lower 4 bits
	if (portvalue == 0x0F) 			// if this row was selected then the value will be 0x0F
	{
		if (keyvalue == 0x07){ 		// look at the value of the low 4 bits
			asciichar = '7';		// return the value of the matching key
		}
		else if (keyvalue == 0x0B){
			asciichar = '8';
		}
		else if (keyvalue == 0x0D){
			asciichar = '9';
		}
		else{
			asciichar = 'C';
		}
		
		P3 = 0x0F;					// put output lines back to 0
		while (P3 != 0x0F); 		// wait while the key is still pressed
		for(i = 0; i<20000; i++);	// wait for output and input pins to stabilize after key is released
		EX0 = 1;
		return;
	}
	// Try last row
	P3=0x1F; 						// check if row one (top) was active
	for(i = 0; i<400; i++); 		// wait for the output and input pins to stabilize
	portvalue = P3 & 0x0F; 			// read the value of the lower 4 bits
	if (portvalue == 0x0F) 			// if this row was selected then the value will be 0x0F
	{
		if (keyvalue == 0x07){ 		// look at the value of the low 4 bits
			asciichar = '*';		// return the value of the matching key
		}
		else if (keyvalue == 0x0B){
			asciichar = '0';
		}
		else if (keyvalue == 0x0D){
			asciichar = '#';
		}
		else{
			asciichar = 'D';
		}
		
		P3 = 0x0F;					// put output lines back to 0
		while (P3 != 0x0F); 		// wait while the key is still pressed
		for(i = 0; i<20000; i++);	// wait for output and input pins to stabilize after key is released
		EX0 = 1;
		return;
	}	
}

// Function to wait for and return keypad input
char getkeychar(){
	while(!keyflag);
	keyflag = 0;
	return asciichar;
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
    OSCXCN  = 0x77;                     // Start ext osc with 11.0592MHz crystal /edit 0x67
    for(i=0; i < 256; i++);             // Wait for the oscillator to start up
    while(!(OSCXCN & 0x80));
    CLKSEL  = 0x01;
    OSCICN  = 0x00;

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
    XBR1     = 0x04;					// INT0 routed to port pin P0.2
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up

	EA 		 = 1;						// Enable global interrupts

    P0MDOUT |= 0x01;                    // Set TX0 on P0.0 pin to push-pull
    P3MDOUT  = 0xF0;                    // Set P3 high nibble as output, low nibble as input
	P3 		 = 0x0F;					// P3 high nibble set to 0v
	
	SFRPAGE = TIMER01_PAGE;
	TCON 	&= 0xFC;					// Clear INT0 flag and set for level triggered

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
    TH1     = 0xFA;						// Set Timer1 reload baudrate value T1 Hi Byte
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