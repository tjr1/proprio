
// **********************************************
// INCLUDE LIBRARIES
// LINUX: /usr/share/arduino/libraries/
// WINDOWS: ../My Documents\Arduino\libraries
// **********************************************

  #include "TimerOne.h"  
  #include <avr/io.h>
  
// **********************************************
// PIN DEFINITIONS 
// **********************************************
  
  //int pinDigital_UART_TX         = 0;
  //int pinDigital_UART_RX         = 1;  
  int pinDigital_CueLED_L        = 2;
  //int pinDigital_PWM_A           = 3;   // UNUSED: Motor Shield Pins
  int pinAnalog_StimChan1        = 3;
  int pinDigital_CapSen_Handle   = 4;
  int pinDigital_CapSen_Sipper   = 5;
  //int pinDigital_X               = 6;
  int pinDigital_CueLED_R        = 7;
  int pinDigital_BRAKE_B         = 8;   // Motor Shield Pins
  int pinDigital_PushButtonGND   = 9;
  //int pinDigital_BRAKE_A         = 9;   // UNUSED: Motor Shield Pins
  int pinDigital_PushButton      = 10;
  int pinDigital_PWM_B           = 11;  // Motor Shield Pins
  //int pinDigital_DIR_A           = 12;  // UNUSED: Motor Shield Pins
  int pinDigital_DIR_B           = 13;  // Motor Shield Pins
  
  //int pinAnalog_NC = A0;
  //int pinAnalog_NC = A1;
  //int pinAnalog_NC = A2;
  //int pinAnalog_NC = A3;
  int pinAnalog_JoystickY = A4;
  int pinAnalog_JoystickX = A5;
  
// **********************************************
// Variables    
// **********************************************

  boolean started = false;
  
// **********************************************
// Functions 
// **********************************************
  
  void setup() 
  { 
    // Initalize Serial Connection
    Serial.begin(9600);
    
    // Initalize Stim Channel
    pinMode(pinAnalog_StimChan1, INPUT);  
    
    //DDRD &= ~(1 << n); // Pin n is input
    //DDRD |= (1 << n); // Pin n is output
    //PORTD &= ~(1 << n); // Pin n goes low
    //PORTD |= (1 << n); // Pin n goes high
    
    // set a timer of length 100000 microseconds 
    // (or 0.1 sec - or 10Hz => the led will blink 5 times, 
    // 5 cycles of on-and-off, per second)
    //Timer1.initialize(1); 
    //Timer1.attachInterrupt( timerIsr );
  }
  
  /// --------------------------
  /// Custom ISR Timer Routine
  /// --------------------------
  //void timerIsr()
  //{
  //  // Toggle LED
  //  digitalWrite( 13, digitalRead( 13 ) ^ 1 );
  //}
  
  void loop() 
  {
    // Initalize Values
    long start = millis();
         
    // Read From Serial Port and Mirror 
    if (Serial.available() > 0) {
      String rxSerialString = Serial.readStringUntil('\n');
      Serial.println(rxSerialString);
    }  
    
      
    
    int pulse_dur = 200;
    double ideal_dt_a = 0;
    int ideal_dt_z = 10;    
    int actual_dt_a = 5;
    int actual_dt_z = ideal_dt_z + 5;  // technically 4.60 or 4.80
    
    int counter = 0;
    int N = pulse_dur/(actual_dt_a + actual_dt_z);
    
    //Rising Pulse
    pinMode(pinAnalog_StimChan1, OUTPUT);
    
    while (counter < N)
    {      
      digitalWrite(pinAnalog_StimChan1, HIGH);
      //delayMicroseconds(ideal_dt_a);
      digitalWrite(pinAnalog_StimChan1, 0);
      delayMicroseconds(ideal_dt_z);
      counter += 1;
    }
    
    //Falling Pulse
    counter = 0;
    while (counter < N)
    {
      digitalWrite(pinAnalog_StimChan1, HIGH);
      delayMicroseconds(ideal_dt_z);
      digitalWrite(pinAnalog_StimChan1, 0);
      //delayMicroseconds(ideal_dt_a);
      counter += 1;
    }
    
    pinMode(pinAnalog_StimChan1, INPUT);
        
    
    // Maintain 100 ms loop duration
    long time_elapsed = millis() - start;
    if (time_elapsed < 100) {
      delay(100 - time_elapsed);  
    }      
  }
  
// **********************************************
// Helper Functions 
// **********************************************
    

