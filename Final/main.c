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
#define COLS_		80
#define	ROWS_		24
int ADCx;
int ADCy;
signed char xPos = 1;
signed char yPos = 1;
char pressed = 0;
char time = 60;
int score = 0;
char str[16];
char rows[11];
char cols[11];
char timer0_flag = 0;
char keyflag = 0;
char asciichar;
char portvalue;
char keyvalue;
char nboxes;
char hits = 0;
unsigned int i;
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
void stickPress(void) __interrupt 2;
void TIMER0_ISR(void) __interrupt 1;
char getkeychar(void);
void Menu(void);
void generateBox(char n);
void checkBox(char x, char y);
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
    char w;
	char choice;
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
	EX0 = 1;
	Menu();

	choice = getkeychar();
	//printf("%c",choice);
	switch(choice){
		case 'A':
			nboxes = 5;
			//printf("worked");
			generateBox(5);
			//printf("\n\rAfter generateBox()");
			break;
		case 'B':
			nboxes = 7;
			generateBox(7);
			break;
		case 'C':
			nboxes = 9;
			generateBox(9);
			break;
		case 'D':
			nboxes = 11;
			generateBox(11);
			break;
		default:
			lcd_clear();
			lcd_puts((char*)&"Invalid input");
	}
	
	printf("\033[H");
	for(w=0;w<11;w++){
		printf("%d,",rows[w]);	
	}
	printf("\n\r");
	for(w=0;w<11;w++){
		printf("%d,",cols[w]);	
	}
	

	printf("\033[H");
	while(1)
    {
		get_XY();
		if(ADCx > 3200){//move left
			printf("\033[1D");
			xPos-=1;
			if(xPos < 1) xPos = 1;
		}
		if(ADCx < 500 ){//move right
			printf("\033[1C");
			xPos+=1;
			if(xPos > COLS_) xPos = COLS_;
		}
		if(ADCy < 150){//move up
			printf("\033[1A");
			yPos-=1;
			if(yPos < 1) yPos = 1;
		}
		if(ADCy > 3200){//move down
			printf("\033[1B");
			yPos+=1;
			if(yPos > ROWS_) yPos = ROWS_;
		}
		
		if(timer0_flag == 20){
			timer0_flag = 0;
			time-=1;
		}

		/*
		//Display time
		sprintf(str,"%u",time);
		lcd_clear();
		lcd_puts(str);
		*/
		
		if(nboxes==0){
			printf("\033[2J");
			printf("YOU WIN");
			while(1);
		}
		
		if(time == 0){
			printf("\033[2J");
			printf("TIME UP");
			while(1);
		}
	
		for(i=0;i<65535;i++);
    }
	
}

void Menu(void)
{
	printf("Select difficulty level\n\r");
	printf("A.) Easy\n\r");
	printf("B.) Medium\n\r");
	printf("C.) Hard\n\r");
	printf("D.) GG\n\r");
}

char getkeychar(){
	while(!keyflag){
		//printf("Keyflag not set\n\r");
	}
	keyflag = 0;
	return asciichar;
}

void generateBox(char n)
{	
	char row,col,j;
	//char box = 129;
	printf("\033[2J");
	for(j=0;j<n;j++)
	{
		row = rand()%24;
		col = rand()%80;
		rows[j] = row;
		cols[j] = col;
		//printf("row:%i   col:%i \r\n", rows[j],cols[j]);
		printf("\033[%d;%dH",row,col);
		printf("%c",127);
		/*
		lcd_cmd(0x01);	
		lcd_goto(0);		
		lcd_puts("True");
		*/
	}
}

void checkBox(char x, char y)
{
	char k;
	hits+=1;
	/*
	sprintf(str,"(%d,%d)",x,y);
	lcd_clear();
	lcd_puts(str);
	*/
	
	for(k=0;k<11;k++)
	{
		if((y == rows[k])&&(x == cols[k]))
		{
			//printf("\033[%c;%cH",y,x);
			rows[k] = 0;
			cols[k] = 0;
			printf(" ");
			nboxes-=1;
			
			//debug
			printf("\033[H");
			for(i=0;i<11;i++){
				printf("%d,",rows[i]);	
			}
			printf("\n\r");
			for(i=0;i<11;i++){
				printf("%d,",cols[i]);	
			}
			
			break;
		}
	}
	printf("\033[H\n\nPresses: %d\n\r%d",hits,nboxes);
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

void stickPress(void) __interrupt 2{
	for(i=0;i<200;i++);
	checkBox(xPos,yPos);
}

void TIMER0_ISR(void) __interrupt 1{  // reset timer to 0x3580 and increment flag
	TH0 = 0x35;
	TL0 = 0x80;
	timer0_flag += 1;
}
//------------------------------------------------------------------------------------
// SYSCLK_Init
//------------------------------------------------------------------------------------
//
// Initialize the system clock to use a 22.1184MHz crystal as its clock source
//
void SYSCLK_INIT(void)
{
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
    XBR1     = 0x14;
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up

	EA 		 = 1;						// Enable global interrupts
	EX1 	 = 1;						// Enable external interrupt 1

    P0MDOUT |= 0x01;                    // Set TX0 on P0.0 pin to push-pull
    P1MDOUT |= 0x40;                    // Set green LED output P1.6 to push-pull
	P3MDOUT  = 0xF0;                    // Set P3 high nibble as output, low nibble as input
	P3 		 = 0x0F;					// P3 high nibble set to 0v

	SFRPAGE  = TIMER01_PAGE;
	IT1 	 = 1;						// /INT1 triggered on falling edge

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

	TMOD &= 0xF0;				// Timer0, Mode 1: 16-bit counter/timer.
	TMOD |= 0x01;
	TH0 = 0x35;					// Set high byte such that timer0 starts at 0x3580
	CKCON &= ~0x09;
	CKCON |= 0x02;				// Timer0 uses SYSCLK/48 as base
	TL0 = 0x80;					// Set high byte such that timer0 starts at 0x3580
	TR0 = 1;					// Start timer0

	SFRPAGE = CONFIG_PAGE;
	ET0 = 1;					// Enable timer0 interrupt

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