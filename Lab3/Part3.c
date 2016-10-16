
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
char choice;
char in_flag = 0;
//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void SPI0_READ(void);
void SPI0_WRITE(void);
void UART0_int(void) __interrupt 4;

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
	char i;
	char j;
	char scroll_dwn = 0;

    WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    SYSCLK_INIT();                      // Initialize the oscillator
	PORT_INIT();
	UART0_INIT();
	//PORT_INIT();                        // Initialize the Crossbar and GPIO
    
	

	SFRPAGE = UART0_PAGE;    
	
	printf("\033[2J");                  // Erase screen & move cursor to home position
	printf("Local char typed");
	printf("\033[13;0H");
	printf("Received char\n\r");              // jump down type
	printf("\033[s"); 									// save position
	
	
	while(1)
    {
		//printf("ENTERED LOOP\r\n");
		SFRPAGE = UART0_PAGE;  
		//Transmit
		if(RI0){
			RI0 = 0;
			choice = SBUF0;
			SFRPAGE = SPI0_PAGE;
			//choice = SPI0DAT;
			SPIF = 0;
			NSSMD0 = 0;
			while(SPI0CFG & 0x80);
			SPIF = 0;
			SPI0DAT = choice;
			for(i=0;i<100;i++);	
			while(!SPIF);	
			SPIF = 0;			
			printf("\033[3;12r");
			printf("\033[2;0H");
			for(j=0;j<scroll_dwn;j++){
				printf("\033[B");
			}							
			//for(j=0;j<100;j++);
			printf("Choice is: %c\r",choice);
			
		
			//Receive

			NSSMD0 = 1;
			for(i=0;i<100;i++);	
			//printf("\033[u"); // restore cursor	
			printf("\033[14;24r");
			printf("\033[14;0H");
			for(j=0;j<scroll_dwn;j++){
				printf("\033[B");
			}							
			printf("Data read from SPI0DAT is: %c\r",SPI0DAT);
			scroll_dwn += 1;
			if (scroll_dwn > 10) scroll_dwn = 10;

			SPIF = 0;
		}
    }
}

void SPI0_READ(void){
	
}

void SPI0_WRITE(void){

}

void UART0_int(void) __interrupt 4{
	if(RI0){
		choice = SBUF0;
		RI0 = 0;
		in_flag = 1;
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

    
	SFRPAGE = CONFIG_PAGE;

	XBR0     = 0x06;                    // Enable UART0,SPI
    XBR1     = 0x00;
    XBR2     = 0x40;                    // Enable Crossbar and weak pull-up
	
	P0MDOUT &= ~0x02; //~0x0A
	P0MDOUT |= 0x35;                    // Set pins 0,2,4,5 to push-pull
    //P0      |= 0x02;                    // RX0 pin and to high impedance
	
	SFRPAGE  = SPI0_PAGE;
    SPI0CFG = 0x40;						// Master mode
	SPI0CN  = 0x0D;					// Enable SPI
	SPI0CKR  = 	0x18;					// SPI clock rate for 230399
	//SPI0CKR  = 19;                    // 1.244 MHz
	
	//SPIF = 1;	
	
	//EIE1 = 0x01;
	EA = 1;
	//ES0 = 1;

	
	SFRPAGE  = SFRPAGE_SAVE;            // Restore SFR page
}

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
    SSTA0   = 0x10;         		    // SMOD0 = 1
    TI0     = 1;                        // Indicate TX0 ready

    SFRPAGE = SFRPAGE_SAVE;             // Restore SFR page
}
