//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <c8051f120.h>
#include <stdio.h>
#include "putget.h"
#include <LCD.c>
#include <stdlib.h>
//------------------------------------------------------------------------------------
// Global Constants
//------------------------------------------------------------------------------------
#define EXTCLK      22118400            // External oscillator frequency in Hz
#define SYSCLK      49766400            // Output of PLL derived from (EXTCLK * 9/4)
#define BAUDRATE    28800              // UART baud rate in bps

//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
char keypadDecoder();
void KeypadISR(void) __interrupt 0;
void Menu(void);
void generateEasy();
void generateMedium();
void generateHard();
void generateGG();
char keyValue;
char keypadPressed;
char decoded;
char userInput;
void generateBox();
char Box;
int i;
int k;
int j;
int row;
int col;
int rows[12];
int cols[12];
void checkBox();
char blank;
//char timer[3];
int xinput;
int yinput;
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;
	keypadPressed = 0;
	decoded = 1;
	Box = 219;
	blank = 255;
	lcd_init();
    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0

    SFRPAGE = UART0_PAGE;               // Direct output to UART0
	
	printf("\033[33m");

    printf("\033[2J");                  // Erase screen & move cursor to home position

	printf("\033[29C");

    //printf("Test of the printf() function.\n\n");

    printf("Press <ESC> to exit \n\r");

	printf("\033[2;35H");

	printf("GAME TITLE \n\r");

	printf("\033[3;0H");

	//printf("\033[4B");
	
	//printf("\033[12;24r");
	
	Menu();
	lcd_cmd(0x3F);		// set display to 2 lines 5x8
						// (0x30=1 line 5x8, 0x34=1 line 5x10)
	lcd_cmd(0x0C);		// turn on display and cursor
	lcd_cmd(0x01);		// clear display
	
	lcd_goto(0);		// set cursor address to 0
    while(1)
    {
		TR0 = 1;
		//wait for key to be pressed and then continue with program
		while(!keypadPressed);
		keypadPressed = 0;
		userInput = keypadDecoder();
		printf("userInput: %c\n\r", userInput);
		if (userInput == 'A')
			generateEasy();
		else if (userInput == 'B')
			generateMedium();
		else if (userInput == 'C')
			generateHard();
		else if (userInput == 'D')
			generateGG();

		keyValue = 'A';

    }
}

void KeypadISR(void) __interrupt 0
{
	int i;
	if(decoded)
	{
		for(i = 0; i < 10000; i++);
		keyValue = P3 & 0x0F;
		TCON &= 0xFD;
		keypadPressed = 1;
		decoded = 0;
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
    char SFRPAGE_SAVE = SFRPAGE;	// Save Current SFR page.
	SFRPAGE = CONFIG_PAGE;
	
	OSCXCN = 0x67;			// Start external oscillator
	for(i=0; i < 256; i++);		// Wait for the oscillator to start up.
	while(!(OSCXCN & 0x80));// Check to see if the Crystal Oscillator Valid Flag is set.
	CLKSEL = 0x01;			// SYSCLK derived from the External Osc cct.
	OSCICN = 0x00;			// Disable the internal oscillator.

	SFRPAGE = CONFIG_PAGE;
	PLL0CN = 0x04;			//set to external oscillator
	SFRPAGE = LEGACY_PAGE;
	FLSCL = 0x10;			//sysclock <= 50mhz
	SFRPAGE = CONFIG_PAGE;
	PLL0CN |= 0x01;			//PLL bias generator is active. Must be set for PLL to operate
	PLL0DIV = 0x04;			//SYSCLK/4
	PLL0FLT = 0x01;			//divided pll reference clock 19-30 MHz
	PLL0MUL = 0x09;			//multiplication factor equals to 9
	for(i=0; i < 256; i++);
	PLL0CN |= 0x02;			//PLL is enabled
	while(!(PLL0CN & 0x10));
	CLKSEL = 0x02;			// SYSCLK derived from the PLL.

	SFRPAGE = SFRPAGE_SAVE;	// Restore SFR page.
}

//------------------------------------------------------------------------------------
// PORT_Init
//------------------------------------------------------------------------------------
//
// Configure the Crossbar and GPIO ports
//
void PORT_INIT(void)
{    
	char SFRPAGE_SAVE = SFRPAGE;    // Save Current SFR page
    SFRPAGE = CONFIG_PAGE;
	
	XBR0=0x04;
	XBR1=0x04;
	XBR2=0x40;						// Enable Crossbar with Weak Pullups

	P7MDOUT=0x07;					// Set E, RW, RS controls to push-pull
	P6MDOUT=0x00;					// P6 must be open-drain to be bidirectional:

    P0MDOUT |= 0x01;    // Set TX0 pin to push-pull
	P0 = 0x02;

	P3MDOUT = 0xF0;
	P3 = 0x0F;

	IE = 0x81;

    SFRPAGE = SFRPAGE_SAVE;     // Restore SFR page
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

//-------------------------------------------------------------------------------------------
// Timer0_Init
//-------------------------------------------------------------------------------------------
//

void Timer0_Init(void)
{
    char SFRPAGE_SAVE = SFRPAGE;

	SFRPAGE = TIMER01_PAGE;


	//INTERRUPT
	IE 		|= 0x02;	// enable Timer0 Interrupt request 	
	ET0		= 1; 	
	
	//TIMER0 Settings 
	CKCON 	&= 0xF4; 	// Timer0 uses SYSCLK/12 as source
    TMOD 	&= 0xF0; 	// clear the 4 least significant bits
						//clears anything leftover in Timer0 from previous iterations
	TMOD 	|= 0x01; 	// Timer0 in mode 1: 16-bit
    TR0 	= 0; 		// Stop Timer0
	TF0 	= 0;
    TH0 	= 0x5E; 		// Clear low byte of register T0
    TL0 	= 0x00;  		// Clear high byte of register T0
	SFRPAGE = SFRPAGE_SAVE;
} 


/*void Timer0_ISR(void)__interrupt 1
{
	char SFRPAGE_SAVE = SFRPAGE;
	SFRPAGE = TIMER01_PAGE;
    TH0 	= 0x5E; 		// Clear low byte of register T0
    TL0 	= 0x00;  		// Clear high byte of register T0
	SFRPAGE = SFRPAGE_SAVE;
//	overflows++; 
	
	//Preset Timer 0s counter
	//Set in 16-bit mode: 65536 ticks per overflow
	//preload with 0x5E00 which is 41472 ticks so that
	//each interrupt happens every 24064 ticks, or every .01 seconds
} */


void Menu(void)
{
	printf("Select difficulty level\n\r");
	printf("A.) Easy\n\r");
	printf("B.) Medium\n\r");
	printf("C.) Hard\n\r");
	printf("D.) GG\n\r");
}

char keypadDecoder()
{
	char portValue;
	char returnVal = 'CHECKFAILED';
	unsigned int i;
	IE = 0x00;
	//printf("keyValue: 0x%x\n\r", keyValue);
	TCON &= 0xFD;

	//check row 1
	P3 = 0x8F;
	for(i = 0; i < 15000; i++);

	portValue = P3 & 0x0F;
	if (portValue == 0x0F && returnVal=='CHECKFAILED')
	{
		if(keyValue == 0x07)
			returnVal = '1';

		else if(keyValue == 0x0B)
			returnVal = '2';

		else if(keyValue == 0x0D)
			returnVal =  '3';

		else if(keyValue == 0x0E)
			returnVal = 'A';
	}
	
	//check row 2
	P3 = 0x4F;
	for(i = 0; i < 15000; i++);

	portValue = P3 & 0x0F;
	if (portValue == 0x0F && returnVal=='CHECKFAILED')
	{
		if(keyValue == 0x07)
			returnVal = '4';

		else if(keyValue == 0x0B)
			returnVal = '5';

		else if(keyValue == 0x0D)
			returnVal = '6';

		else if(keyValue == 0x0E)
			returnVal = 'B';
	}

	//check row 3
	P3 = 0x2F;
	for(i = 0; i < 15000; i++);

	portValue = P3 & 0x0F;
	if (portValue == 0x0F && returnVal=='CHECKFAILED')
	{
		if(keyValue == 0x07)
			returnVal = '7';

		else if(keyValue == 0x0B)
			returnVal = '8';

		else if(keyValue == 0x0D)
			returnVal = '9';

		else if(keyValue == 0x0E)
			returnVal = 'C';
	}

	//check row 4
	P3 = 0x1F;
	for(i = 0; i < 15000; i++);

	portValue = P3 & 0x0F;
	if (portValue == 0x0F && returnVal=='CHECKFAILED')
	{
		if(keyValue == 0x07)
			returnVal = '*';

		else if(keyValue == 0x0B)
			returnVal = '0';

		else if(keyValue == 0x0D)
			returnVal = '#';

		else if(keyValue == 0x0E)
			returnVal = 'D';
	}
	
	P3 &= 0x0F;
	while (P3 != 0x0F);
	for(i = 0; i < 15000; i++);
	IE = 0x81;
	decoded = 1;
	return returnVal;
}

void generateEasy()
{
	printf("\033[2J");
	i = 5;
	generateBox();
	checkBox();	
}

void generateMedium()
{	
	printf("\033[2J");
	i = 7;
	generateBox();
}

void generateHard()
{	
	printf("\033[2J");
	i = 9;
	generateBox();
}

void generateGG()
{	
	printf("\033[2J");
	i = 11;
	generateBox();
}

void generateBox()
{	
	for(j=0;j<i;j++)
	{
		row = rand()%24;
		col = rand()%70;
		rows[j] = row;
		cols[j] = col;
		//printf("row:%i   col:%i \r\n", rows[j],cols[j]);
		printf("\033[%i;%iH",row,col);
		printf("%c", Box);
		lcd_cmd(0x01);	
		lcd_goto(0);		
		lcd_puts("True");
	}
}

/*void LCDtime()
{
	if(overflows == 10) 
		{
			overflows = 0;			
			secondCounter++; //incremented every time 0.1 seconds has elapsed 
			if(secondCounter < 10)
			{
				lcd_cmd(0x01);	
				lcd_goto(0);		
				lcd_puts("Seconds passed: .");
			}
			else if(secondCounter >= 10)
			{	
				//Once secondCounter has been incremented 10 times
				//1 second has passed
				// EXAMPLE: 10.0/10.0 = 1 second 
				time2 = secondCounter/10; //holds the number of seconds
				lcd_cmd(0x01);	
				lcd_goto(0);
				//lcd_puts("Seconds passed: %d", time2);
				//lcd_puts(".%d \r\n", secondCounter%10);
			}
 		}
}*/

void checkBox()
{
	for(k=0;k<j;k++)
	{
		xinput = rows[k];
		yinput = cols[k];
		if((xinput == rows[k])&&(yinput == cols[k]))
		{
			printf("\033[%i;%iH",rows[k],cols[k]);
			printf(" ");
		}
	}
}