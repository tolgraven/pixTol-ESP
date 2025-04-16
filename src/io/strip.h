#pragma once

#include <map>
#include <algorithm>
#include <functional>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#pragma GCC diagnostic pop

#include "renderstage.h"
#include "envelope.h"
#include "log.h"
#include "color.h"

namespace tol {
// // ESP32 doesn't define ICACHE_RAM_ATTR
// #ifndef ICACHE_RAM_ATTR
// #define ICACHE_RAM_ATTR IRAM_ATTR
// #endif
// for esp8266, due to linker overriding the ICACHE_RAM_ATTR for cpp files, these methods are
// moved into a C file so the attribute will be applied correctly
// >> this may have been fixed and is no longer a requirement <<
// extern "C" void ICACHE_RAM_ATTR bitbang_send_pixels_800(uint8_t* pixels, uint8_t* end, uint8_t pin);
// extern "C" void ICACHE_RAM_ATTR bitbang_send_pixels_400(uint8_t* pixels, uint8_t* end, uint8_t pin);

#ifdef ESP8266
using GRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>;
using GRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>;
using DmaRGBW     = NeoPixelBrightnessBus<NeoRgbwFeature, NeoEsp8266Dma800KbpsMethod>;
/* "If Async is used, then neither serial nor serial1 can be used due to ISR conflict" */
/* "maybe next release after required changes in esp8266 board support core is supported" */
/* serial dooes work tho so guess that was changed, but might still be fucking w debug? */
/* if not then it's def the shared interrupt handler, so change that code. */
using Uart0       = NeoEsp8266Uart0800KbpsMethod;
using Uart1       = NeoEsp8266Uart1800KbpsMethod;
using AUart0      = NeoEsp8266AsyncUart0800KbpsMethod;
using AUart1      = NeoEsp8266AsyncUart1800KbpsMethod;
using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  Uart1>;
using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, Uart1>;
// using AUart       = NeoPixelBrightnessBus<NeoGrbFeature, T>;

using Uart0_GRB     = NeoPixelBrightnessBus<NeoGrbFeature,  Uart0>;
using Uart0_GRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, Uart0>;
  // using UartRGBW    = NeoPixelBrightnessBus<NeoRgbwFeature, Uart1>;
/* using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart0800KbpsMethod>; */
/* using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart0800KbpsMethod>; */
#endif
#ifdef ESP32
// using GRB      = NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod>;
// using GRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, Neo800KbpsMethod>;
using GRB      = NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod>;
using GRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp32I2s0800KbpsMethod>;
// using Dma0GRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp32I2s0800KbpsMethod>;
// using Dma0GRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp32I2s0800KbpsMethod>;
// using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart800KbpsMethod>;
// using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>;
// using Uart0_RGBW  = NeoEsp8266AsyncUart0_800KbpsMethod;
// using Uart1_RGBW  = NeoEsp8266AsyncUart1_800KbpsMethod;
#endif

template<class T>
using Bus4        = NeoPixelBrightnessBus<NeoGrbwFeature, T>;
// union UartBus4 {
//   Bus4<AUart0> bus0;
//   Bus4<AUart1> bus1;
//   UartBus4(Bus4<AUart0> b): bus(b) {}
//   UartBus4(Bus4<AUart1> b): bus2(b) {}
// };

// union UartBus4 { // tho no real diff when bypassing all their color mechanisms welp
//   UartGRBW
// };

class iStripDriver {
  public:
    iStripDriver() {}
    virtual ~iStripDriver() {}
    virtual void Begin()  = 0;
    virtual void Show()   = 0;
    virtual void SetBrightness(uint16_t brightness)               = 0;
    virtual void SetPixelColor(uint16_t pixel, RgbColor color)    = 0;
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color)   = 0;
    virtual void GetPixelColor(uint16_t pixel, RgbColor& result)  = 0;
    virtual void GetPixelColor(uint16_t pixel, RgbwColor& result) = 0;

    virtual iStripDriver& ClearTo(RgbColor color)   = 0; // change eg these to return ref to self, chain like d.ClearTo(blue).SetBrightness(100).Show(); ?
    virtual iStripDriver& ClearTo(RgbwColor color)  = 0;
    virtual iStripDriver& ClearTo(RgbColor color, uint16_t first, uint16_t last)    = 0;
    virtual iStripDriver& ClearTo(RgbwColor color, uint16_t first, uint16_t last)   = 0;

    virtual bool CanShow() const        = 0;
    virtual bool IsDirty() const        = 0;
    virtual void Dirty()                = 0;
    virtual void ResetDirty()           = 0;
    virtual uint8_t* Pixels()           = 0;
    virtual size_t PixelsSize() const   = 0;
    virtual size_t PixelSize() const    = 0;
    virtual uint16_t PixelCount() const = 0;

    virtual void RotateLeft(uint16_t rotationCount)                                 = 0;
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)  = 0;
    virtual void ShiftLeft(uint16_t shiftCount)                                     = 0;
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)      = 0;
    virtual void RotateRight(uint16_t rotationCount)                                = 0;
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) = 0;
    virtual void ShiftRight(uint16_t shiftCount)                                    = 0;
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)     = 0;
};

// class StripRGB: public iStripDriver {
//   public:
//     explicit StripRGB(uint16_t ledCount):
//       bus(ledCount) {
//       DEBUG("Created new: RGBW");
//     }
//     virtual ~StripRGB() {}
//     virtual void Begin()  {
//       DEBUG("Starting bus: RGBW");
//       bus.Begin();
//     }
//     virtual void Show()   { bus.Show(); }
//     virtual void SetBrightness(uint16_t brightness)                { bus.SetBrightness(brightness); }
//     virtual void SetPixelColor(uint16_t pixel, RgbColor color)     { bus.SetPixelColor(pixel, color); }
//     virtual void SetPixelColor(uint16_t pixel, RgbwColor color)    { bus.SetPixelColor(pixel, RgbColor(color.R, color.G, color.B)); }
//     virtual void GetPixelColor(uint16_t pixel, RgbColor& result)   { result = bus.GetPixelColor(pixel); }
//     virtual void GetPixelColor(uint16_t pixel, RgbwColor& result)  { result = RgbwColor(bus.GetPixelColor(pixel)); }

//     virtual iStripDriver& ClearTo(RgbColor color)                                   { bus.ClearTo(color); return *this; }
//     virtual iStripDriver& ClearTo(RgbwColor color)                                  { ClearTo(RgbColor(color.R, color.G, color.B)); return *this; }
//     virtual iStripDriver& ClearTo(RgbColor color, uint16_t first, uint16_t last)    { bus.ClearTo(color, first, last); return *this; }
//     virtual iStripDriver& ClearTo(RgbwColor color, uint16_t first, uint16_t last)   { ClearTo(RgbColor(color.R, color.G, color.B), first, last); return *this; }
//     virtual bool CanShow() const        { return bus.CanShow(); }
//     virtual bool IsDirty() const        { return bus.IsDirty(); }
//     virtual void Dirty()                { bus.Dirty(); }
//     virtual void ResetDirty()           { bus.ResetDirty(); }
//     virtual uint8_t* Pixels()           { return bus.Pixels(); }
//     virtual size_t PixelsSize() const   { return bus.PixelsSize(); }
//     virtual size_t PixelSize() const    { return bus.PixelSize(); }
//     virtual uint16_t PixelCount() const { return bus.PixelCount(); }

//     virtual void RotateLeft(uint16_t rotationCount)                                 { bus.RotateLeft(rotationCount); }
//     virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)  { bus.RotateLeft(rotationCount, first, last); }
//     virtual void ShiftLeft(uint16_t shiftCount)                                     { bus.ShiftLeft(shiftCount); }
//     virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)      { bus.ShiftLeft(shiftCount, first, last); }
//     virtual void RotateRight(uint16_t rotationCount)                                { bus.RotateRight(rotationCount); }
//     virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) { bus.RotateRight(rotationCount, first, last); }
//     virtual void ShiftRight(uint16_t shiftCount)                                    { bus.ShiftRight(shiftCount); }
//     virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)     { bus.ShiftRight(shiftCount, first, last); }
//   private:
//     GRB bus;
// };

class StripRGBW: public iStripDriver {
  public:
    // explicit StripRGBW(uint16_t ledCount):
    //   bus(ledCount) {
    //   lg.log("Strip", Log::DEBUG, "Created new: RGBW");
    // }
    explicit StripRGBW(uint16_t ledCount, uint8_t port) //:
      // bus(port == 0? UartBus4(AUart0<AUart0>): )
      // bus( UartBus4(port == 0? AUart0<AUart0>: AUart<AUart1>) )
      // bus( UartBus4((port == 0)? AUart0<AUart0>: AUart<AUart1>) )
      // bus( UartBus4(AUart<(port == 0)? AUart0: AUart1>(ledCount)) )
      // bus( UartBus4((port == 0)? Bus4<AUart0>(ledCount): Bus4<AUart1>(ledCount)) )
    {
      // if(port == 0) bus0 = new Bus4<AUart0>(ledCount);

      // if(port == 0) bus0 = new GRBW(ledCount);
#ifdef ESP8266
      if(port == 0) bus0 = new GRBW(ledCount);
#endif
#ifdef ESP32
      if(port == 0) bus0 = new GRBW(ledCount, 27);
#endif

      // else if(port == 1) bus1 = new Bus4<AUart1>(ledCount);
      // bus(j)
      // bus(ledCount) {
      DEBUG("Created new: RGBW");
    }
    virtual ~StripRGBW() {}
    virtual void Begin()  {
      // DEBUG("Starting bus: RGBW");
      if(bus0) bus0->Begin(); else bus1->Begin();
      // DEBUG("Started bus: RGBW");
    }
    virtual void Show()   { if(bus0) bus0->Show(); else bus1->Show(); }
    virtual void SetBrightness(uint16_t brightness)               { if(bus0) bus0->SetBrightness(brightness); else bus1->SetBrightness(brightness); }
    virtual void SetPixelColor(uint16_t pixel, RgbColor color) {
      // XXX this is the big one. Good conversion that doesn't just compensate with white
      // zero-sum but adds and desaturates nicely so everything immune from allout ugly.
      // And receive HSL with onboard saturation ADSR compressor like 90% limit 30ms response yada yad
      if(bus0) bus0->SetPixelColor(pixel, RgbwColor(color)); else bus1->SetPixelColor(pixel, RgbwColor(color));
    }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color)   { if(bus0) bus0->SetPixelColor(pixel, color); else bus1->SetPixelColor(pixel, color); }
    virtual void GetPixelColor(uint16_t pixel, RgbColor& result) {
      RgbwColor rgbw;
      if(bus0) rgbw = bus0->GetPixelColor(pixel); else rgbw = bus1->GetPixelColor(pixel);
      result = RgbColor(rgbw.R, rgbw.G, rgbw.B);
    }
    virtual void GetPixelColor(uint16_t pixel, RgbwColor& result) {
      if(bus0) result = bus0->GetPixelColor(pixel); else result = bus1->GetPixelColor(pixel); }

    virtual iStripDriver& ClearTo(RgbColor color)                                   { ClearTo(RgbwColor(color)); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color)                                  { if(bus0) bus0->ClearTo(color); else bus1->ClearTo(color);        return *this; }
    virtual iStripDriver& ClearTo(RgbColor color, uint16_t first, uint16_t last)    { ClearTo(RgbwColor(color), first, last); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color, uint16_t first, uint16_t last)   { if(bus0) bus0->ClearTo(color, first, last); else bus1->ClearTo(color, first, last); return *this; }

    virtual bool CanShow() const        { if(bus0) return bus0->CanShow(); else return bus1->CanShow(); }
    virtual bool IsDirty() const        { if(bus0) return bus0->IsDirty(); else return bus1->IsDirty(); }
    virtual void Dirty()                { if(bus0) bus0->Dirty(); else return bus1->Dirty(); }
    virtual void ResetDirty()           { if(bus0) bus0->ResetDirty(); else return bus1->ResetDirty(); }
    virtual uint8_t* Pixels()           { if(bus0) return bus0->Pixels(); else return bus1->Pixels(); }
    virtual size_t PixelsSize() const   { if(bus0) return bus0->PixelsSize(); else return bus1->PixelsSize(); }
    virtual size_t PixelSize()  const   { if(bus0) return bus0->PixelSize(); else return bus1->PixelSize(); }
    virtual uint16_t PixelCount() const { if(bus0) return bus0->PixelCount(); else return bus1->PixelCount(); }

    virtual void RotateLeft(uint16_t rotationCount)                                 { if(bus0) bus0->RotateLeft(rotationCount); else bus1->RotateLeft(rotationCount); }
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)  { if(bus0) bus0->RotateLeft(rotationCount, first, last); else bus1->RotateLeft(rotationCount, first, last); }
    virtual void ShiftLeft(uint16_t shiftCount)                                     { if(bus0) bus0->ShiftLeft(shiftCount); else bus1->ShiftLeft(shiftCount); }
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)      { if(bus0) bus0->ShiftLeft(shiftCount, first, last); else bus1->ShiftLeft(shiftCount, first, last); }
    virtual void RotateRight(uint16_t rotationCount)                                { if(bus0) bus0->RotateRight(rotationCount); else bus1->RotateRight(rotationCount); }
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) { if(bus0) bus0->RotateRight(rotationCount, first, last); else bus1->RotateRight(rotationCount, first, last); }
    virtual void ShiftRight(uint16_t shiftCount)                                    { if(bus0) bus0->ShiftRight(shiftCount); else bus1->ShiftRight(shiftCount); }
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)     { if(bus0) bus0->ShiftRight(shiftCount, first, last); else bus1->ShiftRight(shiftCount, first, last); }
  private:
    // std::vector<UartBus4> bus;
    // UartBus4 bus;
    // Bus4<AUart0>* bus0;
    // Bus4<AUart1>* bus1;
    GRBW* bus1; // temp...
    // GRBW*  bus2;
    GRBW* bus0;

    // UartBus4 bus;
    /* UartGRBW bus; */
    /* DmaRGBW bus; */
};


class Strip: public Outputter {
  public:
  // Strip(): Strip("Strip RGBW 125", (uint8_t)RGBW, 125) {}

  // Strip(iStripDriver* d):
  //   Outputter("Strip, ext driver", d->PixelSize(), d->PixelCount()),
  //   _driver(d), externalDriver(true) {
  //     initDriver();
  // }
  Strip(const std::string& id, uint8_t fieldSize, uint16_t ledCount, uint8_t bufferCount = 1):
    Outputter(id, fieldSize, ledCount, bufferCount) {
      initDriver();
      for(int i=0; i < bufferCount; i++) {
        buffer(i).setPtr(_driver[i]->Pixels()); //dependent on output type... and doesnt seem to work for DMA either heh
      }
      // buffer().setPtr(_driver->Pixels()); //dependent on output type... and doesnt seem to work for DMA either heh
  }
  // virtual ~Strip() { if(!externalDriver) delete _driver; }
  virtual ~Strip() { if(!externalDriver) _driver.clear(); }

  virtual void setFieldCount(uint16_t newFieldCount) {
    if(newFieldCount != fieldCount()) {
      ChunkedContainer::setFieldCount(newFieldCount);
      initDriver();
    }
  }

  // void setColor(RgbColor color)  { for(auto d: _driver) d->ClearTo(color);           show(); }
  // void setColor(RgbwColor color) { for(auto d: _driver) d->ClearTo(color);           show(); }
  // void setColor(HslColor color)  { for(auto d: _driver) d->ClearTo(RgbColor(color)); show(); }
  void setGradient(RgbwColor* from, RgbwColor* to);

  void setPixelColor(uint16_t pixel, RgbwColor color);
  void setPixelColor(uint16_t pixel, RgbColor color);
  void setPixelColor(uint16_t pixel, Color color);
  void setPixelColor(uint16_t pixel, uint16_t pixelToCopy) {
    RgbwColor color;
    getPixelColor(pixelToCopy, color);
    setPixelColor(pixel, color);
  }
  void getPixelColor(uint16_t pixel, RgbwColor& color) {
    pixel = getIndexOfField(pixel);
    _driver[0]->GetPixelColor(pixel, color); // and back it goes
  }
  void getPixelColor(uint16_t pixel, RgbColor& color) {
    pixel = getIndexOfField(pixel);
    _driver[0]->GetPixelColor(pixel, color);
  }
  Color getPixelColor(uint16_t pixel) {
    pixel = getIndexOfField(pixel);
    RgbwColor color;
    _driver[0]->GetPixelColor(pixel, color);
    return Color(color, 4);
  }

  void adjustPixel(uint16_t pixel, std::string action, uint8_t value);

  /* void rotateBack(uint16_t pixels) { _driver->RotateLeft(pixels); } */
  /* void rotateFwd(uint16_t pixels) {  _driver->RotateRight(pixels); } */
  /* void rotateBack(float fraction) {  _driver->RotateLeft(fieldCount() * fraction); } */
  /* void rotateFwd(float fraction) {   _driver->RotateRight(fieldCount() * fraction); } */
  void rotateBack(float fraction) {  rotateBackPixels = fieldCount() * fraction; }
  void rotateFwd(float fraction) {   rotateFwdPixels = fieldCount() * fraction; } //XXX TEMP
  void rotate(float fraction) {
    fraction = fraction - (int)fraction; //so can go on forever
    if(fraction > 0) _driver[0]->RotateRight(fieldCount() * fraction);
    else _driver[0]->RotateLeft(fieldCount() * -fraction);
  }
  void shift(float fraction) {
    fraction = fraction - (int)fraction; //so can go on forever
    if(fraction > 0) _driver[0]->ShiftRight(fieldCount() * fraction);
    else _driver[0]->ShiftLeft(fieldCount() * -fraction);
  }
  void rotateHue(float amount);


  // uint32_t microsTilReady() override {
  //   if(_ready()) return 0; /// not actually for us to call tho... the below stuff should be done in Runnable when stuff's set.
  //   else return (throughput * sizeInBytes() + cooldown) - passed();
  // }

  iStripDriver& getDriver(int i = 0) { return *_driver[i]; }
  uint8_t* destBuffer(int i = 0)      { return _driver[i]->Pixels(); }

  void mirror(bool state = true);
  void fold(bool state = true) { _fold = state; }
  void flip(bool state = true) { _flip = state; }
  void gammaCorrect(bool state = true);

  uint8_t ledsInStrip; //or as fn/just calc on the fly off mirror()?
  uint8_t brightness = 255; //should prob remain as attribute, post any in-buffer mods (gain)... right now w only one place to adjust, gets stuck in one when dimming at night like
  // ^ should get written every now and then (at least if set by non-dmx means) so gets restored after power cycle

  private:
  uint16_t rotateFwdPixels = 0, rotateBackPixels = 0;
  bool _mirror = false, _fold = false, _flip = false, _gammaCorrect = false; //these will go in Transform
  NeoGamma<NeoGammaTableMethod>* colorGamma;

  std::vector<iStripDriver*> _driver; // somehow forgot about why we put ptrs in vectors. ABCs. fun detour not
  bool externalDriver = false;
  uint16_t cooldown = 50; //micros
  bool swapRedGreen = false; //true;

  uint8_t pixelBrightnessCutoff = 8, totalBrightnessCutoff = 6; //in lieu of dithering, have less of worse than nothing

  void applyGain();
  void initDriver();

  uint16_t getIndexOfField(uint16_t position);
  uint16_t getFieldOfIndex(uint16_t field) { return field; } // wasnt defined = vtable breaks

  bool _run();
  bool _ready() override {
    for(auto d: _driver) {
      if(_driver[0]->CanShow())
        return true;
    }
    return false;
  }
};

}

/* class Blinky { */
/*   public: */
/*   Strip* s; */
/*   Blinky(uint8_t fieldSize, uint16_t ledCount): */
/*     Blinky(static_cast<Outputter&>(*(new Strip("Strip tester", fieldSize, ledCount)))) { */
/*   /1* Blinky(uint8_t fieldSize, uint16_t ledCount) { *1/ */
/*   /1*   s(new Strip("Strip tester", fieldSize, ledCount)) { *1/ */
/*   /1*   generatePalette(); *1/ */
/*   } */
/*   Blinky(Outputter& o): //get rid of this fucker already!! */
/*     s(&static_cast<Strip&>(o)) { */
/*     generatePalette(); */
/*     /1* using namespace std::placeholders; *1/ */
/*     /1* auto cb = std::bind(&Blinky::testCB, this, _1); *1/ */
/*   } */
/*   ~Blinky() { delete s; } */

/*   std::map<std::string, RgbwColor> colors; */

/*   void generatePalette() { */
/*     colors["black"]  = RgbwColor(0, 0, 0, 0); */
/*     colors["white"]  = RgbwColor(150, 150, 150, 255); */
/*     colors["red"]    = RgbwColor(255, 15, 5, 8); */
/*     colors["orange"] = RgbwColor(255, 40, 20, 35); */
/*     colors["yellow"] = RgbwColor(255, 112, 12, 30); */
/*     colors["green"]  = RgbwColor(20, 255, 22, 35); */
/*     colors["blue"]   = RgbwColor(37, 85, 255, 32); */
/*   } */

/*   bool color(const std::string& name = "black") { */
/*     if(colors.find(name) != colors.end()) { */
/*       lg.dbg("Set color: " + name); */
/*       s->setColor(colors[name]); */
/*       return true; */
/*     } */
/*     return false; */
/*   } */

/*   void gradient(const std::string& from = "white", const std::string& to = "black") { */
/*     lg.dbg(from + "<>" + to + " gradient requested"); */
/*     /1* RgbwColor* one = colors.find(from) != colors.end()? &colors[from]: nullptr; //colors["white"]; *1/ */
/*     /1* RgbwColor* two = colors.find(to)   != colors.end()? &colors[to]:   nullptr; //colors["black"]; *1/ */
/*     RgbwColor* one = colors.find(from) != colors.end()? &colors[from]: &colors["white"]; */
/*     RgbwColor* two = colors.find(to)   != colors.end()? &colors[to]:   &colors["white"]; //colors["black"]; */
/*     s->setGradient(one, two); */
/*   } */

/*   void test() { */
/*     lg.logf("Blinky", Log::DEBUG, "Run gradient test"); */

/*     /1* color("black"); //homieDelay(1); //homiedelay (and also reg delay? causing crash :/) *1/ */
/*     /1* gradient("black", "blue"); //homieDelay(3); *1/ */
/*     /1* color("blue"); //homieDelay(5); *1/ */
/*     /1* gradient("blue", "green"); //homieDelay(3); *1/ */
/*     /1* gradient("green", "red"); //homieDelay(3); *1/ */
/*     /1* gradient("red", "orange"); //homieDelay(3); *1/ */
/*     /1* gradient("orange", "black"); *1/ */
/*   } */
/*   /1* void testCB(int stage=0) { *1/ */
/*   /1*   lg.logf("Blinky", Log::DEBUG, "Run gradient CB test"); *1/ */
/*   /1*   /2* std::vector<auto> calls = {color("black"), gradient("black", "blue")}; *2/ *1/ */
/*   /1*   /2* std::vector<std::string[2]> args = { {"black", "blue"}, {"blue", "green"}, {"green", "red"}, {"red", "orange"} }; *2/ *1/ */

/*   /1*   /2* color("black"); *2/ *1/ */
/*   /1*   /2* gradient("black", "blue"); *2/ *1/ */
/*   /1*   /2* color("blue"); *2/ *1/ */
/*   /1*   /2* gradient("blue", "green"); *2/ *1/ */
/*   /1*   /2* gradient("green", "red"); *2/ *1/ */
/*   /1*   /2* gradient("red", "orange"); *2/ *1/ */
/*   /1*   /2* gradient("orange", "black"); *2/ *1/ */
/*   /1* } *1/ */

/*   void blink(const std::string& colorName, uint8_t blinks = 1, bool restore = true, uint16_t numLeds = 0) { */
/*     uint16_t originalLedCount = s->fieldCount(); */
/*     if(numLeds && numLeds != s->fieldCount()) s->fieldCount(numLeds); //so can do like clear to end of entire strip if garbage there and not using entire... */

/*     uint16_t sBytes = s->fieldCount() * s->fieldSize(); */
/*     uint8_t was[sBytes]; */
/*     if(s->destBuffer() && restore) memcpy(was, s->destBuffer(), sBytes); */

/*     for(int8_t b = 0; b < blinks; b++) { */
/*       color(colorName); */
/*       homieDelay(10); */
/*       if(restore) memcpy(s->destBuffer(), was, sBytes); */
/*       else        color("black"); */
/*       homieDelay(5); */
/*     } */
/*     if(originalLedCount != s->fieldCount()) s->fieldCount(originalLedCount); */
/*   } */
/* }; */
