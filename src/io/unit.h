#pragma once

#include <vector>
//contains stuff to handle physical UI - encoders, buttons
#include <Encoder.h>
// button libs: InputDebounce 123, clickButton 1105
//OLED screen lib?

enum pin: int8_t {
  INVALID = -1,
  //figure out smart way to generalize D1/general ESP/other pins through mapping...
  //prob just, set at compile time, and 1 == D1 == arduino_1 == esp_1...?
};

class PhysicalUI { //creates and holds components, manages state through callbacks BUT does not act
  // on anything outside of update() - this should be the general way of things, including renderer etc
  // use callbacks, but only to set buffers/variables, not to run anything
  std::vector<PhysicalComponent*> components;
  bool pinAvailable() {
    //go through components, ask them what pins they use?
  }
  public:
  PhysicalUI() {}
  void addComponent(const PhysicalComponent& c) {
  }
  void update() {
    for(c: components) {
      c->update();
    }
  }
};

class PhysicalComponent { // should have setup, update routines, a la Homie.
  // Should also prob be (to start) inheriting HomieNode, but easily decoupled...
  // these need to register themselves with something put in Scheduler, on creation, then automatically setup/update
  std::vector<int8_t> pins;
  String _id;
  virtual void setup() {}

  public:
    PhysicalComponent(const String& id, const std::vector<int8_t>& pins):
      _id(id), pins(pins) {
      setup();
    }
    virtual ~PhysicalComponent {}
    virtual void update() = 0;
};

class RotaryEncoder: public PhysicalComponent {
  // RotaryEncoder(uint8_t aPin = D1, uint8_t bPin = D2, uint8_t clickPin = D3, const String& id = "Rotary Encoder",
  RotaryEncoder(const std::vector<int8_t>& pins, const String& id = "Rotary Encoder",
      bool resetOnClick = true, void (*clickCallback)(long pos) = nullptr, uint16_t debounceMs = 50):
    PhysicalComponent(id, pins),
    resetOnClick(resetOnClick), clickCallback(clickCallback), debounceMs(debounceMs) {
  }
  ~RotaryEncoder() { delete re; }
  Encoder* re;
  long _pos = -999;
  bool resetOnClick = true;
  bool _depressed = false;
  long depressedAt = 0;
  uint16_t debounceMs;

  void setup() override {
    re = new Encoder(pins[0], pins[1]);
    if(pins.size() > 2) { //use a third pin for clicks
      pinMode(pins[3], INPUT_PULLUP);
      attachInterrupt(pins[3], depressed, RISING);
    }
  }

  void depressed() { _depressed = true; }
  void clicked() {
    _depressed = false;
    depressedAt = millis(); // only update time once debounce has passed
    if(resetOnClick) re.write(0);
    if(clickCallback) clickCallback(_pos);
  }

  void (*clickCallback)() = nullptr;
  public:
  void update() {
    long newPos = re.read();
    if(newPos != _pos) _pos = newPos;
    if(depressed && millis()-depressedAt > debounceMs) clicked();
  }
  long pos() { return _pos; }
};
