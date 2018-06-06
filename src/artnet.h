#pragma once

#include <ArtnetnodeWifi.h>
#include "inputter.h"
#include "outputter.h"

class Artnet: public Inputter, public Outputter {
  Artnet(): Inputter(), Outputter()
  {}

  private:
    ArtnetnodeWifi* artnet;
};

// class ArtnetInput: public Artnet: public Inputter {
  public:
    Artnet(const String& deviceName, uint8_t numPorts, uint8_t startingUniverse, uint8_t sourceHz):
      Inputter(), Outputtew(), hz(sourceHz), name(deviceName) {
  // void initArtnet( void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*) ) {
      artnet.setName(name.c_str());
      artnet.setNumPorts(numPorts);
      artnet.setStartingUniverse(startingUniverse);
      for(uint8_t i=0; i < numPorts; i++) {
          artnet.enableDMXOutput(i); 
      }
      artnet.begin();
      artnet.setArtDmxCallback(this->read);
    }

    // where actually put this callback? can't virtualize fucking every potential getup in Inputter ugh...
    virtual bool read(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {

    }

    
  private:

// };
//
// class ArtnetOutput: public Artnet: public Outputter {
//
};
