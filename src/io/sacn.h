#pragma once

#include <ESPAsyncE131.h>
#include "log.h"
#include "renderstage.h"

namespace tol {

class sAcnInput: public NetworkIn {
  ESPAsyncE131* sacn;
  e131_packet_t* packet;
  public:
  sAcnInput(const std::string& id, uint16_t startUni, uint8_t numPorts):
    NetworkIn(id, startUni, numPorts),
    sacn(new ESPAsyncE131(numPorts)), packet(new e131_packet_t()) {
      setType("sacn");
      sacn->begin(E131_MULTICAST, startUni, numPorts);
    }
  ~sAcnInput() { delete sacn; delete packet; }
  bool _ready() override { return !sacn->isEmpty(); } // wouldnt isEmpty mean ready for new packets tho?
  bool _run() override {
    sacn->pull(packet); // Pull packet from ring buffer sacn->stats.num_packets,    // Packet counter sacn->stats.packet_errors,  // Packet error counter
    return onData(htons(packet->universe), htons(packet->property_value_count) - 1, packet->property_values + 1); //htons for byte order. sub baseuni and get buffer index / Start code is ignored
  }
};

}
