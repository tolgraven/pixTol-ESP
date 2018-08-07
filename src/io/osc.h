#pragma once

#include <ArduinoOSC.h>
#include "inputter.h"
#include "outputter.h"


class OSC: public Outputter, public Inputter {
    OSC(const String& pathPrefix, uint8_t numPorts: hz(sourceHz), name(deviceName) {

    }

    void read() {
      osc.parse();
    }

    void subscribe(const String& address, callback) {

    }

};

// class OSCInput: public Inputter {
//   public:
//     OSCInput(const String& pathPrefix, uint8_t numPorts, uint8_t startingUniverse, uint8_t sourceHz): hz(sourceHz), name(deviceName) {
//       // artnet.setName(name.c_str());
//       // artnet.setNumPorts(numPorts);
//       // artnet.setStartingUniverse(startingUniverse);
//       // for(uint8_t i=0; i < numPorts; i++) {
//       //     artnet.enableDMXOutput(i); 
//       // }
//       // artnet.begin();
//       // artnet.setArtDmxCallback(read);
//     }
//
//     // virtual bool read(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
//     //
//     // }
//
//     
//   private:
//     OscServer* server;
//
// };
//
// class OSCOutput: public Outputter {
//
//   private:
//     OscClient* client;
// };
