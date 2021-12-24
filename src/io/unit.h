#pragma once

#include <vector>
//contains stuff to handle physical UI - encoders, buttons
// #include <FunctionalInterrupt.h>
// #include <Encoder.h>
// #include <SmartPins.h>
// button libs: InputDebounce 123, clickButton 1105
//OLED screen lib?
#include "renderstage.h"

enum PhysicalPinID: uint8_t {
  INVALID = 255,
  //figure out smart way to generalize D1/general ESP/other pins through mapping...
  //prob just, set at compile time, and 1 == D1 == arduino_1 == esp_1...?
};

class PhysicalComponent { // should have setup, update routines, a la Homie.
  // Should also prob be (to start) inheriting HomieNode, but easily decoupled...
  // these need to register themselves with something put in Scheduler, on creation, then automatically setup/update
  protected:
  std::string _id;
  std::vector<uint8_t> pins;
  virtual void setup() {}

  public:
    PhysicalComponent(const std::string& id, const std::vector<uint8_t>& pins):
      _id(id), pins(pins) {
      setup();
    }
    // virtual ~PhysicalComponent() {}
    virtual ~PhysicalComponent() { pins.clear(); }
    virtual void loop() = 0; //all likely to be either Inputter or Outputter, no? so already got Run()...
};

class RotaryEncoder: public PhysicalComponent, public Inputter {
  // RotaryEncoder(uint8_t aPin = D1, uint8_t bPin = D2, uint8_t clickPin = D3, const std::string& id = "Rotary Encoder",
  public:
  RotaryEncoder(const std::string& id, const std::vector<uint8_t>& pins,
      bool resetOnClick = true, void (*clickCallback)(long pos) = nullptr, uint16_t debounceMs = 50):
    PhysicalComponent(id, pins), Inputter(id, 1, 2), //two fields (pos and click), one wide
     clickCallback(clickCallback), resetOnClick(resetOnClick), debounceMs(debounceMs) {
  }
  // ~RotaryEncoder() { delete re; }
  
  void (*clickCallback)(long pos); //then gotta pass it in, maybe as a lambda? but better if proper
  //std::function<void(const RotaryEncoder&)> clickCB = &RotaryEncoder::onDepressed;

  private:
  // Encoder* re;
  //SmartPins::spEncoderAuto* rea;
  long _pos = -999;
  bool resetOnClick;
  bool _depressed = false;
  long depressedAt = 0;
  uint16_t debounceMs;

  public:
  void onDepressed() { _depressed = true; }
  void depressed() { _depressed = true; }
  void setup() override {
     // re = new Encoder(pins[0], pins[1]);
    // SmartPins.EncoderAuto(pins[0], pins[1], INPUT_PULLUP, &RotaryEncoder::onDepressed);
    if(pins.size() > 2) { //use a third pin for clicks
       pinMode(pins[3], INPUT_PULLUP);
       //attachInterrupt(pins[3], depressed, RISING); //here we need proper member fn ptr
    }
  }

  void clicked() {
    _depressed = false;
    depressedAt = millis(); // only update time once debounce has passed
    // if(resetOnClick) re->write(0);
    if(clickCallback) clickCallback(_pos);
  }
  bool run() {
    // long newPos = re->read();
    // if(newPos != _pos) _pos = newPos;
    if(_depressed && millis()-depressedAt > debounceMs) clicked();
    return Inputter::run();
  }
  void loop() { run(); }
  long pos() { return _pos; }
};


class OutputPin: public PhysicalComponent, public Outputter {
  public:
  OutputPin(uint8_t p): // OutputPin(PhysicalPinID p):
    PhysicalComponent("Pin", std::vector<uint8_t>(p)),
    Outputter("Pin", 1, 1) {
      pinMode(p, OUTPUT);
  }
  OutputPin(const std::string& id, std::vector<uint8_t> pins):
    PhysicalComponent(id, pins),
    Outputter(id, 1, pins.size()) {
      for(auto p: pins) pinMode(p, OUTPUT);
  }

  bool run() {
    if(pwm) runPWM();
    else {
      uint8_t i = 0;
      for(auto p: pins) {
        digitalWrite(p, buffer().get()[i]); //where in buffer to look
        i++;
      }
    }
    return Outputter::run();
    // timeLastRun = micros();
  }
  void loop() { run(); }
  private:
  // PWM enabled output pin. To activate whatever or eg using NPN transistor to close a circuit
  // put in sep class or?
  bool pwm = false;
  bool initing = false, doneInit = false;
  uint32_t initAt; //use with RS timeLastRun...
  void runPWM() { //update PWM with thing
    if(buffer().get()[0] && !initing && !doneInit) { //for fog machine, need to run it prob 100% for a tiny bit to ramp up
      //blastoff
      // set timer for early part timeout
    }
    if(doneInit) {
      // analogWrite(pins[0], buffer().get());
    }
  }
};


class PhysicalUI { //creates and holds components, manages state through callbacks BUT does not act
  // on anything outside of update() - this should be the general way of things, including renderer etc
  // use callbacks, but only to set buffers/variables, not to run anything
  std::vector<PhysicalComponent*> components;
  bool pinAvailable() {
    //go through components, ask them what pins they use?
    return false;
  }
  public:
  PhysicalUI() {}
  void addComponent(const PhysicalComponent& c) {
  }
  void loop() {
    for(auto c: components) {
      c->loop();
    }
  }
};
