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
#define	ROWS_		50
#define CHAR_ 		219
int ADCx;
int ADCy;
int sens;
char time;
int score = 0;
char str[16];
char rows[11];
char cols[11];
char timer0_flag = 0;
char keyflag = 0;
signed char xPos, yPos;
//Key wakeup vars
char asciichar;
char portvalue;
char keyvalue;

char nboxes;
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
void read_ADC(void);
void stickPress(void) __interrupt 2;
void TIMER0_ISR(void) __interrupt 1;
char getkeychar(void);
void Menu(void);
void generateBox(void);
void drawTarget(void);
void eraseTarget(char row, char col);
void checkTarget(char x, char y);
void checkBox(char x, char y);
void WinScreen(void);
void LoseScreen(void);
void playGame(void);
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
	//char c;
	WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    PORT_INIT();                        // Initialize the Crossbar and GPIO
    SYSCLK_INIT();                      // Initialize the oscillator
    UART0_INIT();                       // Initialize UART0
	TIMER0_INIT();						// Initialize TIMER0
	ADC0_INIT();
	lcd_init();							// Initialize LCD

    SFRPAGE = UART0_PAGE;               // Direct output to UART0
	EX0 = 1;
    
	while(1){
		playGame();
	}
	
}

void playGame(){
	unsigned int speed;
	xPos = 1;					// Corresponds with column
	yPos = 1;					// Corresponds with row
	score = 0;

	printf("\033[1;37;44m");
	printf("\033[2J");
	Menu();
	printf("\033[H");
	
	TR0 = 1;					// Start timer0
	while(1){
		// Cursor movement control
		read_ADC();
		speed = -11*sens+65535;		
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
		
		if(timer0_flag == 10){
			timer0_flag = 0;
			time-=1;
		}

		// Write to lcd
		// Display time
		sprintf(str,"%u",time);
		lcd_clear();
		lcd_goto(0);
		lcd_puts(str);
		// Display score
		lcd_goto(0x40);
		sprintf(str,"Score: %u",score);
		lcd_puts(str);

		//Check win
		if(nboxes==0){
			WinScreen();
			break;
		}
		
		//Check lose
		if(time == 0){
			LoseScreen();
			break;
		}

		if(time == 5){
			P2 = 0x01;
		}
		if(time == 4){
			P2 = 0x00;
		}
		

		for(i=0;i<speed;i++);
	}
	TR0 = 0;
	TH0 = 0xA6;
	TL0 = 0x00;
	while(1){	
		if(getkeychar()=='#') break;
	}
}

void Menu(void)
{
	char choice;
	printf("Select difficulty level\n\r");
	printf("A.) Easy\n\r");
	printf("B.) Medium\n\r");
	printf("C.) Hard\n\r");
	printf("D.) GG\n\r");

	choice = getkeychar();
	//printf("%c",choice);
	switch(choice){
		case 'A':
			time = 60;
			nboxes = 5;
			drawTarget();
			break;
		case 'B':
			time = 45;
			nboxes = 7;
			drawTarget();
			break;
		case 'C':
			time = 30;
			nboxes = 9;
			drawTarget();
			break;
		case 'D':
			time = 15;
			nboxes = 11;
			drawTarget();
			break;
		default:
			lcd_clear();
			lcd_puts((char*)&"Invalid input");
	}
}

char getkeychar(){
	while(!keyflag);
	keyflag = 0;
	return asciichar;
}

void WinScreen(void)
{
    printf("\033[H");
    printf("\033[30m");
    printf("\033[47m");
    printf("\033[2J");
    printf("\033[24;35H");
    for(i=0; i<10; i++)
    {
        printf("-");
    }
    printf("\033[25;36H");
    printf("You WIN!");
    printf("\033[25;35H");
    printf("|");
    printf("\033[25;44H");
    printf("|");
    printf("\033[26;35H");
    for(i=0; i<10; i++)
    {
        printf("-");
    }
}

void LoseScreen(void)
{
    printf("\033[H");
    printf("\033[30m");
    printf("\033[47m");
    printf("\033[2J");
    printf("\033[24;34H");
    for(i=0; i<12; i++)
    {
        printf("-");
    }
    printf("\033[25;35H");
    printf("You LOSE!!");
    printf("\033[25;34H");
    printf("|");
    printf("\033[25;45H");
    printf("|");
    printf("\033[26;34H");
    for(i=0; i<12; i++)
    {
        printf("-");
    }
}

void drawTarget(){
	char row,col,j;
	char redo;
	//char box = 129;
	printf("\033[2J");
	for(j=0;j<nboxes;j++)
	{
		redo = 0;
		//srand(TL0);
		row = rand()%(ROWS_-2) +2;  // check my logic
		col = rand()%(COLS_-2) +2;


		// Check that targets down collide
		for(i=0;i<j;i++){
			if((row >= rows[i]-2)&&\
			   (row <= rows[i]+2)&&\
			   (col >= cols[i]-2)&&\
			   (col <= cols[i]+2)){
				redo+=1;
				break;
			}
		}
		
		if(redo){
			j-=1;
			continue;
		}


		rows[j] = row;
		cols[j] = col;
		
		
		printf("\033[37m");	// change color to yellow
		// Yellow blocks to mark target
		printf("\033[%d;%dH",row-1,col-1);
		printf("%c%c%c",CHAR_,CHAR_,CHAR_);
		printf("\033[%d;%dH",row+1,col-1);
		printf("%c%c%c",CHAR_,CHAR_,CHAR_);
		printf("\033[%d;%dH",row,col-1);
		printf("%c%c%c",CHAR_,CHAR_,CHAR_);
		// Red bullseye
		printf("\033[%d;%dH",row,col);
		printf("\033[31m"); // change color to red
		printf("%c",CHAR_);

		
	}
}

void eraseTarget(char row,char col){
	printf("\033[%d;%dH",row-1,col-1);
	printf("   ");
	printf("\033[%d;%dH",row,col-1);
	printf("   ");
	printf("\033[%d;%dH",row+1,col-1);
	printf("   ");
}


void checkTarget(char x, char y){
	char k;
	char hit = 0;
	
	for(k=0;k<11;k++)
	{
		// Hit detection by rows

		// row above
		if(y==rows[k]-1){
			if((x>=cols[k]-1)&&(x<=cols[k]+1)){
				score+=10;
				hit+=1;
				break;
			}
		}
		// row below
		if(y==rows[k]+1){
			if((x>=cols[k]-1)&&(x<=cols[k]+1)){
				score+=10;
				hit+=1;
				break;
			}
		}
		// center row
		if(y==rows[k]){
			if(x==cols[k]){
				// Bullseye
				score+=100;
				hit+=1;
				break;
			}
			if((x==cols[k]-1)||(x==cols[k]+1)){
				score+=10;
				hit+=1;
				break;
			}
		}
	}
	if(hit){
		
		printf("\033[s");
		eraseTarget(rows[k],cols[k]);
		printf("\033[u");
		rows[k] = 200; // unreachable location
		cols[k] = 200; // unreachable location
		nboxes-=1;
	}
	//printf("\033[H\n\nPresses: %d\n\r%d",hits,nboxes);
}


void read_ADC(){
	AMX0SL = 0x00;
	for(i=0;i<300;i++);
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
	ADCy = ADC0; //ADC0H;
	
	
	AMX0SL = 0x01;
	for(i=0;i<300;i++);
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
	ADCx = ADC0; //ADC0H;

	AMX0SL = 0x02;
	for(i=0;i<300;i++);
	AD0INT = 0; 				// Clear conversion interrupt flag
	AD0BUSY = 1;   				// Start conversion
	while(!AD0INT);				// Wait for conversion to end
	sens = ADC0; //ADC0H;
	
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
	//printf("\033[1;1H");
	for(i=0;i<300;i++);
	//checkBox(xPos,yPos);
	checkTarget(xPos,yPos);
}

void TIMER0_ISR(void) __interrupt 1{  // reset timer to 0x3580 and increment flag
	TH0 = 0xA6;
	TL0 = 0x00;
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
    OSCXCN  = 0x77;                     // Start ext osc with 11.0592MHz crystal
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
    XBR1     = 0x14;
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up

	EA 		 = 1;						// Enable global interrupts
	EX1 	 = 1;						// Enable external interrupt 1

    P0MDOUT |= 0x01;                    // Set TX0 on P0.0 pin to push-pull
    P1MDOUT |= 0x40;                    // Set green LED output P1.6 to push-pull
	P2MDOUT  = 0x01;
	P2       = 0x00;
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
    TH1     = 0xFA;    					// Set Timer1 reload baudrate value T1 Hi Byte
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
	TH0 = 0xA6;					// Set high byte such that timer0 starts at 0xA600
	CKCON &= ~0x09;
	CKCON |= 0x02;				// Timer0 uses SYSCLK/48 as base
	TL0 = 0x00;					// Set high byte such that timer0 starts at 0xA600


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