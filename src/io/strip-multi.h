#pragma once

#include <algorithm>
#include <any>
#include <functional>
#include <NeoPixelBus.h>

// #define FASTLED_ESP32_I2S true
// #include <FastLED.h>
// CRGB leds[8][120];

#include "renderstage.h"
#include "log.h"


class Strip: public Outputter {

  using StripDriver = NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod>;
  // template <typename... TSends, size_t... Is>
  // void addPortRMT(uint16_t pixelCount, uint16_t port, std::index_sequence<Is...> ) {
  //     // in C++17, that's just
  //       (output.emplace_back(new NeoPixelBus<NeoGrbFeature, TSends>(pixelCount, port)), ...);
  //       (output.at(Is)->Begin(), ...);
  //       (buffer(Is).setPtr(output.at(Is)->Pixels()), ...); // tho, mod lib so can go other way round -> point driver to renderer res,
  // }

  std::vector<std::unique_ptr<StripDriver>> output;

  const std::pair<int, int> defaultPinRange{25, 33};

  public:
  Strip(const String& id, uint8_t subPixels, uint16_t pixelCount, uint16_t startPort, uint16_t numPorts = 1):
    Outputter(id, subPixels, pixelCount, numPorts) {
      setType("Strip");
      // _portId = startPort;
      _init(startPort, numPorts);
    }

  // static constexpr int getPin(int offset) { return 14 + offset; }

  void _init(uint16_t startPort, uint16_t numPorts) {
    uint8_t pin = startPort;

    // FastLED.addLeds<NEOPIXEL, 25>(leds[0], 120); FastLED.addLeds<NEOPIXEL, 26>(leds[1], 120); FastLED.addLeds<NEOPIXEL, 27>(leds[2], 120); FastLED.addLeds<NEOPIXEL, 14>(leds[3], 120); FastLED.addLeds<NEOPIXEL, 12>(leds[4], 120); FastLED.addLeds<NEOPIXEL, 18>(leds[5], 120); FastLED.addLeds<NEOPIXEL, 17>(leds[6], 120); FastLED.addLeds<NEOPIXEL, 16>(leds[7], 120);
    for(uint8_t ch = 0; ch < numPorts; ch++, pin++) {
      auto bus = new StripDriver(fieldCount(), ch, pin);
      bus->Begin();
      buffer(ch).setPtr(bus->Pixels()); // tho, mod lib so can go other way round -> point driver to renderer res, and sharing their Buffer obj so know wazup.
      output.emplace_back(bus); // but move from ctor bc might not want uniform or continuous...
    }
  }

  bool _run() override {
    bool outcome = false;
    // for(auto&& out: output) { // tho might want i-for and checking the ptrs then memcpy if needed...
    for(int i=0; i < buffers().size(); i++) { // tho might want i-for and checking the ptrs then memcpy if needed...
      auto&& out = output[i];
      if(out->CanShow()) {
        // if(out->Pixels() != buffer(i).get()) {
        //   // memcpy(out->Pixels(), buffer(i).get(), buffer(i).lengthBytes()); // tho ideally I mean they're just pointing to the same spot nahmean
        //   memcpy(out->Pixels(), buffer(i).get(), out->PixelsSize()); // tho ideally I mean they're just pointing to the same spot nahmean
        // }
        out->Dirty();
        out->Show();
        outcome = true;
      }
    }
    // FastLED.show();
    return outcome;
  }

  bool _ready() override {
    for(auto&& d: output) {
      if(d->CanShow())
        return true;
    }
    return false;
  }
};
