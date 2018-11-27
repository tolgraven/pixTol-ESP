#pragma once

#include <functional>
#include <ArtnetnodeWifi.h>
#include <LoggerNode.h>
#include "renderstage.h"

// #include "espArtNetRDM.h"
#define ART_FIRM_VERSION 0x0A00   // 2-byte Artnet FW ver
#define ARTNET_OEM 0x00ff    // Artnet OEM code - "unknown"
#define ESTA_MAN 0x7fff      // ESTA Manufacturer code - prototyping reserved
#define ESTA_DEV 0xEE000000  // RDM Device ID (used with Man Code to make 48bit UID)

class ArtnetInput: public Inputter {
  public:
  ArtnetInput(const String& id, uint16_t startUni, uint8_t numPorts):
    Inputter(id, 1, 512, numPorts), startUni(startUni), artnet(new ArtnetnodeWifi()) {
      using namespace std::placeholders;
      auto cb = std::bind(&ArtnetInput::callback, this, _1, _2, _3, _4);
      artnet->setProperCallback(cb); // artnet->setArtDmxCallback(cb);
      artnet->setName(id.c_str()); // artnet.setShortName(); artnet.setLongName();
      artnet->setStartingUniverse(startUni);
      artnet->setNumPorts(numPorts);
      for(int i = 0; i < numPorts; i++)
        artnet->enableDMXOutput(i);
      artnet->begin();
    }

    bool ready() { return true; } //XXX how hmm
    bool run() {
      if(!ready()) return false;
      switch(artnet->read()) { // calls handleDMX which calls callback...
        case OpDmx: return Inputter::run(); //return true if new frame has arrived XXX if multiple ports must wait til all arrived, OR ArtSync...
        case OpPoll: //LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
          break;
        case OpNzs: LN.logf("artnet", LoggerNode::DEBUG, "NZs kiwi??"); break;
        // case OpInput OpRdm etc zillion other things not impl...
      }
      return false;
    }

    void callback(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
      uint8_t index = universe - startUni; //XXX check proper
      if(index < _bufferCount) {
        buffers[index]->set(data, length);
        newData = true;
      }
    }

  private:
    // int lastArtnetCode;
    uint16_t startUni;
    uint8_t lastSeq;
    ArtnetnodeWifi* artnet;
};

// class ArtnetOutput: public Outputter {
//   public:
//     ArtnetOutput(const String& id, uint8_t numPorts, ArtnetnodeWifi* artnet)
//     : Outputter(id, 1, 512, numPorts)
//     , artnet(artnet) {
//     // below would go in dmxserial class tho
//     // artnet->setDMXOutput(uint8_t outputID, uint8_t uartNum, uint16_t attachedUniverse)<|0|>
//   }
//     virtual bool canEmit() { return true; }
//     virtual void emit(uint8_t* data, uint16_t length) { // runrun
//     }
//   private:
//     ArtnetnodeWifi* artnet;
// };
//
// class IO {
//   public:
//     IO() {}
//     IO(Inputter* in, Outputter* out) {
//       if(in != nullptr) input = in;
//       if(out != nullptr) output = out;
//     }
//     virtual void enableInput(bool state, int8_t bufferIndex = -1) = 0; // idx -1 = all
//     virtual void enableOutput(bool state, int8_t bufferIndex = -1) = 0;
//
//   protected:
//     Inputter* input = nullptr;
//     Outputter* output = nullptr;
// };

// class Artnet: public IO {
//   private:
//     ArtnetnodeWifi artnet;
//     esp8266ArtNetRDM artRDM;
//
//   public:
//     Artnet(const String& deviceName, uint8_t numPorts, uint8_t startingUniverse, uint8_t sourceHz)
//    {
//       artnet.setName(deviceName.c_str()); // artnet.setShortName(); artnet.setLongName();
//       artnet.setNumPorts(numPorts);
//       artnet.setStartingUniverse(startingUniverse);
//       input = new ArtnetInput("artnet - in", numPorts, &artnet);
//       input->index = startingUniverse;
//       input->bufferCount = numPorts;
//
//       output = new ArtnetOutput("artnet - out", numPorts, &artnet);
//       for(uint8_t i=0; i < numPorts; i++) {
//           artnet.enableDMXOutput(i);
//       }
//       artnet.begin();
//     }
//
//     // virtual bool addDestination(uint8_t* data, uint16_t length, uint16_t offset = 0) {
//     // }
//
//     virtual void setActive(bool state = true, int8_t bufferIndex = -1) {
//       if(state) artnet.enableDMX();
//       else artnet.disableDMX();
//     }
//     virtual void enable() { artnet.enableDMX(); }
//     virtual void disable() { artnet.disableDMX(); }
// };
