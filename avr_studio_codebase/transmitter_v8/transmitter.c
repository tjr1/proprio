/*
* FinaLProJect.c
*
* Created: 4/14/2012 1:54:55 PM
* Author: 
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include<stdlib.h>
#include<string.h>
#include <util/delay.h>
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
#define ADDR      0xbb                                // address in memory

// bit handling macros for i/o registers
#define READ(U, N) ((U) >> (N) & 1u)
#define SET(U, N) ((void)((U) |= 1u << (N)))
#define CLR(U, N) ((void)((U) &= ~(1u << (N))))
#define FLIP(U, N) ((void)((U) ^= 1u << (N)))

#define begin 	{
#define end		}

/*Variable Initializations*/
uint8_t msg;
uint8_t test;
int flag;

int matSize;
uint16_t testMat[14][90];
uint16_t rowTX[90];



/*Method Initializations
void aux_putchar(char c) ;
char aux_getchar(void);

//--- AUX putchar ------------------------------------------------
// on pin D3
// 2400 baud is 416 usec/bit
#define bit_time 416

char aux_getchar(void) {return 'x';}

void aux_putchar(char c) 
begin
    char i ;
    //start bit
    CLR(PORTD, 3) ;
    _delay_us(bit_time) ;

    for (i=0; i<=7; i++){
        if (c & 0x01)
            SET(PORTD,3) ;
        else
            CLR(PORTD,3) ;
        c >>= 1 ;
        _delay_us(bit_time) ;
    }
    SET(PORTD, 3) ;
    _delay_us(bit_time) ;
end

FILE aux_uart = FDEV_SETUP_STREAM(aux_putchar, aux_getchar, _FDEV_SETUP_RW);

//--- AUX putchar ------------------------------------------------
*/



/*USART SEND*/
void USART_SENDBYTE(unsigned char SendByte)
begin 
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = SendByte;
end

/*Send Packet*/
void Send_Packet( uint8_t row_addr, uint8_t col_addr, uint16_t msg)
begin
	USART_SENDBYTE(SYNC);
	USART_SENDBYTE(ADDR); // send address, to exclude other user on the same
	                      // RF channel

	//Column Address
	USART_SENDBYTE(col_addr);

	USART_SENDBYTE((uint8_t)(msg & 0x000F));
	USART_SENDBYTE((uint8_t)((msg & 0x00F0)>>4));
	USART_SENDBYTE((uint8_t)((msg & 0x0F00)>>8));
	USART_SENDBYTE(row_addr);

	USART_SENDBYTE(ANTISYNC);
end

/*Main*/
int main(void)
begin

  		DDRC |= 0b00001000;
		PORTC = 0b00001000;
		
		UBRR0H = (uint8_t) (UBRRVAL>>8);  	// high byte
		UBRR0L = (uint8_t) UBRRVAL;   		// low byte        
		UCSR0B = (1<<TXEN0);  				// enable transmitter	
		// data frame format, asynch mode, no parity, 1 stop bit, 8 bit size
        UCSR0C = (0<<UMSEL00) | (0<<UMSEL01) | (0<<UPM00) | (0<<UPM01) | (0<<USBS0) | (3<<UCSZ00) ;    
		

		Send_Packet(0,255,0xffff);

		
		/*
		for(int k = 50; k < 90; k++)
		begin

			for(int j = 0; j < 14; j++)
			begin
				Send_Packet(j, k, 0xf0ff);
			end

			//_delay_ms(10);

		end
		*/
		
		
		/*
		for(int j = 0; j < 14; j++)
		begin
			Send_Packet(j, 4, 0x0ff0);
		end

		for(int j = 0; j < 14; j++)
		begin
			Send_Packet(j, 89, 0x0ff0);
		end
		*/

		/*
		////////////////print normal sexy////////////////////
		//"Y"
		Send_Packet(3,60,0x00ff);
		Send_Packet(4,61,0x10ff);
		Send_Packet(5,62,0x20ff);
		Send_Packet(6,62,0x30ff);
		Send_Packet(7,62,0x40ff);
		Send_Packet(4,63,0x10ff);
		Send_Packet(3,64,0x00ff);
		_delay_ms(200);
		//"X"
		Send_Packet(3,66,0x0f0f);
		Send_Packet(7,66,0x4f0f);
		Send_Packet(4,67,0x1f0f);
		Send_Packet(6,67,0x3f0f);
		Send_Packet(5,68,0x2f0f);
		Send_Packet(4,69,0x1f0f);
		Send_Packet(6,69,0x3f0f);
		Send_Packet(3,70,0x0f0f);
		Send_Packet(7,70,0x4f0f);
		_delay_ms(200);
		//"E"
		Send_Packet(3,72,0x0ff0);
		Send_Packet(5,72,0x2ff0);
		Send_Packet(7,72,0x4ff0);
		Send_Packet(3,73,0x0ff0);
		Send_Packet(5,73,0x2ff0);
		Send_Packet(7,73,0x4ff0);
		Send_Packet(3,74,0x0ff0);
		Send_Packet(4,74,0x1ff0);
		Send_Packet(5,74,0x2ff0);
		Send_Packet(6,74,0x3ff0);
		Send_Packet(7,74,0x4ff0);
		_delay_ms(200);
		//"S"
		Send_Packet(3,76,0x00f0);
		Send_Packet(5,76,0x20f0);
		Send_Packet(6,76,0x30f0);
		Send_Packet(7,76,0x40f0);
		Send_Packet(3,77,0x00f0);
		Send_Packet(5,77,0x20f0);
		Send_Packet(7,77,0x40f0);
		Send_Packet(3,78,0x00f0);
		Send_Packet(4,78,0x10f0);
		Send_Packet(5,78,0x20f0);
		Send_Packet(7,78,0x40f0);
		*/
		/*
		////////////////print inverted sexy////////////////////
		//"Y"
		Send_Packet(3,60,0x0ff0);
		Send_Packet(4,61,0x1ff0);
		Send_Packet(5,62,0x2ff0);
		Send_Packet(6,62,0x3ff0);
		Send_Packet(7,62,0x4ff0);
		Send_Packet(4,63,0x1ff0);
		Send_Packet(3,64,0x0ff0);
		_delay_ms(200);
		//"X"
		Send_Packet(3,66,0x0fff);
		Send_Packet(7,66,0x4fff);
		Send_Packet(4,67,0x1fff);
		Send_Packet(6,67,0x3fff);
		Send_Packet(5,68,0x2fff);
		Send_Packet(4,69,0x1fff);
		Send_Packet(6,69,0x3fff);
		Send_Packet(3,70,0x0fff);
		Send_Packet(7,70,0x4fff);
		_delay_ms(200);
		//"E"
		Send_Packet(3,72,0x0ff0);
		Send_Packet(5,72,0x2ff0);
		Send_Packet(7,72,0x4ff0);
		Send_Packet(3,73,0x0ff0);
		Send_Packet(5,73,0x2ff0);
		Send_Packet(7,73,0x4ff0);
		Send_Packet(3,74,0x0ff0);
		Send_Packet(4,74,0x1ff0);
		Send_Packet(5,74,0x2ff0);
		Send_Packet(6,74,0x3ff0);
		Send_Packet(7,74,0x4ff0);
		_delay_ms(200);
		//"S"
		Send_Packet(3,76,0x00f0);
		Send_Packet(5,76,0x20f0);
		Send_Packet(6,76,0x30f0);
		Send_Packet(7,76,0x40f0);
		Send_Packet(3,77,0x00f0);
		Send_Packet(5,77,0x20f0);
		Send_Packet(7,77,0x40f0);
		Send_Packet(3,78,0x00f0);
		Send_Packet(4,78,0x10f0);
		Send_Packet(5,78,0x20f0);
		Send_Packet(7,78,0x40f0);
		*/
		/*
		//////////Reset Sexy pixels/////////////////////////////////////////////
		Send_Packet(3,60,0x0fff);
		Send_Packet(4,61,0x1fff);
		Send_Packet(5,62,0x2fff);
		Send_Packet(6,62,0x3fff);
		Send_Packet(7,62,0x4fff);
		Send_Packet(4,63,0x1fff);
		Send_Packet(3,64,0x0fff);
		_delay_ms(200);
		//"X"
		Send_Packet(3,66,0x0fff);
		Send_Packet(7,66,0x4fff);
		Send_Packet(4,67,0x1fff);
		Send_Packet(6,67,0x3fff);
		Send_Packet(5,68,0x2fff);
		Send_Packet(4,69,0x1fff);
		Send_Packet(6,69,0x3fff);
		Send_Packet(3,70,0x0fff);
		Send_Packet(7,70,0x4fff);
		_delay_ms(200);
		//"E"
		Send_Packet(3,72,0x0fff);
		Send_Packet(5,72,0x2fff);
		Send_Packet(7,72,0x4fff);
		Send_Packet(3,73,0x0fff);
		Send_Packet(5,73,0x2fff);
		Send_Packet(7,73,0x4fff);
		Send_Packet(3,74,0x0fff);
		Send_Packet(4,74,0x1fff);
		Send_Packet(5,74,0x2fff);
		Send_Packet(6,74,0x3fff);
		Send_Packet(7,74,0x4fff);
		_delay_ms(200);
		//"S"
		Send_Packet(3,76,0x0fff);
		Send_Packet(5,76,0x2fff);
		Send_Packet(6,76,0x3fff);
		Send_Packet(7,76,0x4fff);
		Send_Packet(3,77,0x0fff);
		Send_Packet(5,77,0x2fff);
		Send_Packet(7,77,0x4fff);
		Send_Packet(3,78,0x0fff);
		Send_Packet(4,78,0x1fff);
		Send_Packet(5,78,0x2fff);
		Send_Packet(7,78,0x4fff);
		*/
	while(1)
	{
		
	}
		
end





