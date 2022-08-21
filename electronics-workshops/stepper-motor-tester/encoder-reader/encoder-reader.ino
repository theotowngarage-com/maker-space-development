#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Encoder.h>

String inputString = ""; // a string to hold incoming data
bool stringComplete = false;

// Change these pin numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder encoderInput(2, 3);

// The X Stepper pins
#define STEPPER1_EN_PIN 8
#define STEPPER1_DIR_PIN 7
#define STEPPER1_STEP_PIN 9
// The Y stepper pins
#define STEPPER2_EN_PIN 5
#define STEPPER2_DIR_PIN 4
#define STEPPER2_STEP_PIN 6
// Define some steppers and the pins the will use
AccelStepper stepper1(AccelStepper::DRIVER, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);
MultiStepper steppers;

void setup() {
  Serial.begin(115200);
  Serial.println("Stepper motor and encoder tester");

  // configure stepper motors
  pinMode (STEPPER1_EN_PIN,OUTPUT);
  pinMode (STEPPER2_EN_PIN,OUTPUT);
  digitalWrite (STEPPER1_EN_PIN, LOW);
  digitalWrite (STEPPER2_EN_PIN, LOW);
  
  stepper1.setMaxSpeed(10000.0);
  stepper1.setAcceleration(500000.0);
  
  stepper2.setMaxSpeed(10000.0);
  stepper2.setAcceleration(500000.0);

  steppers.addStepper(stepper1);
  steppers.addStepper(stepper2);
  stepper1.setSpeed(0);
  stepper2.setSpeed(0);
}

bool useEncoder = true;
long lastPosition  = -999;

void loop() {
  long newValue;
  newValue = encoderInput.read();
  if (newValue != lastPosition) {
    if(useEncoder) {
      stepper1.setSpeed(newValue);
      stepper2.setSpeed(newValue);
//      Serial.print("Set speed = ");
//      Serial.print(newValue);
//      Serial.println();
    }
    lastPosition = newValue;
  }
  // if a character is sent from the serial monitor,
  // reset both back to zero.
  serialEvent(); 
  if (stringComplete) 
  {
    if (inputString.charAt(0)=='h'){
      Help();
    }
    else if (inputString=="xon\n"){
      ENAXON();
    }   
    else if (inputString=="xoff\n"){
      ENAXOFF();
      useEncoder = false;
    }
    else if (inputString=="stop\n"){
      stepper1.stop();
      stepper2.stop();
      inputString = "";
    }
    else if (inputString.charAt(0)=='s'){
      useEncoder = true;
      MSpeed();
    }
    else if (inputString.charAt(0)=='x'){
      useEncoder = false;
      NextX();
    } 
    
    if (inputString != "") {
      Serial.println("BAD COMMAND=" + inputString);
    }
    inputString = ""; 
    stringComplete = false;
  }
  
  stepper1.runSpeed();
  stepper2.runSpeed();
}

// ********************************************************      Serial in
void serialEvent()
{ 
  if (Serial.available()) {
    char inChar = (char)Serial.read();            // get the new byte:
    if (inChar > 0)     {inputString += inChar;}  // add it to the inputString:
    if (inChar == '\n') { stringComplete = true;} // if the incoming character is a newline, set a flag so the main loop can do something about it: 
  }
}

void Help(){ // **************************************************************   Help
  Serial.println("Commands step by step guide");
  Serial.println("Type xon  -turns TB6600 on");
  Serial.println("Type x+Number(0-60000) eg x1904 -to set next move steps");
  Serial.println("Type mx -to make motor move to next postion");
  Serial.println("Type cdon -turns on postion data when moving will increase time of step");
  Serial.println("Type x0");
  Serial.println("Type mx");
  Serial.println("Type s+Number(0-2000) eg s500 -to set Microseconds betwean steps");
  Serial.println("Type s2000");
  Serial.println("Type x300");
  Serial.println("Type mx");
  Serial.println("Type xoff -turns TB6600 off");
  Serial.println("Type cdoff -turns off postion data when moving");
  
  inputString="";
}

void ENAXON(){ // *************************************************************   ENAXON
  Serial.println("Enable drivers");
  digitalWrite (STEPPER1_EN_PIN, LOW);
  digitalWrite (STEPPER2_EN_PIN, LOW);
  inputString="";
}

void ENAXOFF(){  // ***********************************************************   ENAXOFF
  Serial.println("Disable drivers");
  digitalWrite (STEPPER1_EN_PIN, HIGH);
  digitalWrite (STEPPER2_EN_PIN, HIGH);
  inputString="";
}

void MSpeed(){  // ************************************************************   MoveSpeed
  inputString.setCharAt(0,' ');
  int MoveSpeed=inputString.toInt();
  Serial.print("Speed=");
  Serial.println(MoveSpeed);
  
  stepper1.setSpeed(MoveSpeed);
  stepper2.setSpeed(MoveSpeed);
  encoderInput.write(MoveSpeed);
  inputString="";
}
void NextX(){ // **************************************************************    NextX
  inputString.setCharAt(0,' ');
  int NX=inputString.toInt();
  Serial.print("NX=");
  Serial.println(NX);
  stepper1.moveTo(NX);
  inputString="";
}
