#pragma once

#include <functional>
#include <ArtNetE131Lib.h>
#include "log.h"
#include "renderstage.h"

class Artnet: public NetworkIn, public Outputter {
  std::unique_ptr<espArtNetRDM> artnet;

  public:
  Artnet(const String& id, uint16_t startUni, uint8_t numPorts,
         bool enableIn = true, bool enableOut = false):
    NetworkIn(id, startUni, numPorts),
    Outputter(id, 1, 512, numPorts),
    artnet(new espArtNetRDM()) {
      Inputter::setType("artnet"); Inputter::setEnabled(enableIn);
      Inputter::setRunFn(std::bind(&Artnet::_runIn, this));
      Outputter::setType("artnet"); Outputter::setEnabled(enableOut);
      Outputter::setRunFn(std::bind(&Artnet::_runOut, this));

      using namespace std::placeholders;
      artnet->init(id.c_str());

      // all this goes out in own fns. Now time wrapping RDM and that...
      // and reimpl config parse/save.
      if(enableIn) {
        uint8_t groupID = artnet->addGroup(0, 0); // net, subnet
        for(int port=0; port < numPorts; port++) {
          artnet->addPort(groupID, port, startUni + port, (uint8_t)port_type::RECEIVE_DMX);
        }
      }

      if(enableOut) {
        uint8_t groupID = artnet->addGroup(0, 1); // net, subnet
        for(int port=0; port < numPorts; port++) {
          artnet->addPort(groupID, port, startUni + port, (uint8_t)port_type::SEND_DMX);
        }
      }
      auto cb = [this](uint8_t group, uint8_t port, uint16_t length, bool sync) {
        uint8_t* data = this->artnet->getDMX(group, port);
        if(data) this->onData(port, length, data);
      };
      artnet->setArtDMXCallback(cb);
      artnet->begin();
  }

  bool _runIn() { // might this result in dupl calls or does the multiple _runFn negate virtual dispatch as hoped?
    // OLA now complaining about "got unknown packet", "mismatched version" 3584 and stuff
    // investigate!!
    switch(artnet->handler()) { // calls handleDMX which calls callback...
      case ARTNET_ARTDMX: return true; // case OpDmx: return true;
    }
    // but so for example if ArtSync enabled should return false until sync arrives -> transparent from outside
    return false;
  }
  bool _runOut() {
      for(uint8_t port=0; port < Outputter::buffers().size(); port++) {
        uint8_t group = 1; //send-group, made-up for now...
        auto& buf = Outputter::buffer(port);
        // it's weird tho how buf1/controls somehow throttles even w/o dirty check? not actually impl like...
        // turns out setting maxHz makes it start pushing data even when no changes.
        // WEIRD! bc should only affect microsTilReady() right?

        if(buf.dirty()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
        // if(buf.dirty() || port.hasnt_sent_in_4s_yada()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
          artnet->sendDMX(group, port, buf.get(), buf.lengthBytes()); // so it's required it properly set up. wont overflow at least
          buf.setDirty(false); // our view anyways. Many Buf same addr both good and tricky
        }
      }
    return true;
  }

  void cbRDM() {} // get on so can start using
};

