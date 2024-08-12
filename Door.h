#ifndef DOOR_H
#define DOOR_H

class Door {

public:
  enum State { OPEN, CLOSED, OPENING, CLOSING, MOVING, INVALID };

private:
  int openPin, closePin;
  static const char * states[]; //Must follow the same order as the enum (see below)

  State getCurrentState(){
    int openSensor = digitalRead(openPin) == 0;   //Logic in pullup is reversed
    int closeSensor = digitalRead(closePin) == 0;
    if(openSensor && closeSensor) return INVALID;
    if(openSensor) return OPEN;
    if(closeSensor) return CLOSED;
    
    //Neither sensor is activated, it is MOVING
    return MOVING;
  }
  
public:

  Door(int openPin, int closePin) : openPin(openPin), closePin(closePin){
  }

  void Init(){
    pinMode(closePin, INPUT_PULLUP);
    pinMode(openPin, INPUT_PULLUP);
  }

  State getState(){
    State now = getCurrentState();
    return now;
  }

  const char * getStateStr(){
    return states[getState()];   
  }
};

//Must follow the same order as the enum
const char * Door::states[] = {"ctrl_open", "ctrl_closed", "ctrl_opening", "ctrl_closing", "ctrl_moving", "ctrl_invalid"}; 

#endif
