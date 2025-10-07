#include <Arduino.h>
#include <P1AM.h>

enum MachineStates{
  Waiting,
  ColorSensing,
  CountedMove,
  EjectState,
  BinState
};

MachineStates curState = Waiting;

int modInput = 1;
int modOutput = 2;
int modAnalog = 3;

//Inputs
int pulse = 1;
int lbIn = 2;
int lbOut = 3;
int LBw = 4;
int LBr = 5;
int LBb = 6;

//Outputs
int conv = 1;
int compressor = 2;
int ejectW = 3;
int ejectR = 4;
int ejectB = 5;
int ArmW = 6;
int ArmR = 7;
int ArmB = 8;

//Analog Input
int color = 1; 

//Vars
int colorValue = 10000; //Maxes out at 8100 for sensor
int distToEject = 0;
bool prevKeyState = false;
int distMoved = 0;
bool curKey = false;
char targetColor = 'b';

void setup() {
  delay(1000);
  Serial.begin(9600);
  delay(1000);

  //Start up P1am modules!
    while (!P1.init()){
    delay(1);
  }
}

bool InputTriggered(){
  return !P1.readDiscrete(modInput, lbIn);
}

bool OutputTriggered(){
  return !P1.readDiscrete(modInput, lbOut);
}

void ToggleConveyor(bool s){
  P1.writeDiscrete(s, modOutput, conv);
}
int GetColor(){
      return P1.readAnalog(modAnalog, color);
    }

bool GetPulseKey(){
  return P1.readDiscrete(modInput, pulse);
} 

void ToggleCompressor(bool s){
  P1.writeDiscrete(s, modOutput, compressor);
}

void UseEjector(char c){
  int tempPin;
  if(c == 'w'){
    tempPin = ejectW;
  }
  else if(c == 'r'){
    tempPin = ejectR;
  }
  else{
    tempPin = ejectB;
  }
  P1.writeDiscrete(true, modOutput, tempPin);
  delay(1000);
  P1.writeDiscrete(false, modOutput, tempPin);
}

void loop() {
  switch (curState){
    case Waiting:
    //Wait for light barrier to be tripped
    //After tripped, swich state and turn on conveyor
    if(InputTriggered()){
      curState = ColorSensing;
      ToggleConveyor(true);
      colorValue = 10000;
    }

    break;

    case ColorSensing: //Wiring of photo resistor matter
    //Get color and find min
    colorValue = min(GetColor(), colorValue);
    //Keep on going untill 2nd light barrier
    //Then switch states
    if(OutputTriggered()){
      curState = CountedMove;
      distMoved = 0;
      //Decide how far or move
      if(colorValue < 2500){
        distToEject = 3;
        targetColor = 'w';
      }
      else if(colorValue < 4600){ //4600 can't be first at it would never trigger the 2500 statment
        distToEject = 9;
        targetColor = 'r';
      }
      else{
        distToEject = 14; 
        targetColor = 'b';
      }
      ToggleCompressor(true);
    }
    //Serial.print(GetColor()); //Must include () to include funtion and not some value. This was used to display color values to serial port.
    //delay(10);

    break;

    case CountedMove:
    //Watch the pulse key to move that far
    curKey = GetPulseKey();
    if(curKey && !prevKeyState){
      distMoved ++;
    }
    prevKeyState = curKey;
    //Switch states and turn off conveyor
    if(distMoved >= distToEject){
      curState = EjectState;
      ToggleConveyor(false);
    }

    break;

    case EjectState:
    UseEjector(targetColor);
    curState = BinState;
    break;

    case BinState:
    // Binstate readings
    bool binW = !P1.readDiscrete(modInput, LBw);
    bool binR = !P1.readDiscrete(modInput, LBr);
    bool binB = !P1.readDiscrete(modInput, LBb);
   //Used to check which bin(s) has a puck
   char BinPin = ' ';
   delay(500);
   if(binW){
    BinPin = 'w';
    }
    else if(binR){
    BinPin = 'r';
    }
    else if (binB){
    BinPin = 'b';
    }
    Serial.println(BinPin);
    curState = Waiting;
    break;

    
  /*default:
  break*/


    //Signal robot arm to know theres objects to pick up.
  }

  /*bool isOn = P1.readDiscrete(1, 2); //Used to find sensor data
  Serial.println(isOn);
  delay(100);*/ 
}
