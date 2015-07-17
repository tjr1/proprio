/* 
 * Arduino 101: timer and interrupts
 * 3: Timer2 compare interrupt example. Quadrature Encoder
 * more infos: http://www.letmakerobots.com/node/28278
 * created by RobotFreak 
 *
 * Credits:
 * based on code from Peter Dannegger
 * http://www.mikrocontroller.net/articles/Drehgeber
 */


#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WConstants.h"
#endif

// Encoder Pins
#define encLtA 2
#define encLtB 3
#define encRtA 11 
#define encRtB 12
#define ledPin 13

#define LT_PHASE_A		digitalRead(encLtA)
#define LT_PHASE_B		digitalRead(encLtB)
#define RT_PHASE_A		digitalRead(encRtA)
#define RT_PHASE_B		digitalRead(encRtB)

static volatile int8_t encDeltaLt, encDeltaRt;
static int8_t lastLt, lastRt;
int encLt, encRt;

ISR( TIMER2_COMPA_vect )
{
  int8_t val, diff;

  digitalWrite(ledPin, HIGH);   // toggle LED pin
  val = 0;
  if( LT_PHASE_A )
    val = 3;
  if( LT_PHASE_B )
    val ^= 1;					// convert gray to binary
  diff = lastLt - val;				// difference last - new
  if( diff & 1 ){				// bit 0 = value (1)
    lastLt = val;				// store new as next last
    encDeltaLt += (diff & 2) - 1;		// bit 1 = direction (+/-)
  }

  val = 0;
  if( RT_PHASE_A )
    val = 3;
  if( RT_PHASE_B )
    val ^= 1;					// convert gray to binary
  diff = lastRt - val;				// difference last - new
  if( diff & 1 ){				// bit 0 = value (1)
    lastRt = val;				// store new as next last
    encDeltaRt += (diff & 2) - 1;		// bit 1 = direction (+/-)
  }
  digitalWrite(ledPin, LOW);   // toggle LED pin
}


void QuadratureEncoderInit(void)
{
  int8_t val;

  cli();
  TIMSK2 |= (1<<OCIE2A);
  sei();
  pinMode(encLtA, INPUT);
  pinMode(encRtA, INPUT);
  pinMode(encLtB, INPUT);
  pinMode(encRtB, INPUT);

  val=0;
  if (LT_PHASE_A)
    val = 3;
  if (LT_PHASE_B)
    val ^= 1;
  lastLt = val;
  encDeltaLt = 0;

  val=0;
  if (RT_PHASE_A)
    val = 3;
  if (RT_PHASE_B)
    val ^= 1;
  lastRt = val;
  encDeltaRt = 0;

  encLt = 0;
  encRt = 0;
}

int8_t QuadratureEncoderReadLt( void )			// read single step encoders
{
  int8_t val;

  cli();
  val = encDeltaLt;
  encDeltaLt = 0;
  sei();
  return val;					// counts since last call
}

int8_t QuadratureEncoderReadRt( void )			// read single step encoders
{
  int8_t val;

  cli();
  val = encDeltaRt;
  encDeltaRt = 0;
  sei();
  return val;					// counts since last call
}

void setup()
{
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  QuadratureEncoderInit();
}


void loop()
{
  encLt += QuadratureEncoderReadLt();
  encRt += QuadratureEncoderReadRt();
  Serial.print("Lt: ");
  Serial.print(encLt, DEC);
  Serial.print(" Rt: ");
  Serial.println(encRt, DEC);
  delay(1000);
}


