#pragma once

#include <algorithm>
#include <any>
#include <functional>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <NeoPixelBus.h>
#pragma GCC diagnostic pop


#include "renderstage.h"
#include "task.h"
#include "log.h"

namespace tol {

class Strip: public Outputter {

  using StripDriver = NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod>;

  std::vector<std::unique_ptr<StripDriver>> output;
  const std::pair<int, int> defaultPinRange{12, 32};
  std::vector<int>badPins = {1, 3, 6, 7, 8, 9, 10, 11, 20, 21, 22, 24, 28, 29, 30, 31};
  inline static std::vector<int>usedPins;
  const int numRmtChs = 8;
  inline static int highestUsedRmtCh = -1; // we got 0-7... shared between instances.

  int flushes = 0, flushAttempts = 0;

  public:
  Strip(const String& id, uint8_t subPixels, uint16_t pixelCount, uint16_t startPort,
        int16_t numPorts = 1):
    Outputter(id, subPixels, pixelCount, numPorts) {
      setType("Strip");
      // _portId = startPort;
      setup(startPort, numPorts);
    }

  // void init() override { } // called when task is started...

  void setup(uint16_t startPort, uint16_t numPorts) {
    if(!output.empty()) return; // should wipe and reinit tho i suppose
     
    uint8_t pin = startPort;
    if(pin < defaultPinRange.first) {
      return; // error
    }

    for(uint8_t i = 0; i < numPorts; i++, pin++) {
      while(std::find(badPins.begin(), badPins.end(), pin) != badPins.end()) // XXX actually also skip used!
        pin++;
      
      if(highestUsedRmtCh + 1 >= numRmtChs) {
        return; // full. error
      }
      if(pin > defaultPinRange.second) {
        return; // error
      }
      usedPins.push_back(pin);
      highestUsedRmtCh++;
      
      auto bus = new StripDriver(fieldCount(), i, pin);
      bus->Begin();
      buffer(i).setPtr(bus->Pixels()); // tho, mod lib so can go other way round -> point driver to renderer res, and sharing their Buffer obj so know wazup.
      output.emplace_back(bus); // but move from ctor bc might not want uniform or continuous...
      lg.f(__func__, Log::DEBUG, "%s with rmt ch %u at pin %d.\n", id().c_str(), highestUsedRmtCh, pin);
    }
  }

  void onEvent(const PatchOut& out) {
    buffer(out.i).setCopy(out.buffer); // will already need to do one copy to driver buf so might as well do it here?
    flushAttempts++;

    if(flush(out.i)) flushes++;
  }

  bool flush(int i) override {
    auto& out = output[i];
    if(out->CanShow()) { // will def lead to skipped frames. Should publish something when ready hmm
      if(out->Pixels() != *buffer(i)) {
        // someone repointed our buffah!! booo
        // memcpy(out->Pixels(), *buffer(i), out->PixelsSize()); // tho ideally I mean they're just pointing to the same spot nahmean
      }
      
      out->Dirty();
      
      portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
      // taskENTER_CRITICAL(&mux); // doesnt seem to help and somehow slows down other stuff??
      portENTER_CRITICAL_SAFE(&mux); // seems to work
      out->Show();
      // taskEXIT_CRITICAL(&mux);
      portEXIT_CRITICAL_SAFE(&mux);
      return true;
    }
    return false;
  }

  // ok I forget new driver cant just have ptr persistent at incoming buf location?
  // oh well sort later, but def copy cause have to stay out of touching port dmaing memory..
  bool _run() override {
    bool outcome = false;
    for(int i=0; i < buffers().size(); i++) {
      outcome |= flush(i);
    }
    return outcome;
  }

  bool _ready() override {
    for(auto&& d: output) {
      if(d->CanShow())
        return true;
    }
    return false;
  }

  // void tick() override { _run(); } // will lead to some superflous fuckers so basically stupid. Use to log instead or something...

};

}
