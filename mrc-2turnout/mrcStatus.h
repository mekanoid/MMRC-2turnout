/*
 * MRC Status led library
 */
#include <Arduino.h>

#ifndef mrcStatus_h
#define mrcStatus_h

class mrcStatus {

  private:
    byte debug = 1;
    String dbText = "Status : ";
    byte pin;
    unsigned long previousMillis;
    int interval = 1000;
    int state = 1;
    enum Action {
      OFF = 0,
      ON = 1,
      BLINK = 2
    } action;
    
  public:
    mrcStatus(byte pin);
    void init();
    void loop();
    void on();
    void off();
    void blink(int time);
    boolean status();
};

#endif
