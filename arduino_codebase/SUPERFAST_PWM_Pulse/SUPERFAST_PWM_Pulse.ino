  
  // A sketch that creates an 8MHz, 50% duty cycle PWM and a 250KHz,
  // 6bit resolution PWM with varying duty cycle (changes every 5μs
  // or about every period.
  
  #include <avr/io.h>
  #include <util/delay.h>
  
  //DDRD &= ~(1 << n); // Pin n is input
  //DDRD |= (1 << n); // Pin n is output
  //PORTD &= ~(1 << n); // Pin n goes low
  //PORTD |= (1 << n); // Pin n goes high
  
  // Duty Cycle OUTPUT MAP
  // 30 -> ~.~~V
  // 25 -> 0.32V
  // 20 -> 0.64V
  // 15 -> 1.00V
  // 10 -> 1.30V
  // 5  -> 1.60V
  // 0  -> 1.90V
  int duty_cycle = 15;  // Range from 0 -> (OCR2A+1)/2
  volatile unsigned int freq = 300;    // Frequency [Hertz]
  
  int main(void)
  {
    // set a timer of length 100000 microseconds 
    // (or 0.1 sec - or 10Hz => the event will fire 
    // 5 times, 5 cycles of on-and-off, per second)
    //Timer1.initialize( 30000 ); 
    //Timer1.attachInterrupt( timerIsr ); // attach the service routine here
    
    
    // output pin for OCR2B, this is Arduino pin number
    pinMode(3, OUTPUT); 
  
    // External Trigger Pin (UNUSED)
    pinMode(4, OUTPUT); // ext trigger pin
    
    // Pin 13 has an LED connected on most Arduino boards
    pinMode(13, OUTPUT); 
  
    // stop interrupts
    cli();
  
    // Timer 2 Setup
    // In the next line of code, we:
    // 1. Set the compare output mode to clear OC2A and OC2B on compare match.
    //    To achieve this, we set bits COM2A1 and COM2B1 to high.
    // 2. Set the waveform generation mode to fast PWM (mode 3 in datasheet).
    //    To achieve this, we set bits WGM21 and WGM20 to high.
    TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  
    // In the next line of code, we:
    // 1. Set the waveform generation mode to fast PWM mode 7 —reset counter on
    //    OCR2A value instead of the default 255. To achieve this, we set bit
    //    WGM22 to high.
    // 2. Set the prescaler divisor to 1, so that our counter will be fed with
    //    the clock's full frequency (16MHz). To achieve this, we set CS20 to
    //    high (and keep CS21 and CS22 to low by not setting them).
    TCCR2B = _BV(WGM22) | _BV(CS20);
  
    // OCR2A holds the top value of our counter, so it acts as a divisor to the
    // clock. When our counter reaches this, it resets. Counting starts from 0.
    // Thus 63 equals to 64 divs.
    OCR2A = 64-1; 
    
    // This is the duty cycle. Think of it as the last value of the counter our
    // output will remain high for. Can't be greater than OCR2A of course. A
    // value of 0 means a duty cycle of 1/64 in this case.
    OCR2B = (OCR2A+1)/2;
      
    
    // Set Timer1 interrupt at 1Hz
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCNT1  = 0; // initialize counter value to 0
        
    // set WGM12 to turn on CTC mode
    // Set CS12 and CS10 bits for 1024 prescaler
    // Set CS12 bit for 256 prescaler
    // Set CS11 and CS10 bits for 64 prescaler
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
    
    // set compare match register for 1000hz (4ms) increments
    // OCR1A = Frq of Crystal / Prescaler / Desired Frq - 1  
    // 249   = (16 * 10^6) / (64 * 1000) - 1   (must be < 65536)
    OCR1A = 250 * (1000 / freq) - 1;
    
    // enable timer compare interrupt
    TIMSK1 |= _BV(OCIE1A);
      
    // allow interrupts
    sei();
    
    
    while (1)
    {
      // loop
    }
  }
  
  
  /// --------------------------
  /// Custom ISR Timer Routine
  /// --------------------------
  ISR(TIMER1_COMPA_vect)
  {
    //PORTD ^= (1 << 4); // ext trigger toggle
    
    // output pin for OCR2B, this is Arduino pin number
    DDRD |= (1 << 3); // Pin n is output
    
    // ext trigger pin high
    //PORTD |= (1 << 4); 
    
    OCR2B = duty_cycle;
    _delay_us(200);
        
    OCR2B = OCR2A - duty_cycle;
    _delay_us(200);
    
    // output pin for OCR2B, this is Arduino pin number
    DDRD &= ~(1 << 3); // Pin n is input
    
    // ext trigger pin low
    //PORTD &= ~(1 << 4); 
  }
  
  
  
  
  
  
  
  
