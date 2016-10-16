// SPISLAVE.C 
// RPK
// September 25, 2001
// August 15, 2008 (revised)
//
// Compile, link & hex convert: CC6811 SPISLAVE
//
// Synchronous Serial Port slave device for the 68HC11 EVB
// for use as a test device for a Synchronous Serial Port Master
// implemented on the C8051F120 SiLabs EVB.
//
// Hardware connections:
// 68HC11 BUS              C8051 PORT
// MISO (pin 22) output TO MISO (P0.3) input
// MOSI (pin 23) input  TO MOSI (P0.4) output
// SCK  (pin 24) input  TO SCK  (P0.2) output
// SS*  (pin 25) input  TO NSS  (P0.5) output
// VSS  (pin  1) (GND)  TO VSS  (P0-9) (GND)


#include<HC11A1.h>
#include<introl.h>
#include<stdio.h>
#include<stdlib.h>


void main()
{
	char char0, char1;
	unsigned char l;

	_H11DDRD = 0x04;	//0x3C or 0x04
	_H11SPCR = 0x40;	//0x40 for slave mode, 0x50 for master mode
	printf("\033[2J");
//	for(l=0;l<28;l++)printf("\n")

	printf("\nHC11 SPI Slave mode initialization: %x %x \n\r", &_H11SPCR, _H11SPCR);
	printf("   Hardware connections:\r");
	printf("68HC11 BUS              C8051 PORT\r");
	printf("MISO (pin 22) output TO MISO (P0.3) input\r");
	printf("MOSI (pin 23) input  TO MOSI (P0.4) output\r");
	printf("SCK  (pin 24) input  TO SCK  (P0.2) output\r");
	printf("SS*  (pin 25) input  TO NSS  (P0.5) output\r");
	printf("VSS  (pin  1) (GND)  TO VSS  (P0-9) (GND)\r");

	printf("\nThis program echoes characters received on the SPI port\r");
	printf("to the 68HC11 console and back to the sender through the SPI\r");
	printf("on the next transmission cycle from the master device.\r\n");
	printf("Receiving a <del> (0x7F) will result in the sending of a\r");
	printf("95 character message (ASCII 0x20 through 0x7E) followed \r\n");
	printf("by 4 more characters: 0x0D, 0x0A, 0x07, and 0xFF. \r\n");
	printf("Restart this program after the SPI master if encountering problems.\r");

	while(1)
	{
		while(_H11SPSR < 0x80);		//wait for character
		char0 = _H11SPDR;			//read character
		_H11SPDR = char0;			//echo character back to sender
// This doesn't do anything if SS* is not released
		putchar(char0);				//print on console
		while(_H11SPSR < 0x80);		//wait for transfer
		char1 = _H11SPDR;			//dummy read
//		printf(" '%c'  %x(hex) received\n\r",char0,char0);
		if(char0 == '\177')			//if <del> is received, then
		{
// A stumbling point here is that the slave can't load the data register
// for sending out data unless the master releases the SS* output line tied
// to the slave's SS* input. Otherwise any value that is loaded is ignored.
// This prevents the transfer of new data from the slave to the master.
// Only the previous byte already there is transferred back to the master.
			for(l = 32;l<127;l++)
			{
				_H11SPDR = l;
//				putchar(l);
				while(_H11SPSR < 0x80);
				char0 = _H11SPDR;	//read character
			}
			
			_H11SPDR = '\r';		// \r <return>
//			putchar('\r');
			while(_H11SPSR < 0x80);
			char1 = _H11SPDR;		//dummy read
			_H11SPDR = '\n';		// \n <new line>
//			putchar('\n');
			while(_H11SPSR < 0x80);
			char1 = _H11SPDR;		//dummy read
			_H11SPDR = '\007';		// \007 <bel>
//			putchar('\007');
			while(_H11SPSR < 0x80);
			char1 = _H11SPDR;		//dummy read
			_H11SPDR = '\377';		// \377 <rubout>
			while(_H11SPSR < 0x80);
			char1 = _H11SPDR;		//dummy read

		}
	}
}