#ifndef PIXTOL_CONFIG
#define PIXTOL_CONFIG

#include <Homie.h>
// #include <HomieNode.hpp>

class ConfigNode: public HomieNode {
 public:
    ConfigNode();

    HomieSetting<bool> debug;              
    HomieSetting<long> logArtnet;         
    HomieSetting<bool> logToMqtt;        
    HomieSetting<bool> logToSerial;      

    HomieSetting<long> dmxHz;
    HomieSetting<long> strobeHzMin;
    HomieSetting<long> strobeHzMax;

    HomieSetting<bool> clearOnStart;

    HomieSetting<long> bytesPerLed;
    HomieSetting<long> ledCount;

    HomieSetting<long> universes;
    HomieSetting<long> startUni;
    HomieSetting<long> startAddr;

 protected:
  virtual bool handleInput(const String& property, const HomieRange& range, const String& value);
    // virtual void setup() override;
    // virtual void loop() override;
  
//  private:
};

// extern ConfigNode cfg;

#endif
