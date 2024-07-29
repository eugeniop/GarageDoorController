#ifndef DOOR_H
#define DOOR_H

class Door {

  int openPin, closePin;
  static const char * states[];
  
public:

  enum State { OPEN, CLOSED, MOVING, INVALID };

  Door(int openPin, int closePin) : openPin(openPin), closePin(closePin){
  }

  void Init(){
    pinMode(closePin, INPUT_PULLUP);
    pinMode(openPin, INPUT_PULLUP);  
  }

  State getState(){
    int openSensor = digitalRead(openPin) == 0;
    int closeSensor = digitalRead(closePin) == 0;

    if(openSensor && closeSensor) return INVALID;
    if(openSensor) return OPEN;
    if(closeSensor) return CLOSED;
    return MOVING;
  }

  const char * getStateStr(){
    return states[getState()];   
  }
};

const char * Door::states[] = {"open", "closed", "moving", "invalid"}; 

#endif
