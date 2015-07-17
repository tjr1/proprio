
// **********************************************
// PIN DEFINITIONS 

// **********************************************
// Other DEFINITIONS 

int counter1 = 0;
int x_axis = 0;
int y_axis = 0;
String incomingSerialCom = "";

void setup() {
  Serial.begin(9600);
  
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
}

void loop() {
  
  // Initalize Values
  long start = millis();
    
  // Read From Serial Port
  if (Serial.available() > 0) {
    incomingSerialCom = Serial.readStringUntil('\n');
    Serial.print("\"" + incomingSerialCom + "\"\n");
  }  
  
  // Read From Joystick
  x_axis = analogRead(A1);
  y_axis = analogRead(A0);   
    
  // Print To Serial Port
  Serial.print(x_axis);
  Serial.print(",");
  Serial.print(y_axis);
  Serial.print(",\n"); 
  
  // Maintain 100 ms loop duration
  long time_elapsed = millis() - start;
  if (time_elapsed < 100) {
    delay(100 - time_elapsed);  
  }  
}
