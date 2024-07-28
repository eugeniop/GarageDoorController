#ifndef _RELAY_H
#define _RELAY_H

class Relay {
  int pin;
public:
  Relay(int pin) : pin(pin){
  }

  void Init(){
    pinMode(pin, OUTPUT);
    off();
  }

  void onFor(long ms){
    on();
    delay(ms);
    off();
  }

  void on(){
    Serial.println("ON");
    digitalWrite(pin, HIGH);
  }

  void off(){
    Serial.println("OFF");
    digitalWrite(pin, LOW);
  }
};

#endif
