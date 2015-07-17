
// **********************************************
// INCLUDE LIBRARIES
// LINUX: /usr/share/arduino/libraries/
// WINDOWS: ../My Documents\Arduino\libraries
// **********************************************

  #include <MotorShield.h>  
  
// **********************************************
// PIN DEFINITIONS 
// **********************************************
  
  //int pinDigital_UART_TX         = 0;
  //int pinDigital_UART_RX         = 1;  
  int pinDigital_CueLED_L        = 2;
  //int pinDigital_PWM_A           = 3;   // UNUSED: Motor Shield Pins
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
  
  int pinAnalog_JoystickY = 4;
  int pinAnalog_JoystickX = 5;
  
// **********************************************
// Variables    
// **********************************************
  
  int GBL_counter_dispense = 0;
  int GBL_counter_reward = 0;
  
  boolean GBL_handle_reset_flag = true;
  boolean GBL_verbose = false;
    
  boolean GBL_subjectTouchingSipper = false;
  boolean GBL_subjectTouchingHandle = false;
    
  long GBL_x_axis = 0;
  long GBL_y_axis = 0;
  long GBL_x_axis_home = 0;
  long GBL_y_axis_home = 0;
  
  MS_DCMotor motor(MOTOR_B);
  
// **********************************************
// Functions 
// **********************************************
  
  void setup() 
  { 
    // Initalize Serial Connection
    Serial.begin(9600);
    
    // Initalize Joystick
    pinMode(pinAnalog_JoystickX, INPUT);
    pinMode(pinAnalog_JoystickY, INPUT);
    
    // Read From Joystick
    GBL_x_axis_home = analogRead(pinAnalog_JoystickX);
    GBL_y_axis_home = analogRead(pinAnalog_JoystickY); 
    
    // Initialize Manual Reward Trigger
    pinMode(pinDigital_PushButton, INPUT_PULLUP);
    pinMode(pinDigital_PushButtonGND, OUTPUT);
    digitalWrite(pinDigital_PushButtonGND, LOW);
    
    // Initalize LEDs
    pinMode(pinDigital_CueLED_L, OUTPUT);  
    pinMode(pinDigital_CueLED_R, OUTPUT); 
    
    // Initalize Capacitive Sensor Inputs
    pinMode(pinDigital_CapSen_Handle, INPUT);
    pinMode(pinDigital_CapSen_Sipper, INPUT);
    
    // Initalize Pump Motor 
    motor.run(BRAKE);
    motor.setSpeed(255);
  }
  
  void loop() 
  {
    // Initalize Values
    long start = millis();
      
    // Read From Serial Port and Mirror 
    if (Serial.available() > 0) {
      String rxSerialString = Serial.readStringUntil('\n');
      parseRXSerial(rxSerialString);
    }  
    
    // Read From Joystick
    GBL_x_axis = analogRead(pinAnalog_JoystickX) - GBL_x_axis_home;
    GBL_y_axis = analogRead(pinAnalog_JoystickY) - GBL_y_axis_home;  
   
    // Read From Capacitive Sensors
    GBL_subjectTouchingSipper = digitalRead(pinDigital_CapSen_Sipper) == HIGH;
    GBL_subjectTouchingHandle = digitalRead(pinDigital_CapSen_Handle) == HIGH; 
      
    // Read from Button
    boolean button_pressed = digitalRead(pinDigital_PushButton) == LOW;
    
    
    // **********************************************
    // Reward?
    // **********************************************
    boolean DISPENSE = false; 
    //DISPENSE = DISPENSE | rewardForTouchingHandle(100, subjectTouchingHandle);  //ms
    //DISPENSE = DISPENSE | rewardForTouchingSipper(100, subjectTouchingSipper);  //ms
    
    // Lock Josystick Rewards?
    boolean unlocked = joystickUnlocked_Sipper(500, GBL_subjectTouchingSipper) | 
                       joystickUnlocked_Handle(500, GBL_subjectTouchingHandle);
    DISPENSE = DISPENSE & unlocked;

    // Mx distance is about 550, Min distance is 0.
    //DISPENSE = DISPENSE | rewardForMovingHandle(300);    // pixels
    //DISPENSE = DISPENSE | rewardForMovingHandleEveryOtherSide(300);    // pixels
    DISPENSE = DISPENSE | rewardForMovingHandleBothSides(300, unlocked);

    // Manual Reward
    DISPENSE = DISPENSE | button_pressed;
    
    
    // **********************************************
    // Liquid Reward
    // **********************************************
    if (DISPENSE) 
    {
      GBL_counter_reward = GBL_counter_reward + 1;
      if (GBL_verbose)
        Serial.print("Reward #" + String(GBL_counter_reward, DEC) + " \n");

      digitalWrite(pinDigital_CueLED_L, HIGH);
      digitalWrite(pinDigital_CueLED_R, HIGH);
      
      GBL_counter_dispense = GBL_counter_dispense + 1;
      if (GBL_counter_dispense > 9) 
        sendPulse(255, 1000);
      else if (GBL_counter_dispense == 1) 
        sendPulse(175, 100);  
        // 20 drops at this flow rate equals 1 mL 
        // Each drop is approx. 0.05 mL   
        
      // Joystick Reset Flag
      GBL_handle_reset_flag = false;
    }
    
    else if (DISPENSE == false)
    {
      GBL_counter_dispense = 0;
      digitalWrite(pinDigital_CueLED_L, LOW);
      digitalWrite(pinDigital_CueLED_R, LOW);
    }
    
    // Print To Serial Port
    if (GBL_verbose)  
      Serial.println(verboseSerialOutput());
    
    
    // Maintain 100 ms loop duration
    long time_elapsed = millis() - start;
    if (time_elapsed < 100) {
      delay(100 - time_elapsed);  
    }      
  }
  
// **********************************************
// Helper Functions 
// **********************************************
    
  void parseRXSerial(String rxSerialString)
  {
    // Echo Serial Command 
    Serial.println("\"" + rxSerialString + "\"");
          
    String name = "";
    int value = 0;
 
    for (int i = 0; i < rxSerialString.length(); i++) 
      if (rxSerialString.substring(i, i+1) == ",") 
      {
        name = rxSerialString.substring(0, i);
        value = rxSerialString.substring(i+1).toInt();       
        break;
      }
        
    if (name.equals("verbose"))
      GBL_verbose = value; 
   
    else if (name.equals("identity"))
      Serial.println("Arduino Remote Trainer");
      
    else if (name.equals("all"))
      Serial.println(verboseSerialOutput());
      
    else if (name.equals("data_list"))
      Serial.println("GBL_counter_reward, GBL_x_axis, GBL_y_axis, GBL_subjectTouchingHandle, GBL_subjectTouchingSipper, handleDistance");
  }
  
  String verboseSerialOutput()
  {
    long handleDistance = sqrt(GBL_x_axis*GBL_x_axis + GBL_y_axis*GBL_y_axis);
    String output = "R#:" + String(GBL_counter_reward, DEC) + ", " +
                    String(GBL_x_axis, DEC) + ", " +
                    String(GBL_y_axis, DEC) + ", " +
                    String(GBL_subjectTouchingHandle, BIN) + ", " +
                    String(GBL_subjectTouchingSipper, BIN) + ", " +
                    String(handleDistance, DEC) + ",";
    return output;
  }
  
  // Reward Function + Global Variable --------------------------
  boolean pulled_left = false;
  boolean pulled_right = false;
  boolean rewardForMovingHandleBothSides(long distance, boolean unlocked)
  {    
    long c = sqrt(GBL_x_axis*GBL_x_axis + GBL_y_axis*GBL_y_axis);
    
    if (GBL_handle_reset_flag == true && c > distance && unlocked)
    {
      if (GBL_x_axis > 0)
        pulled_left = true;
      
      if (GBL_x_axis < 0)
        pulled_right = true;
        
      if (pulled_left && pulled_right)
        return true;
    }

    else if (GBL_handle_reset_flag == false && c < 25)
    {
      pulled_left = false;
      pulled_right = false;
      GBL_handle_reset_flag = true;
    }
    
    return false;
  }
  
  // Reward Function + Global Variable --------------------------
  boolean dir_last_pull_is_left = true;
  boolean rewardForMovingHandleEveryOtherSide(long distance)
  {    
    long c = sqrt(GBL_x_axis*GBL_x_axis + GBL_y_axis*GBL_y_axis);
    
    if (GBL_handle_reset_flag == true && c > distance)
    {
      if (dir_last_pull_is_left == true && GBL_x_axis > 0)
      {
        dir_last_pull_is_left = false;
        return true;
      }
      else if (dir_last_pull_is_left == false && GBL_x_axis <= 0)
      {
        dir_last_pull_is_left = true;
        return true;
      }
    }

    else if (GBL_handle_reset_flag == false && c < 25)
      GBL_handle_reset_flag = true;

    return false;
  }
  
  // Reward Function + Global Variable --------------------------
  boolean rewardForMovingHandle(long distance)
  {    
    long c = sqrt(GBL_x_axis*GBL_x_axis + GBL_y_axis*GBL_y_axis);
    
    if (GBL_handle_reset_flag == true && c > distance)
      return true;

    else if (GBL_handle_reset_flag == false && c < 25)
      GBL_handle_reset_flag = true;

    return false;
  }

  // Reward Function + Global Variable --------------------------
  long counter_sipperlock = 0;
  boolean joystickUnlocked_Sipper(long duration, boolean subjectTouchingSipper)
  {
    if (subjectTouchingSipper == true)
      counter_sipperlock = millis();
      
    long time_elapsed = millis() - counter_sipperlock;
    return (time_elapsed <= duration);       
  }
  
  // Reward Function + Global Variable --------------------------
  long counter_handlelock = 0; 
  boolean joystickUnlocked_Handle(long duration, boolean subjectTouchingHandle)
  {
    if (subjectTouchingHandle == true)
      counter_handlelock = millis();
      
    long time_elapsed = millis() - counter_handlelock;
    return (time_elapsed <= duration);       
  }
  
  // Reward Function + Global Variable --------------------------
  long counter_handle = 0;
  boolean rewardForTouchingHandle(long duration, boolean subjectTouchingHandle)
  {
    if (subjectTouchingHandle == true)
    {
      long time_elapsed = millis() - counter_handle;
      if (time_elapsed >= duration) 
      {
        counter_handle = millis() + 1000;
        return true;  
      }
    }
    else
      counter_handle = max(counter_handle, millis());

    return false;
  }
  
  // Reward Function + Global Variable --------------------------
  long counter_sipper = 0;     
  boolean rewardForTouchingSipper(long duration, boolean subjectTouchingSipper)
  {
    if (subjectTouchingSipper == true)
    {
      long time_elapsed = millis() - counter_sipper;
      if (time_elapsed >= duration) 
      {
        counter_sipper = millis() + 1000;
        return true;  
      }
    }
    else
      counter_sipper = max(counter_sipper, millis());

    return false;
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
