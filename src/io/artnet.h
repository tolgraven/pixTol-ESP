#pragma once

#include <ArtnetnodeWifi.h>
#include <LoggerNode.h>
#include "renderstage.h"

class IO {
  public:
    IO() {}
    IO(Inputter* in, Outputter* out) {
      if(in != nullptr) input = in;
      if(out != nullptr) output = out;

    }
    virtual void enableInput(bool state, int8_t bufferIndex = -1) = 0; // idx -1 = all
    virtual void enableOutput(bool state, int8_t bufferIndex = -1) = 0;

    // virtual void 

  protected:
    Inputter* input = nullptr;
    Outputter* output = nullptr;
};


class ArtnetInput: public Inputter {

  public:
    ArtnetInput(const String& id, uint8_t numPorts, ArtnetnodeWifi* artnet)
    : Inputter(id, 1, 512)
    , artnet(artnet)
  {
      artnet->setArtDmxCallback(ArtnetInput::inputCallback);
    // artnet->getDmxFrame();
    // artnet->artDmxCallback(); //etc, whichever feels right...
  }
    // virtual bool canRead() { return isActive; }
    virtual bool read() {
      if(canRead()) {
        switch(artnet->read()) { // calls handleDMX which calls callback...
          case OpDmx:
            // if(log_artnet >= 4) {
            //     LN.logf("artnet", LoggerNode::DEBUG, "DMX");
            // }
            // uint8_t index = 0; //actually get by uni
            // data[index] = artnet->getDmxFrame(); //just gets pointer, so prob same as last time, unless multiple unis
            return true; //return true if new frame has arrived XXX if multiple ports must wait til all arrived, OR ArtSync...

          case OpPoll:
            LN.logf("artnet", LoggerNode::DEBUG, "Art Poll Packet");
            break;
          // case OpInput OpRdm etc zillion other things not impl...
        }
      return false;
      }
    }
    uint16_t startUni;

    static void inputCallback(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
      // handle sequence check, update droppedFrames?
      data[universe - index] = data;
      

    }
  private:
    uint8_t lastSeq;
    ArtnetnodeWifi* artnet;

};

class ArtnetOutput: public Outputter {

  public:
    ArtnetOutput(const String& id, uint8_t numPorts, ArtnetnodeWifi* artnet)
    : Outputter(id, 1, 512, numPorts) 
    , artnet(artnet)
  {
    // below would go in dmxserial class tho 
    // artnet->setDMXOutput(uint8_t outputID, uint8_t uartNum, uint16_t attachedUniverse)<|0|>
  }
    virtual bool canEmit() { return true; }
    virtual void emit(uint8_t* data, uint16_t length) {
      // runrun
    }
  private:
    ArtnetnodeWifi* artnet;
};

class Artnet: public IO {
  // enum Functions {
  //   Dimmer = 1, Strobe = 2, Hue = 3, Attack = 4, Release = 5,
  //   Bleed = 6, Noise = 7, RotateFwd = 8, RotateBack = 9, DimmerAttack = 10, DimmerRelease = 11
  // }; // oh yeah. since Modulator holds static vector of all its instances, it'll be
    // easy to just fetch by ID for updates. so no need for an extra buffer just for that...

  private:
    ArtnetnodeWifi artnet;

  public:
    Artnet(const String& deviceName, uint8_t numPorts, uint8_t startingUniverse, uint8_t sourceHz)
   {
      artnet.setName(deviceName.c_str()); // artnet.setShortName(); artnet.setLongName();
      artnet.setNumPorts(numPorts);
      artnet.setStartingUniverse(startingUniverse);
      input = new ArtnetInput("artnet - in", numPorts, &artnet);
      input->index = startingUniverse;
      input->bufferCount = numPorts;

      output = new ArtnetOutput("artnet - out", numPorts, &artnet);
      for(uint8_t i=0; i < numPorts; i++) {
          artnet.enableDMXOutput(i); 
      }

      artnet.begin();
    }


    // virtual bool addDestination(uint8_t* data, uint16_t length, uint16_t offset = 0) {
    //
    // }

    virtual void setActive(bool state = true, int8_t bufferIndex = -1) {
      /* isActive = state; */
      if(state) artnet.enableDMX();
      else artnet.disableDMX();
    }
    virtual void enable() { artnet.enableDMX(); }
    virtual void disable() { artnet.disableDMX(); }

};
