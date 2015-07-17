#include "trtSettings.h"
#include "trtkernel644.c"
#include <util/delay.h>
#include <stdio.h>
#include "lcd_lib.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

// serial communication library
// Don't mess with the semaphores
#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>
//#include "trtUart.h"
//#include "trtUart.c"

/* system definitions*/
#ifndef F_CPU                                            // defines system clock frequency
#define F_CPU        16000000
#endif

#define BAUDRATE  2400                                // define the BAUDRATE
#define UBRRVAL        ((F_CPU/(BAUDRATE*16UL))-1)
#define SYNC      0xAA                                // synchro signal
#define ANTISYNC  0x7F                                // synchro signal
#define ADDR     0xbb                                // address in memory

// bit handling macros for i/o registers
#define READ(U, N) ((U) >> (N) & 1u)
#define SET(U, N) ((void)((U) |= 1u << (N)))
#define CLR(U, N) ((void)((U) &= ~(1u << (N))))
#define FLIP(U, N) ((void)((U) ^= 1u << (N)))

// two semaphores to protect message --
// sender must wait until message is received 
// -- init to 1 becuase don't need to wait for first message
// receiver must wait until message is sent
#define SEM_TX_WAIT  4
#define SEM_RX_WAIT  5

// semaphore to protect shared variable (not needed for this version)
#define SEM_SHARED 7

// directions for anmations
#define LEFT 	0
#define RIGHT 	1
#define UP		2
#define DOWN	3

// states for receiving data
#define WAIT_SYNC 		0
#define WAIT_ADDRESS 	1
#define WAIT_DATA		2

// the usual
#define begin {
#define end }

// Display texts for the LCD stored to flash
const int8_t LCD_dispRPM[] PROGMEM = "RPM:\0";

// input arguments to each thread
// not actually used in this example
int args[4] ;

volatile int motor_period, motor_period_ovlf;	// motor periods needed for rpm calculation
float rpm;										// rpm measurement

int8_t lcd_buffer1[17];							// LCD display buffer for RPM measurement

//*************************************************************
// --- SPI Stuff ------------------------
static uint8_t MAX_PORT0 = 0;
static uint8_t MAX_PORT1 = 1;
static uint8_t MAX_PORT2 = 2;
static uint8_t MAX_PORT3 = 3;
static uint8_t MAX_PORT4 = 4;
static uint8_t MAX_PORT5 = 5;
static uint8_t MAX_PORT6 = 6;
static uint8_t MAX_PORT7 = 7;
static uint8_t MAX_PORT8 = 8;
static uint8_t MAX_PORT9 = 9;

static uint8_t pin_high = 0xFF;		//disable
static uint8_t pin_low  = 0x00;		//enable

unsigned int led;

// Matrix for pixels																		  //36																      //72																	  //108																	  //144																	  //180
int matSize;
volatile uint16_t testMat[14][90];

//PACKET Varibles
volatile uint16_t col_data;
volatile uint8_t nible_1,nible_2,nible_3,nible_4;
volatile int col,row,SET_COL;

// state variables
volatile int state;
volatile int next_state;
volatile uint8_t data;

volatile int ledPos;	//position of 

//for animation control
uint8_t moveTime;
volatile int runAnimations;

void initTimers(void);
void initLCD(void);
void SPIinitialize(void);
void SPIwrite(int index, uint8_t address, uint8_t data);
void initUART(void);


//*************************************************************
// --- external interrupt ISR ------------------------
ISR (INT0_vect) 
begin
		//calculate motor speed
        motor_period = TCNT2 + motor_period_ovlf  ;
        TCNT2 = 0 ;
        motor_period_ovlf = 0 ;	
		ledPos = 0;
end

//************************************************************
// --- set up extra 8 bits on timer 2 ----------------
ISR (TIMER2_OVF_vect) 
begin
        motor_period_ovlf = motor_period_ovlf + 256 ;
end


//************************************************************
/*Interupt for receive*/
ISR(USART0_RX_vect)
{		
	data = UDR0;	//retrieve data from receiver register
	
	//if transmitter is ever cut while transmitting, this will get it back to WAIT_SYNC state
	if(data == SYNC)
	begin
		state = WAIT_SYNC;
		next_state = WAIT_SYNC;
	end
	
	//state machine for receiving data
	switch(state){
		
		//Wait for SYNC 8-bits
		case WAIT_SYNC:
			//reset all the data and pixel 
			SET_COL = 0;
			row = 0;
			col = 0;
			nible_1 = 0;
			nible_2 = 0;
			nible_3 = 0;
			nible_4 = 0;

			//if the sync 8-bits are received, confirm address of transmitter
			if(data==SYNC)
				next_state=WAIT_ADDRESS;
			else
				next_state = WAIT_SYNC; 
		break;

		//Wait ADDR to confirm data is sent from right transmitter
		case WAIT_ADDRESS:

			//if the address 8-bits are received, get data bits
			if(data == ADDR)
				next_state = WAIT_DATA;
			else
				next_state = WAIT_SYNC;
		break;

		//Wait for 5, 8-bit packets of data to arrive
		case WAIT_DATA:

			//If antisync 8-bits are received, the packet is complete
			if (data == ANTISYNC)
			{
				next_state = WAIT_SYNC;
				row = nible_4;

				//Combine the bits received into 16-bit pixel information
				col_data = (uint16_t)nible_1
							+ (((uint16_t)nible_2)<<4)
							+ (((uint16_t)nible_3)<<8)
							+ (((uint16_t)nible_4)<<12);
				
				//Confirm data is within bounds
				if((row < 14) && (row>=0) && (col<90) && (col>=0))
					testMat[row][col] = col_data;
				
				//If OPP code of col = 255, clear entire display
				if(col == 255)
				begin
					for(int k = 0; k < 14; k++)
					begin
						for(int j = 0; j < 90; j++)
						begin
							testMat[k][j] = 0xffff;
						end
					end
				end

				//If OPP code of col = 254, run animation
				if(col == 254)
					runAnimations = 1;
				else if(col == 253)
					runAnimations = 0;

				next_state = WAIT_SYNC;
	
			}
			else
			{
				//Parse incoming data into appropriate nibbles in order of receving
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

					next_state = WAIT_DATA;	
					SET_COL++;
				}
				else
				{
					next_state = WAIT_SYNC;
				}
			}		
		break;

		default:
			state=WAIT_SYNC;
		break;
	}//end switch

	state = next_state;
}

//***************************************************************
// --- Execute main loop ----------------------------------
void task1(void* args)
begin
	uint32_t rel, dead;
	while(1)
	begin
		if(motor_period != 0)
			rpm = 1/(.000064*motor_period);	//calculate current rpm
		else
			rpm = 0;

		//update release and deadline
		rel = trtCurrentTime() + SECONDS2TICKS(0.005);
		dead = trtCurrentTime() + SECONDS2TICKS(0.006);
		trtSleepUntil(rel, dead);
	end
end

//***************************************************************
// --- Run LED code ----------------------------------
void task3(void* args)
begin
	
	uint32_t rel, dead;
	float spr;
	uint8_t r[14];
	uint8_t g[14];
	uint8_t b[14];
	uint16_t tempR;
	uint16_t tempG;
	uint16_t tempB;


	while(1)
	begin
		
		//if the ledPos varible is valid
		if(ledPos < 90)
		begin

			//for each row pixel in the col (= ledPos), update the LED intensity.   
			for(int k = 0; k < 14; k++)
			begin
				//Map 4-bit value to 8-bit value.  (ie. 0a->aa, 04->44)
				tempR = testMat[k][ledPos] & 0x000f;
				tempG = testMat[k][ledPos] & 0x00f0;
				tempB = testMat[k][ledPos] & 0x0f00;
				r[k] = (uint8_t)((tempR<<4)+tempR);
				g[k] = (uint8_t)((tempG)+(tempG>>4));
				b[k] = (uint8_t)((tempB>>4)+(tempB>>8));
			end

			/*
			LED Mapping Scheme:
			LED#	Color	Chip_Sel	Port#
			1		B		0			0
			1		G		0			1
			1		R		0			2
			2		B		0			5
			2		G		0			6
			2		R		0			7
			3		B		2			0
			3		G		2			1
			3		R		2			2
			4		B		0			8
			4		G		0			9
			4		R		1			8
			5		B		0			3
			5		G		0			4
			5		R		2			3
			6		B		1			9
			6		G		2			5
			6		R		2			6
			7		B		2			4
			7		G		1			2
			7		R		1			3
			8		B		2			7
			8		G		2			8
			8		R		2			9
			9		B		1			4
			9		G		4			7	
			9		R		4			8
			10		B		3			3
			10		G		3			2
			10		R		3			1
			11		B		4			9
			11		G		3			5
			11		R		3			6
			12		B		3			0
			12		G		4			4
			12		R		4			3
			13		B		3			7
			13		G		3			8
			13		R		3			9
			14		B		4			2
			14		G		4			1
			14		R		4			0
			*/	

			//Update all Red LED ports
			SPIwrite(0, MAX_PORT2, r[0]);
			SPIwrite(0, MAX_PORT7, r[1]);
			SPIwrite(2, MAX_PORT2, r[2]);
			SPIwrite(1, MAX_PORT8, r[3]);
			SPIwrite(2, MAX_PORT3, r[4]);
			SPIwrite(2, MAX_PORT6, r[5]);
			SPIwrite(1, MAX_PORT3, r[6]);
			SPIwrite(2, MAX_PORT9, r[7]);
			SPIwrite(4, MAX_PORT8, r[8]);
			SPIwrite(3, MAX_PORT1, r[9]);
			SPIwrite(3, MAX_PORT6, r[10]);
			SPIwrite(4, MAX_PORT3, r[11]);
			SPIwrite(3, MAX_PORT9, r[12]);
			SPIwrite(4, MAX_PORT0, 255);

			//Update all Green LED ports
			SPIwrite(0, MAX_PORT1, g[0]);
			SPIwrite(0, MAX_PORT6, g[1]);
			SPIwrite(2, MAX_PORT1, g[2]);
			SPIwrite(0, MAX_PORT9, g[3]);
			SPIwrite(0, MAX_PORT4, g[4]);
			SPIwrite(2, MAX_PORT5, g[5]);
			SPIwrite(1, MAX_PORT2, g[6]);
			SPIwrite(2, MAX_PORT8, g[7]);
			SPIwrite(4, MAX_PORT7, g[8]);
			SPIwrite(3, MAX_PORT2, g[9]);
			SPIwrite(3, MAX_PORT5, g[10]);
			SPIwrite(4, MAX_PORT4, g[11]);
			SPIwrite(3, MAX_PORT8, g[12]);
			SPIwrite(4, MAX_PORT1, 255);

			//Update all Blue LED ports
			SPIwrite(0, MAX_PORT0, b[0]);
			SPIwrite(0, MAX_PORT5, b[1]);
			SPIwrite(2, MAX_PORT0, b[2]);
			SPIwrite(0, MAX_PORT8, b[3]);
			SPIwrite(0, MAX_PORT3, b[4]);
			SPIwrite(1, MAX_PORT9, b[5]);
			SPIwrite(2, MAX_PORT4, b[6]);
			SPIwrite(2, MAX_PORT7, b[7]);
			SPIwrite(1, MAX_PORT4, b[8]);
			SPIwrite(3, MAX_PORT3, b[9]);
			SPIwrite(4, MAX_PORT9, b[10]);
			SPIwrite(3, MAX_PORT0, b[11]);
			SPIwrite(3, MAX_PORT7, b[12]);
			SPIwrite(4, MAX_PORT2, 255);
		
		end

		else
		begin

			//turn the other "half" of the canvas off
			for(int k = 0; k < 5; k++)
			begin
				for(int j = 0; j < 10; j++)
				begin
					SPIwrite(k, j, 255);
				end
			end
		end
		
		if(ledPos < 179)
			ledPos++;

		if(rpm != 0)
		begin
			spr = (1.0/rpm)/180.0;
			//update release and deadline based on calculated rpm
			rel = trtCurrentTime() + SECONDS2TICKS(.3*spr);
			dead = rel;
		end
		
		else
		begin
		
			spr = 0;
			//update release and deadline
			rel = trtCurrentTime() + SECONDS2TICKS(.01);
			dead = trtCurrentTime() + SECONDS2TICKS(.015);
		end
		trtSleepUntil(rel, dead);
	end
end

//***************************************************************
// --- Run Animations Code ----------------------------------
void task4(void* args)
begin
	uint32_t rel, dead;
	uint8_t curTime;
	uint8_t s0x,s0y,s1x,s1y,s2x,s2y,s3x,s3y,s4x,s4y,s5x,s5y,s6x,s6y,s7x,s7y,s8x,s8y,s9x,s9y,s10x,s10y;
	uint8_t dir; // direciton of motion: 0=left, 1=right, 2=up, 3=down
	dir = RIGHT;
	curTime = 0;
	s0x = 3; s1x = 5; s2x = 6; s3x = 7; s4x = 3; s5x = 5; s6x = 7; s7x = 3; s8x = 4; s9x = 5; s10x = 7;
	s0y = 76; s1y = 76; s2y = 76; s3y = 76; s4y = 77; s5y = 77; s6y = 77; s7y = 78; s8y = 78; s9y = 78; s10y = 78;
	while(1)
	begin
		if(runAnimations == 1)
		begin
			if((curTime < moveTime) && (dir == LEFT))
				dir = LEFT;
			else if((curTime == moveTime) && (dir == LEFT))
				dir = RIGHT;
			else if((curTime > 0) && (dir == RIGHT))
				dir = RIGHT;
			else if((curTime == 0) && (dir == RIGHT))
				dir = LEFT;
			if(dir == LEFT)
			begin
				curTime++;
				// clear old image
				testMat[s0x][s0y] = (uint16_t)0xffff;
				testMat[s1x][s1y] = (uint16_t)0xffff;
				testMat[s2x][s2y] = (uint16_t)0xffff;
				testMat[s3x][s3y] = (uint16_t)0xffff;
				testMat[s4x][s4y] = (uint16_t)0xffff;
				testMat[s5x][s5y] = (uint16_t)0xffff;
				testMat[s6x][s6y] = (uint16_t)0xffff;
				testMat[s7x][s7y] = (uint16_t)0xffff;
				testMat[s8x][s8y] = (uint16_t)0xffff;
				testMat[s9x][s9y] = (uint16_t)0xffff;
				testMat[s10x][s10y] = (uint16_t)0xffff;
				// update and write new image
				s0y++; s1y++; s2y++; s3y++; s4y++; s5y++; s6y++; s7y++; s8y++; s9y++; s10y++;
				testMat[s0x][s0y] = (uint16_t)0xf0f0;
				testMat[s1x][s1y] = (uint16_t)0xf0f0;
				testMat[s2x][s2y] = (uint16_t)0xf0f0;
				testMat[s3x][s3y] = (uint16_t)0xf0f0;
				testMat[s4x][s4y] = (uint16_t)0xf0f0;
				testMat[s5x][s5y] = (uint16_t)0xf0f0;
				testMat[s6x][s6y] = (uint16_t)0xf0f0;
				testMat[s7x][s7y] = (uint16_t)0xf0f0;
				testMat[s8x][s8y] = (uint16_t)0xf0f0;
				testMat[s9x][s9y] = (uint16_t)0xf0f0;
				testMat[s10x][s10y] = (uint16_t)0xf0f0;
			end
			else if(dir == RIGHT)
			begin
				curTime--;
				// clear old image
				testMat[s0x][s0y] = (uint16_t)0xffff;
				testMat[s1x][s1y] = (uint16_t)0xffff;
				testMat[s2x][s2y] = (uint16_t)0xffff;
				testMat[s3x][s3y] = (uint16_t)0xffff;
				testMat[s4x][s4y] = (uint16_t)0xffff;
				testMat[s5x][s5y] = (uint16_t)0xffff;
				testMat[s6x][s6y] = (uint16_t)0xffff;
				testMat[s7x][s7y] = (uint16_t)0xffff;
				testMat[s8x][s8y] = (uint16_t)0xffff;
				testMat[s9x][s9y] = (uint16_t)0xffff;
				testMat[s10x][s10y] = (uint16_t)0xffff;
				// update and write new image
				s0y--; s1y--; s2y--; s3y--; s4y--; s5y--; s6y--; s7y--; s8y--; s9y--; s10y--;
				testMat[s0x][s0y] = (uint16_t)0xf0f0;
				testMat[s1x][s1y] = (uint16_t)0xf0f0;
				testMat[s2x][s2y] = (uint16_t)0xf0f0;
				testMat[s3x][s3y] = (uint16_t)0xf0f0;
				testMat[s4x][s4y] = (uint16_t)0xf0f0;
				testMat[s5x][s5y] = (uint16_t)0xf0f0;
				testMat[s6x][s6y] = (uint16_t)0xf0f0;
				testMat[s7x][s7y] = (uint16_t)0xf0f0;
				testMat[s8x][s8y] = (uint16_t)0xf0f0;
				testMat[s9x][s9y] = (uint16_t)0xf0f0;
				testMat[s10x][s10y] = (uint16_t)0xf0f0;
			end
		end
		//update release and deadline
		rel = trtCurrentTime() + SECONDS2TICKS(.5);
		dead = trtCurrentTime() + SECONDS2TICKS(.55);
		trtSleepUntil(rel, dead);
	end
end

//***************************************************************
// --- Main Program ----------------------------------
int main(void) 
begin
	initTimers();
	initUART();
	//initLCD();
	// LED Port
	DDRD = 0x00;
	SPIinitialize(); 
	//init rand number generator
	srand(1234);
	led = 0;
	
	//initialize led matrix
	state = WAIT_SYNC;
	next_state = WAIT_SYNC;
	ledPos = 0;
	matSize = 90;
	for(int k = 0; k < 14; k++)
	begin
		for(int j = 0; j < matSize; j++)
		begin
			testMat[k][j] = (uint16_t)0xffff;
		end
	end
	
	//initialize for animation
	runAnimations = 0;	
	moveTime = 10;

	//initialize motor period calculation
	motor_period = 0;
	motor_period_ovlf = 0;

  // start TRT
  trtInitKernel(300); // 80 bytes for the idle task stack
  
  // message protection
  trtCreateSemaphore(SEM_TX_WAIT, 1) ; // message send interlock
  trtCreateSemaphore(SEM_RX_WAIT, 0) ; // message receive interlock

 // --- creat tasks  ----------------
  trtCreateTask(task1, 200, SECONDS2TICKS(0.8), SECONDS2TICKS(0.13), &(args[0]));
  trtCreateTask(task3, 200, SECONDS2TICKS(0.5), SECONDS2TICKS(0.1), &(args[1]));
  trtCreateTask(task4, 200, SECONDS2TICKS(0.1), SECONDS2TICKS(0.15), &(args[1]));

  // --- Idle task --------------------------------------
  // For debugging, blink an LED
  // For production, you would probably comment out the body of the WHILE
  while (1) 
  begin

  end

end // main

//***************************************************************
// --- Initialize Timer for INT0 ----------------------------------
void initTimers(void) 
begin
	//set up INT0
	EIMSK = 1<<INT0 ; // turn on int0
	EICRA = 3 ;       // rising edge
	// turn on timer 2 to be read in int0 ISR	
	TCCR2B = 7 ; // divide by 1024
	// turn on timer 2 overflow ISR for double precision time
	TIMSK2 = 1 ;
end

//***************************************************************
// --- Initialize LCD ----------------------------------
void initLCD(void)
begin
	//initialize LCD
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
end

//***************************************************************
// --- Initialize UART COMM ----------------------------------
void initUART(void)
begin
	state=WAIT_SYNC;
	next_state = WAIT_SYNC;
	
	//USART Initialization
	UBRR0H = (uint8_t) (UBRRVAL>>8);  // high byte
	UBRR0L = (uint8_t) UBRRVAL;   // low byte        
	UCSR0B =  (1<<RXEN0)| (1 << RXCIE0) ;  // enable Receiver	
	// data frame format, asynch mode, no parity, 1 stop bit, 8 bit size
    UCSR0C = (0<<UMSEL00) | (0<<UMSEL01) | (0<<UPM00) | (0<<UPM01) | (0<<USBS0) | (3<<UCSZ00) ;   
	
	//Initializing Varibles for state machine and data decoding
	SET_COL = 0;
	col=0;
	row=0;	
	nible_1 = 0;
	nible_2 = 0;
	nible_3 = 0;
	nible_4 = 0;
	col_data = 0;
	data = 0;
end

//***************************************************************
// --- Initialize SPI ----------------------------------
void SPIinitialize(void)
begin

	//set up i/o data direction
	//DDRB.0 = 1; //output chip select for MAX0
    //DDRB.1 = 1; //output chip select for MAX1 
	//DDRB.2 = 1; //output chip select for MAX2
    //DDRB.3 = 1; //output chip select for MAX3 
	//DDRB.4 = 1; //output chip select for MAX4 
    //DDRB.5 = 1; //output MOSI to ADC
    //DDRB.6 = 0; //input MISO from ADC
    //DDRB.7 = 1; //output SCLK   
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2) | (1<<DDB3) | (1<<DDB4) | (1<<DDB5) | (1<<DDB7) ; 
	
	// initialize notSYNC
	SET(PORTB,0) ;
	SET(PORTB,1) ;
	SET(PORTB,2) ;
	SET(PORTB,3) ;
	SET(PORTB,4) ;

	//set up SPI
    //bit 7 SPIE=0 no ISR
    //bit 6 SPE=1 enable spi
    //bit 5 DORD=0 msb first 
    //bit 4 MSTR=1 Mega32 is spi master
    //bit 3 CPOL=0 clock polarity (0 baseline)
    //bit 2 CPHA=0 clock phase (pos edge)
    //bit 1,0 rate sel=10 along with SPRC=1 sets clk to f/32 = 500 kHz
    SPCR = (1<<SPE) | (1<<MSTR); //SPCR SPCR = 0b01011110 ;                                                                               
    SPSR = (1<<SPI2X) ; //SPSR = 1;

	// FOR EACH MAX CHIP

	for (int j = 0; j < 5; j++)
	begin

		//FOR EACH OUTPUT PIN ON MAX CHIP J
		for (int k = 0; k < 10; k++)
		begin
			SPIwrite(j, k, pin_high);
		end

		//CONFIG REGISTERS
		SPIwrite(j, 0x15, 0x03);		// Max Current Sink
		SPIwrite(j, 0x13, 0b11111111);	//PORT 0-7 set to full current
	
		//Configuration Register
		//D7 = 0, DOUT = MISO, Internal Oscilator drives PWM
		//D6 = 0, Don't Care
		//D5 = 1, PWM Stagger = in phase
		//D4 = 0, Run Mode
		//D3 = 0, Run Mode
		//D2 = 0, Ramp up Enable
		//D1 = 1, CS Run
		//D0 = 1, Run Mode
		SPIwrite(j, 0x10, 0b00010001);
	end

end

//***************************************************************
// --- Write to SPI bus ----------------------------------
void SPIwrite(int index, uint8_t address, uint8_t data)
begin

uint8_t junk;
	
	//chip select low begins conversion 
	CLR(PORTB,index);   

	//send address
	SPDR = address;
 
	//wait until SPIF is set (SPI done)
    while (!(SPSR & (1<<SPIF))) ;
		junk = SPDR ; // get back junk		  

        
	// Send SPI data
    SPDR = data ; 

	// wait until SPIF is set (SPI done)
    while (!(SPSR & (1<<SPIF))) ;
		junk = SPDR; // read back values from MAX6966


	//chip select high ends conversion
    SET(PORTB,index);  

end




