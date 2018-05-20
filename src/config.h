#ifndef PIXTOL_CONFIG
#define PIXTOL_CONFIG

#include <Homie.h>
#include <LoggerNode.h>

class ConfigNode: public HomieNode {
 public:
    ConfigNode();

    // seems homie (now?) has a 10 settings limit. scale down settings, or patch homie?
    // HomieSetting<bool> debug;              
    HomieSetting<long> logArtnet;         
    // HomieSetting<bool> logToMqtt;        
    // HomieSetting<bool> logToSerial;      

    HomieSetting<long> dmxHz;
    // HomieSetting<double> strobeHzMin;
    // HomieSetting<double> strobeHzMax;

    HomieSetting<bool> clearOnStart;

    HomieSetting<long> bytesPerLed;
    HomieSetting<long> interFrames;
    HomieSetting<long> ledCount;
    // HomieSetting<long> sourceLedCount;

    // HomieSetting<long> universes;
    HomieSetting<long> startUni;
    // HomieSetting<long> startAddr;

    //temp til strip class in action
    HomieSetting<bool> setMirrored;             
    HomieSetting<bool> setFolded; 
    // HomieSetting<bool> setFlipped;

 protected:
  virtual bool handleInput(const String& property, const HomieRange& range, const String& value);
    // virtual void setup() override;
    // virtual void loop() override;
  
//  private:
};

// extern ConfigNode cfg;

#endif
