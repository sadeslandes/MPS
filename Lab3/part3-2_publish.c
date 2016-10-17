
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
char scroll_dwn = 0;
char dummy;
//------------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------------
void main(void);
void SYSCLK_INIT(void);
void PORT_INIT(void);
void UART0_INIT(void);
void SPI0_READ(void);
void SPI0_WRITE(void);
void write_dummy(void);

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
void main(void)
{
	WDTCN = 0xDE;                       // Disable the watchdog timer
    WDTCN = 0xAD;

    SYSCLK_INIT();                      // Initialize the oscillator
	PORT_INIT();
	UART0_INIT();
    
	

	SFRPAGE = UART0_PAGE;    
	
	printf("\033[2J");                  // Erase screen & move cursor to home position
	printf("Local char typed");
	printf("\033[13;0H");
	printf("Received char\n\r");              // jump down type
	printf("\033[s"); 									// save position
	
	
	while(1)
    {
		SFRPAGE = UART0_PAGE;  
		if(RI0){
			RI0 = 0;
			SPI0_WRITE();
			SPI0_READ();
		}
    }
}

void SPI0_WRITE(void){
	char i;
	char j; 

	choice = SBUF0;      		// get char from terminal
	SFRPAGE = SPI0_PAGE;		
	SPIF = 0;
	NSSMD0 = 0;					// select slave
	while(SPI0CFG & 0x80);		// Make sure SPI is not busy
	SPIF = 0;					// clear SPIF
	SPI0DAT = choice;           // Load char into SPI0DAT
	for(i=0;i<100;i++);			// Small delay
	while(!SPIF);				// Wait until transmission complete
	SPIF = 0;					
	
	//Read Dummy
	NSSMD0 = 1;					// Release slave
	dummy = SPI0DAT;			// Get received char from SPI0DAT (dummy)
	//for(i=0;i<100;i++);			// Small delay
	
	if(choice == 0x7F){
		while(SPI0DAT != 0xFF){
			for(i=0;i<100;i++);	
			write_dummy();
			SPI0_READ();
			//if(choice == 0xFF) break;
		}		
	}
	else{
		//ANSI formatting
		printf("\033[2;12r");
		printf("\033[2;0H");
		for(j=0;j<scroll_dwn;j++){
			printf("\033[B");
		}
		
		//Output local char						
		printf("Choice is: %c\r",choice);

		write_dummy();		
	}
}

void write_dummy(void){
	//Write Dummy
	NSSMD0 = 0;					// Select slave
	while(SPI0CFG & 0x80);		// Make sure SPI is not busy
	SPIF = 0;					// Clear SPIF
	SPI0DAT = 0xFE;				// Load dummy into SPI0DAT
	//for(i=0;i<100;i++);			// Small delay
	while(!SPIF);				// Wait until transmission complete
	SPIF = 0;
}

void SPI0_READ(void){
	char j;	

	NSSMD0 = 1;					// Release slave
	//Output dummy
	printf("\033[2;30H");
	printf("DUMMY: 0x%x", dummy);	
	SPIF = 0;

	//ANSI format	
	printf("\033[14;24r");
	printf("\033[14;0H");
	for(j=0;j<scroll_dwn;j++){
		printf("\033[B");
	}
	
	//Output choice	
	//choice = SPI0DAT;						
	printf("Data read from SPI0DAT is: %c\r",choice);
	
	scroll_dwn += 1;
	if (scroll_dwn > 10) scroll_dwn = 10;

	SPIF = 0;
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
	SPI0CKR  = 	0x18;					// SPI clock rate for 995,328
	
	EA = 1;
	
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
