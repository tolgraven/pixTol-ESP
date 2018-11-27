#pragma once

#include <ESPAsyncE131.h>
#include <LoggerNode.h>
#include "renderstage.h"

class sAcnInput: public Inputter {
  public:
  sAcnInput(const String& id, uint16_t startUni, uint8_t numPorts):
    Inputter(id, 1, 512, numPorts), startUni(startUni),
    sacn(new ESPAsyncE131(numPorts)), packet(new e131_packet_t()) {
      sacn->begin(E131_MULTICAST, startUni, numPorts);
    }

  bool ready() { return !sacn->isEmpty(); }
  bool run() {
    if(!ready()) return false;
    sacn->pull(packet);  // Pull packet from ring buffer
    uint16_t length = htons(packet->property_value_count) - 1; // Start code is ignored, we're interested in dimmer data
    uint16_t universe = htons(packet->universe); //htons fixes byte order...
    auto index = universe - startUni;
    // sacn->stats.num_packets,    // Packet counter
    // sacn->stats.packet_errors,  // Packet error counter
    if(index < _bufferCount) {
      buffers[index]->set(packet->property_values, length, 1);
      return Inputter::run(); //sets vars n shit
    }
    // LN.logf(__func__, LoggerNode::DEBUG, "sACN packet uni %d", universe);
    return false;
  }

private:
  int lastStatusCode;
  uint8_t lastSeq;
  uint16_t startUni;
  ESPAsyncE131* sacn;
  e131_packet_t* packet;
};
