#include <MotorShield.h>
#include <CapacitiveSensor.h>

// **********************************************
// PIN DEFINITIONS 
//int pinCapSenCommon = 4;
//int pinCapSen1 = 5;
//int pinCapSen2 = 6;
int pinCueLED_L = 2;
int pinCueLED_R = 7;
int pinPushButtonGND = 9;
int pinPushButton = 10;
int pinCapSenBOOL_HANDLE = 12;
int pinCapSenBOOL_SIPPER = 6;


// **********************************************
// Other DEFINITIONS 
MS_DCMotor motor(MOTOR_B);

// 10M resistor between pins 4 & 2, pin 2 is sensor pin
// add a wire and or foil if desired
//CapacitiveSensor   cs_4_2 = CapacitiveSensor(pinCapSenCommon,pinCapSen1);        
//CapacitiveSensor   cs_4_6 = CapacitiveSensor(pinCapSenCommon,pinCapSen2);     

int counter1 = 0;
int counter2 = 0;
int counter3 = 0;
boolean BUTTON_EVNT = false;
boolean DISPENSE = false;
String incomingSerialCom = "";
boolean targetLeft = false;

void setup() {
  // engage the motor's brake 
  motor.run(BRAKE);
  motor.setSpeed(255);
  Serial.begin(9600);
  
  pinMode(pinPushButton, INPUT_PULLUP);
  pinMode(pinPushButtonGND, OUTPUT);
  digitalWrite(pinPushButtonGND, LOW);
  
  pinMode(pinCueLED_R, OUTPUT);  
  pinMode(pinCueLED_L, OUTPUT); 
  
  // Initalize CapSen
  //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  //cs_4_6.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

  pinMode(pinCapSenBOOL_HANDLE, INPUT);
  pinMode(pinCapSenBOOL_SIPPER, INPUT);
}

void loop() {
  
  // **********************************************
  // Initalize Values
  long start = millis();
  counter2 = counter2 + 1;
  DISPENSE = false;
  
  // **********************************************
  // Read From Serial Port
  if (Serial.available() > 0) {
    incomingSerialCom = Serial.readStringUntil('\n');
    //Serial.print("\"" + incomingSerialCom + "\"\n");
    
    DISPENSE = DISPENSE | incomingSerialCom.equals("H_L") | incomingSerialCom.equals("H_R");
    targetLeft = incomingSerialCom.equals("H_L");
  }  
  
  // **********************************************
  // Read From Button
  if (digitalRead(pinPushButton) == LOW) {
    BUTTON_EVNT = true;
  }
  else {
    BUTTON_EVNT = false;
  }  
  DISPENSE = DISPENSE | BUTTON_EVNT;
    
  // **********************************************
  // Read From Digital Cap Sensor
  boolean capSenBOOL_SIPPER = digitalRead(pinCapSenBOOL_SIPPER) == HIGH;
  boolean capSenBOOL_HANDLE = digitalRead(pinCapSenBOOL_HANDLE) == HIGH;
  
  if (true)
  {
    if (capSenBOOL_SIPPER)
    {
      if (counter3 <= 0)
      {
        counter3 = 30; 
        DISPENSE = DISPENSE | capSenBOOL_SIPPER;    
      }
      else
        counter3 = counter3 - 1; 
    }
  }
  
  if (false)
  {
    if (counter3 <= 0)
    { 
      if (capSenBOOL_SIPPER)
        counter3 = 10;
        
      DISPENSE = DISPENSE | capSenBOOL_SIPPER;    
    }
    else
      counter3 = counter3 - 1;
  }
  
  // **********************************************
  // Read Analog Capacitive Sensors
  //long total1 = cs_4_2.capacitiveSensor(30);
  //long total2 = cs_4_6.capacitiveSensor(30); 
    
  // **********************************************
  // Dispense Liquid Reward 
  if (DISPENSE)  {
    Serial.print("DISPENSING...\n");
    if (targetLeft) digitalWrite(pinCueLED_L, HIGH);
    if (!targetLeft) digitalWrite(pinCueLED_R, HIGH);
     
    counter1 = counter1 + 1;
    
    if (counter1 > 9) {
      sendPulse(255, 1000);
    }
    else if (counter1 == 1) 
    {
      // 20 drops at this flow rate equals 1 mL 
      // Each drop is approx. 0.05 mL
      sendPulse(175, 100);  
    }
  }
  else 
  {
    counter1 = 0;
    digitalWrite(pinCueLED_L, LOW);
    digitalWrite(pinCueLED_R, LOW);
  }
    
  // **********************************************
  // Print To Serial Port
  Serial.print(BUTTON_EVNT | DISPENSE);             // Button Event
  Serial.print(",");
  Serial.print(capSenBOOL_SIPPER);                  // print sensor output 1
  Serial.print(",");
  Serial.print(capSenBOOL_HANDLE);                  // print sensor output 2
  Serial.print(",\n"); 
  
  // **********************************************
  // Maintain 100 ms loop duration
  long time_elapsed = millis() - start;
  if (time_elapsed < 100) {
    delay(100 - time_elapsed);  
  }  
}


// helper function to send pulse to motor
void sendPulse(int motorSpeed, int pulseDur) {
  motor.run(BRAKE);
  motor.setSpeed(motorSpeed);
  
  // set direction to forward and release the brake in a single call
  motor.run(BACKWARD|RELEASE);
  delay(pulseDur);
  
  // engage brake
  motor.run(BRAKE);
  
  // turn off speed and engage brake
  motor.setSpeed(0);
  motor.run(RELEASE);
}

// helper function to print the motor's states in human-readable strings.
String decodeState(int state) {
  String result = "";
  switch (state) {
    case FORWARD:
      result = "Forward";
      break;
    case BACKWARD:
      result = "Backward";
      break;
    case BRAKE:
     result = "Brake";
     break;
   case RELEASE:
     result = "Release";
     break;
   }
  return result;
}
