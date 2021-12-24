#pragma once

/* #include <HomieNode.hpp> */
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include "renderstage.h"
#include "log.h"
#include "color.h"

// want to create like one instance for  color, no? but then that entails rgbw range 4, rgb 3, hsl 3, named colors string...
// feasible? To start, simply static ranges - use for controls...
/* class MqttHomie: public Inputter, public HomieNode { */
class MqttHomie: public Inputter {
  public:
    // MqttHomie(): Inputter("Homie", 1, 128), HomieNode("Render-control", "Inputter") {
  MqttHomie(const std::string& id, const std::string& topic, uint16_t fieldCount): //or does num-anything make sense with dynamic auto-mapping?
    /* Inputter(id, 1, fieldCount), HomieNode(id, "Input") { */
    Inputter(id, 1, fieldCount) {

    type("MqttHomie");
    /* advertiseRange(topic, 0, fieldCount-1).settable(); */
  }

  bool handleInput(const std::string& property, const HomieRange& range, const std::string& value) {
    if(range.index < fieldCount()) {
      uint8_t* val = new uint8_t(value.toFloat() * 255);
      buffer().setCopy(val, 0, 0, range.index);
      delete val; //XXX does it get copied properly?
      Inputter::run();
      return true;
    }
    return false;
  }
  // virtual void setup() override;
  // virtual void loop(); // relevant?
};
