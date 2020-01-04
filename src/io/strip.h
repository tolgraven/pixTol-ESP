#pragma once

#include <map>
#include <algorithm>
/* #include <functional> */
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include "renderstage.h"
#include "envelope.h"
#include "log.h"
#include "color.h"

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
using DmaGRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>;
using DmaGRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>;
using DmaRGBW     = NeoPixelBrightnessBus<NeoRgbwFeature, NeoEsp8266Dma800KbpsMethod>;
/* "If Async is used, then neither serial nor serial1 can be used due to ISR conflict" */
/* "maybe next release after required changes in esp8266 board support core is supported" */
/* serial dooes work tho so guess that was changed, but might still be fucking w debug? */
/* if not then it's def the shared interrupt handler, so change that code. */
/* using Uart0  = NeoEsp8266AsyncUart0800KbpsMethod; */
/* using Uart1  = NeoEsp8266AsyncUart1800KbpsMethod; */
using Uart0  = NeoEsp8266Uart0800KbpsMethod;
using Uart1  = NeoEsp8266Uart1800KbpsMethod;
using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  Uart1>;
using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, Uart1>;
using UartRGBW    = NeoPixelBrightnessBus<NeoRgbwFeature, Uart1>;
/* using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart0800KbpsMethod>; */
/* using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart0800KbpsMethod>; */
// using BitBangGRB  = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod>;
// using BitBangGRBW = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>;
using BitBangGRB  = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEspBitBangSpeed800Kbps>;
using BitBangGRBW = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEspBitBangSpeed800Kbps>;
#endif
#ifdef ESP32
using DmaGRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp32I2s1800KbpsMethod>;
using DmaGRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp32I2s1800KbpsMethod>;
using Dma0GRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp32I2s0800KbpsMethod>;
using Dma0GRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp32I2s0800KbpsMethod>;
// using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart800KbpsMethod>;
// using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>;
// using Uart0_RGBW  = NeoEsp8266AsyncUart0_800KbpsMethod;
// using Uart1_RGBW  = NeoEsp8266AsyncUart1_800KbpsMethod;
using BitBangGRB  = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEspBitBangSpeed800Kbps>;
using BitBangGRBW = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEspBitBangSpeed800Kbps>;
#endif

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

class StripRGB: public iStripDriver {
  public:
    explicit StripRGB(uint16_t ledCount):
      bus(ledCount) {
      lg.log("Strip", Log::DEBUG, "Created new: RGB");
    }
    virtual ~StripRGB() {}
    virtual void Begin()  {
      lg.log("Strip", Log::DEBUG, "Starting bus: RGB");
      bus.Begin();
    }
    virtual void Show()   { bus.Show(); }
    virtual void SetBrightness(uint16_t brightness)                { bus.SetBrightness(brightness); }
    virtual void SetPixelColor(uint16_t pixel, RgbColor color)     { bus.SetPixelColor(pixel, color); }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color)    { bus.SetPixelColor(pixel, RgbColor(color.R, color.G, color.B)); }
    virtual void GetPixelColor(uint16_t pixel, RgbColor& result)   { result = bus.GetPixelColor(pixel); }
    virtual void GetPixelColor(uint16_t pixel, RgbwColor& result)  { result = RgbwColor(bus.GetPixelColor(pixel)); }

    virtual iStripDriver& ClearTo(RgbColor color)                                   { bus.ClearTo(color); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color)                                  { ClearTo(RgbColor(color.R, color.G, color.B)); return *this; }
    virtual iStripDriver& ClearTo(RgbColor color, uint16_t first, uint16_t last)    { bus.ClearTo(color, first, last); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color, uint16_t first, uint16_t last)   { ClearTo(RgbColor(color.R, color.G, color.B), first, last); return *this; }
    virtual bool CanShow() const        { return bus.CanShow(); }
    virtual bool IsDirty() const        { return bus.IsDirty(); }
    virtual void Dirty()                { bus.Dirty(); }
    virtual void ResetDirty()           { bus.ResetDirty(); }
    virtual uint8_t* Pixels()           { return bus.Pixels(); }
    virtual size_t PixelsSize() const   { return bus.PixelsSize(); }
    virtual size_t PixelSize() const    { return bus.PixelSize(); }
    virtual uint16_t PixelCount() const { return bus.PixelCount(); }

    virtual void RotateLeft(uint16_t rotationCount)                                 { bus.RotateLeft(rotationCount); }
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)  { bus.RotateLeft(rotationCount, first, last); }
    virtual void ShiftLeft(uint16_t shiftCount)                                     { bus.ShiftLeft(shiftCount); }
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)      { bus.ShiftLeft(shiftCount, first, last); }
    virtual void RotateRight(uint16_t rotationCount)                                { bus.RotateRight(rotationCount); }
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) { bus.RotateRight(rotationCount, first, last); }
    virtual void ShiftRight(uint16_t shiftCount)                                    { bus.ShiftRight(shiftCount); }
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)     { bus.ShiftRight(shiftCount, first, last); }
  private:
    DmaGRB bus;
};

class StripRGBW: public iStripDriver {
  public:
    explicit StripRGBW(uint16_t ledCount):
      bus(ledCount) {
      lg.log("Strip", Log::DEBUG, "Created new: RGBW");
    }
    virtual ~StripRGBW() {}
    virtual void Begin()  {
      lg.log("Strip", Log::DEBUG, "Starting bus: RGBW");
      bus.Begin();
    }
    virtual void Show()   { bus.Show(); }
    virtual void SetBrightness(uint16_t brightness)               { bus.SetBrightness(brightness); }
    virtual void SetPixelColor(uint16_t pixel, RgbColor color) {
      // XXX this is the big one. Good conversion that doesn't just compensate with white
      // zero-sum but adds and desaturates nicely so everything immune from allout ugly.
      // And receive HSL with onboard saturation ADSR compressor like 90% limit 30ms response yada yad
      bus.SetPixelColor(pixel, RgbwColor(color));
    }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color)   { bus.SetPixelColor(pixel, color); }
    virtual void GetPixelColor(uint16_t pixel, RgbColor& result) {
      RgbwColor rgbw = bus.GetPixelColor(pixel);
      result = RgbColor(rgbw.R, rgbw.G, rgbw.B);
    }
    virtual void GetPixelColor(uint16_t pixel, RgbwColor& result) { result = bus.GetPixelColor(pixel); }

    virtual iStripDriver& ClearTo(RgbColor color)                                   { ClearTo(RgbwColor(color)); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color)                                  { bus.ClearTo(color);        return *this; }
    virtual iStripDriver& ClearTo(RgbColor color, uint16_t first, uint16_t last)    { ClearTo(RgbwColor(color), first, last); return *this; }
    virtual iStripDriver& ClearTo(RgbwColor color, uint16_t first, uint16_t last)   { bus.ClearTo(color, first, last); return *this; }

    virtual bool CanShow() const        { return bus.CanShow(); }
    virtual bool IsDirty() const        { return bus.IsDirty(); }
    virtual void Dirty()                { bus.Dirty(); }
    virtual void ResetDirty()           { bus.ResetDirty(); }
    virtual uint8_t* Pixels()           { return bus.Pixels(); }
    virtual size_t PixelsSize() const   { return bus.PixelsSize(); }
    virtual size_t PixelSize()  const   { return bus.PixelSize(); }
    virtual uint16_t PixelCount() const { return bus.PixelCount(); }

    virtual void RotateLeft(uint16_t rotationCount)                                 { bus.RotateLeft(rotationCount); }
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)  { bus.RotateLeft(rotationCount, first, last); }
    virtual void ShiftLeft(uint16_t shiftCount)                                     { bus.ShiftLeft(shiftCount); }
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)      { bus.ShiftLeft(shiftCount, first, last); }
    virtual void RotateRight(uint16_t rotationCount)                                { bus.RotateRight(rotationCount); }
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) { bus.RotateRight(rotationCount, first, last); }
    virtual void ShiftRight(uint16_t shiftCount)                                    { bus.ShiftRight(shiftCount); }
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)     { bus.ShiftRight(shiftCount, first, last); }
  private:
    DmaGRBW bus;
    /* UartGRBW bus; */
    /* DmaRGBW bus; */
};


class Strip: public Outputter {
  public:
  Strip(): Strip("Strip RGBW 125", (uint8_t)RGBW, 125) {}

  Strip(iStripDriver* d):
    Outputter("Strip, ext driver", d->PixelSize(), d->PixelCount()),
    _driver(d), externalDriver(true) {
      initDriver();
  }
  Strip(const String& id, uint8_t fieldSize, uint16_t ledCount):
    Outputter(id, fieldSize, ledCount) {
      initDriver();
      buffer().setPtr(_driver->Pixels()); //dependent on output type... and doesnt seem to work for DMA either heh
  }
  virtual ~Strip() { if(!externalDriver) delete _driver; }

  virtual void setFieldCount(uint16_t newFieldCount) {
    if(newFieldCount != _fieldCount) {
      _fieldCount = newFieldCount;
      initDriver();
    }
  }

  void setColor(RgbColor color)  { _driver->ClearTo(color);           show(); }
  void setColor(RgbwColor color) { _driver->ClearTo(color);           show(); }
  void setColor(HslColor color)  { _driver->ClearTo(RgbColor(color)); show(); }
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
    _driver->GetPixelColor(pixel, color); // and back it goes
  }
  void getPixelColor(uint16_t pixel, RgbColor& color) {
    pixel = getIndexOfField(pixel);
    _driver->GetPixelColor(pixel, color);
  }
  Color getPixelColor(uint16_t pixel) {
    pixel = getIndexOfField(pixel);
    RgbwColor color;
    _driver->GetPixelColor(pixel, color);
    return Color(color, 4);
  }

  void adjustPixel(uint16_t pixel, String action, uint8_t value);

  /* void rotateBack(uint16_t pixels) { _driver->RotateLeft(pixels); } */
  /* void rotateFwd(uint16_t pixels) {  _driver->RotateRight(pixels); } */
  /* void rotateBack(float fraction) {  _driver->RotateLeft(_fieldCount * fraction); } */
  /* void rotateFwd(float fraction) {   _driver->RotateRight(_fieldCount * fraction); } */
  void rotateBack(float fraction) {  rotateBackPixels = _fieldCount * fraction; }
  void rotateFwd(float fraction) {   rotateFwdPixels = _fieldCount * fraction; } //XXX TEMP
  void rotate(float fraction) {
    fraction = fraction - (int)fraction; //so can go on forever
    if(fraction > 0) _driver->RotateRight(_fieldCount * fraction);
    else _driver->RotateLeft(_fieldCount * -fraction);
  }
  void shift(float fraction) {
    fraction = fraction - (int)fraction; //so can go on forever
    if(fraction > 0) _driver->ShiftRight(_fieldCount * fraction);
    else _driver->ShiftLeft(_fieldCount * -fraction);
  }
  void rotateHue(float amount);

  bool ready() { return _driver->CanShow(); }

  uint16_t microsTilReady() {
    if(ready()) return 0;
    else return (throughput * sizeInBytes() + cooldown) - (micros() - timeLastRun);
  }
  bool run();

  iStripDriver& getDriver() { return *_driver; }
  uint8_t* destBuffer()      { return _driver->Pixels(); }

  void mirror(bool state = true);
  void fold(bool state = true) { _fold = state; }
  void flip(bool state = true) { _flip = state; }
  void gammaCorrect(bool state = true);

  uint8_t ledsInStrip; //or as fn/just calc on the fly off mirror()?
  uint8_t brightness = 255; //should prob remain as attribute, post any in-buffer mods (gain)... right now w only one place to adjust, gets stuck in one when dimming at night like
  // ^ should get written every now and then (at least if set by non-dmx means) so gets restored after power cycle

  private:
  uint16_t rotateFwdPixels = 0, rotateBackPixels = 0;
  iStripDriver* _driver = nullptr;
  bool externalDriver = false;
  uint16_t cooldown = 50; //micros
  bool swapRedGreen = false; //true;

  uint8_t pixelBrightnessCutoff = 8, totalBrightnessCutoff = 6; //in lieu of dithering, have less of worse than nothing

  void applyGain();
  void initDriver();
  bool show();

  uint16_t getIndexOfField(uint16_t position);
  uint16_t getFieldOfIndex(uint16_t field) { return field; } // wasnt defined = vtable breaks
};


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

/*   std::map<String, RgbwColor> colors; */

/*   void generatePalette() { */
/*     colors["black"]  = RgbwColor(0, 0, 0, 0); */
/*     colors["white"]  = RgbwColor(150, 150, 150, 255); */
/*     colors["red"]    = RgbwColor(255, 15, 5, 8); */
/*     colors["orange"] = RgbwColor(255, 40, 20, 35); */
/*     colors["yellow"] = RgbwColor(255, 112, 12, 30); */
/*     colors["green"]  = RgbwColor(20, 255, 22, 35); */
/*     colors["blue"]   = RgbwColor(37, 85, 255, 32); */
/*   } */

/*   bool color(const String& name = "black") { */
/*     if(colors.find(name) != colors.end()) { */
/*       lg.dbg("Set color: " + name); */
/*       s->setColor(colors[name]); */
/*       return true; */
/*     } */
/*     return false; */
/*   } */

/*   void gradient(const String& from = "white", const String& to = "black") { */
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
/*   /1*   /2* std::vector<String[2]> args = { {"black", "blue"}, {"blue", "green"}, {"green", "red"}, {"red", "orange"} }; *2/ *1/ */

/*   /1*   /2* color("black"); *2/ *1/ */
/*   /1*   /2* gradient("black", "blue"); *2/ *1/ */
/*   /1*   /2* color("blue"); *2/ *1/ */
/*   /1*   /2* gradient("blue", "green"); *2/ *1/ */
/*   /1*   /2* gradient("green", "red"); *2/ *1/ */
/*   /1*   /2* gradient("red", "orange"); *2/ *1/ */
/*   /1*   /2* gradient("orange", "black"); *2/ *1/ */
/*   /1* } *1/ */

/*   void blink(const String& colorName, uint8_t blinks = 1, bool restore = true, uint16_t numLeds = 0) { */
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
