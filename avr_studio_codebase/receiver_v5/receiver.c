/* Sign Language Translator*/
/* Receiver*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include<stdlib.h>
#include<string.h>
#include <util/delay.h>
#include"lcd_lib.h"
#include <stdio.h>
#include "uart.h"


uint8_t lcd_buffer[8];	// LCD display buffer

/* system definitions*/

#ifndef F_CPU                                            // defines system clock frequency
#define F_CPU        16000000
#endif

#define BAUDRATE  2400                                // define the BAUDRATE
#define UBRRVAL        ((F_CPU/(BAUDRATE*16UL))-1)
#define SYNC      0xAA                                // synchro signal
#define ANTISYNC  0x7F                                // synchro signal
#define ADDR     0xbb                                // address in memory

#define debug 	0

/*Variable Initializations*/
unsigned char msg[1];
uint8_t test;
uint8_t count;

int matSize;
volatile uint16_t testMat[14][90];
uint16_t colTX[14];
uint16_t col_data;
uint8_t nible_1,nible_2,nible_3,nible_4;
int col,row,msbit,SET_COL;


volatile int lcd_flag;
volatile unsigned char lcd_char[15];
int char_index;

enum STATE{
	WAIT_SYNC,
	WAIT_ADDRESS,
	WAIT_DATA,
	DATA_RECEIEVED
};

volatile enum STATE state;
volatile enum STATE next_state;


// LCD initializatin
void init_lcd(void) 
{
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
}

//Display on the LCD
void DispLCD(unsigned char msg, int row, int col)
	{	
		
		LCDGotoXY(col,row);
		sprintf(lcd_buffer,"%c",msg);
		LCDstring(lcd_buffer, strlen(lcd_buffer));
	}
		


/*Interupt for receive*/
ISR(USART0_RX_vect)
{		
	uint8_t data;

	data = UDR0;
	
	switch(state){
		case WAIT_SYNC:
			if(data==SYNC)
				next_state=WAIT_ADDRESS;

			if (debug) lcd_char[++char_index] = 'w';
		break;

		case WAIT_ADDRESS:
if(data == ADDR)
				next_state = WAIT_DATA;
			else
				next_state = WAIT_SYNC;

if (debug) lcd_char[++char_index] = 'g';
		break;

		case WAIT_DATA:
			if (data == ANTISYNC)
			{
				next_state = DATA_RECEIEVED;
if (debug) lcd_char[++char_index] = 'a';	
			}
			else
			{
				if (SET_COL < 5)
				{
					if (SET_COL == 0) 
						col = data;

					else if(SET_COL == 1)
						nible_1 = data & 0x0F;
					
					else if(SET_COL == 2)
						nible_2 = data & 0x0F;
						
					else if(SET_COL == 3)
						nible_3 = data & 0x0F;						
						
					else if(SET_COL == 4)
						nible_4 = data & 0x0F;

if (debug) lcd_char[++char_index] = 'c';
					next_state = WAIT_DATA;	
					SET_COL++;
				}
				else //(SET_COL >= 5)
				{
if (debug) lcd_char[++char_index] = 'd';
					next_state = DATA_RECEIEVED;
				}
			}		
		break;

		case DATA_RECEIEVED:
			row = nible_1;

			col_data = (uint16_t)nible_1;
			col_data = col_data + (((uint16_t)nible_2)<<4);
			col_data = col_data + (((uint16_t)nible_3)<<8);
			col_data = col_data + (((uint16_t)nible_4)<<12);

			testMat[row][col] = col_data;

if (1) 
if (col == 1 && col_data == 0x0aff)
lcd_char[++char_index] = 'm';
else
lcd_char[++char_index] = 'p';

			next_state = WAIT_SYNC;

			SET_COL = 0;
			row = 0;
			col = 0;
			nible_1 = 0;
			nible_2 = 0;
			nible_3 = 0;
			nible_4 = 0;
		break;

		default:
			state=WAIT_SYNC;
	}//end switch

	state = next_state;
}

/*Main*/
int main(void)
{	

	lcd_flag=0;
	init_lcd(); 
	count = 0;  
	state=WAIT_SYNC;
	
	//USART Initialization
	UBRR0H = (uint8_t) (UBRRVAL>>8);  // high byte
	UBRR0L = (uint8_t) UBRRVAL;   // low byte        
	UCSR0B =  (1<<RXEN0)| (1 << RXCIE0) ;  // enable Receiver	
	// data frame format, asynch mode, no parity, 1 stop bit, 8 bit size
    UCSR0C = (0<<UMSEL00) | (0<<UMSEL01) | (0<<UPM00) | (0<<UPM01) | (0<<USBS0) | (3<<UCSZ00) ;   
	
	SET_COL = 0;
	col=0;
	row=0;	
	nible_1 = 0;
	nible_2 = 0;
	nible_3 = 0;
	nible_4 = 0;
	
	sei();
	DispLCD('O', 1, 5); 


	
	while(1){
		LCDclr();				//clear the display
		LCDGotoXY(0,0);
		for(int h = 0; h < 15; h++)
			DispLCD(lcd_char[h], 1, count++);
		
		char_index = 0;
		count = 0;
		_delay_ms(600);
	}//end while
}


