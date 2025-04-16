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
  std::vector<int>badPins = {1, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 20, 21, 22, 24, 28, 29, 30, 31};
  inline static std::vector<int>usedPins;
  const int numRmtChs = 8;
  inline static int highestUsedRmtCh = -1; // we got 0-7... shared between instances.

  int flushes = 0, flushAttempts = 0;

  public:
  Strip(const std::string& id, uint8_t subPixels, uint16_t pixelCount, uint16_t startPort,
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
      
      auto bus = new StripDriver(fieldCount(), highestUsedRmtCh, pin);
      bus->Begin();
      buffer(i).setPtr(bus->Pixels()); // tho, mod lib so can go other way round -> point driver to renderer res, and sharing their Buffer obj so know wazup.
      output.emplace_back(bus); // but move from ctor bc might not want uniform or continuous...
      lg.f(__func__, Log::DEBUG, "%s with rmt ch %u at pin %d.\n", id().c_str(), highestUsedRmtCh, pin);
    }
  }

  void onEvent(const PatchOut& out) override {
    if(out.buffer.fieldSize() != buffer(out.destIdx).fieldSize())
      return; // temp for having multiple renderers and whatnot, need proper patching tho...
    buffer(out.destIdx).setCopy(out.buffer); // will already need to do one copy to driver buf so might as well do it here?
    flushAttempts++;

    if(flush(out.destIdx)) flushes++;
  }

  bool flush(int i) override {
    auto& out = output[i];
    while(!out->CanShow()) { } // spinlock here will limit renderer to actually outputtable fps. Suppose more efficient would be one thread per pin tho?
    // while(!out->CanShow()) { vTaskDelay(1); } // would make sense if sep task per pin...
      
    out->Dirty();
    
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    // portENTER_CRITICAL_SAFE(&mux); // seems to work
    xPortEnterCriticalTimeout(&mux, portMUX_NO_TIMEOUT);
    out->Show();
    portEXIT_CRITICAL_SAFE(&mux);
    return true;
      
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
