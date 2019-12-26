/*
 * MMRC Turnout library
 */
#include <Arduino.h>
#include <Servo.h> 

#ifndef mmrcServo_h
#define mmrcServo_h

class mmrcServo {
  private:
    byte debug = 1;
    String dbText = "Servo  : ";
    byte pin;
    int currentPosition;
    int maxPosition = 85;
    int minPosition = 75;
    int endPosition;
    int moveInterval = 100;
    int backStep;
    int servoStatus;
    unsigned long previousMillis;
    enum Action {
      NON = 0,
      MIN = 1,
      MAX = 2
    } servoAction;

  public:
    mmrcServo(byte pin);
    Servo myservo;
    typedef void(*callback_t)();                // Test med callback
    void onFinished(callback_t callback);       // Test med callback
    void init();
    void limits(int min, int max, int interval, int backStep);
    void loop();
    void through();
    void diverge();
    void moveTo (int newPosition);
    boolean status();
    boolean position();
    callback_t onFinished_callback;        // Callback function for pressed events.

};

#endif
