#pragma once

#include <map>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include "renderstage.h"
#include "envelope.h"
// #include "color.h"
#include "util.h"

using DmaGRB      = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>;
using DmaGRBW     = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>;
using UartGRB     = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart800KbpsMethod>;
using UartGRBW    = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>;
using BitBangGRB  = NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod>;
using BitBangGRBW = NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>;

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
    explicit StripRGB(uint16_t ledCount): bus(ledCount) {}
    virtual ~StripRGB() {}
    virtual void Begin()  { bus.Begin(); }
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
    explicit StripRGBW(uint16_t ledCount): bus(ledCount) {}
    virtual ~StripRGBW() {}
    virtual void Begin()  { bus.Begin(); }
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
};


class Strip: public Outputter {
  public:
    Strip():
      Outputter("Default strip", RGBW, 120), bytesPerLed(RGBW) {}

    Strip(iStripDriver* d):
      Outputter("Strip, ext driver", LEDS(d->PixelSize()), d->PixelCount()),
      driver(d), externalDriver(true), bytesPerLed(LEDS(d->PixelSize())) {
        initDriver();
    }
    Strip(const String& id, LEDS fieldSize, uint16_t ledCount):
      Outputter(id, (uint8_t)fieldSize, ledCount),
      bytesPerLed(fieldSize) {
        initDriver();
    }
    virtual ~Strip() { if(externalDriver) delete driver; } //actually dumb? if providing externally then might be using for other stuff

    // will have to get rid of below ones if want Outputter clean? nah cant be But would be alright, just conveniance stuff no?
    void setLedCount(uint16_t ledCount) {
      _fieldCount = ledCount;
      initDriver();
    }
    void setBrightness(uint16_t newBrightness) {
      if(newBrightness < totalBrightnessCutoff) newBrightness = 0; //until dithering...
      brightness = newBrightness;
      // driver->SetBrightness(brightness); //XXX be non-destructive & copy to new buffer before fucking? or just use ext buffers anyways
      // cause remember if this way order also becomes important, can't set level before pixelbuffer
      // ready, tho easily fixabble skipping SetBrightness here and running it later...
    }

    void setColor(RgbColor color)  { driver->ClearTo(color);           show(); }
    void setColor(RgbwColor color) { driver->ClearTo(color);           show(); }
    void setColor(HslColor color)  { driver->ClearTo(RgbColor(color)); show(); }

    void setGradient(RgbwColor* from, RgbwColor* to) { // XXX do with N points/colors...
      for(uint16_t pixel = 0; pixel < _fieldCount; pixel++) {
        if(!from) getPixelColor(pixel, *to); // like, if either nullptr, instead blend with whatever current...
        if(!to)   getPixelColor(pixel, *from); // also use alpha once color lib a'go
        RgbwColor color = RgbwColor::LinearBlend(*from, *to, (float)pixel/_fieldCount);
        setPixelColor(pixel, color);
      }
      show();
    }

    void setPixelColor(uint16_t pixel, RgbwColor color) {
      pixel = getIndexOfField(pixel);
      if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
      if(_gammaCorrect) color = colorGamma->Correct(color);

      driver->SetPixelColor(pixel, color);
			if(_mirror) driver->SetPixelColor(ledsInStrip-1 - pixel, color);
    }

    void setPixelColor(uint16_t pixel, RgbColor color) {
      pixel = getIndexOfField(pixel);
      if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
      if(_gammaCorrect) color = colorGamma->Correct(color);

      driver->SetPixelColor(pixel, color);
      if(_mirror) driver->SetPixelColor(ledsInStrip-1 - pixel, color);
    }

    void getPixelColor(uint16_t pixel, RgbwColor& color) {
      pixel = getIndexOfField(pixel);
      driver->GetPixelColor(pixel, color); // and back it goes
    }
    void getPixelColor(uint16_t pixel, RgbColor& color) {
      pixel = getIndexOfField(pixel);
      driver->GetPixelColor(pixel, color); // and back it goes
    }

    void adjustPixel(uint16_t pixel, String action, uint8_t value) { //should go off callback really...
      if(_fieldSize == 4) {
        RgbwColor color;
        getPixelColor(pixel, color);
        if(action == "lighten") {
          color.Lighten(value);
        } else if(action == "darken") {
          color.Darken(value);
        }
        setPixelColor(pixel, color);
      }
    }

    void rotateBack(uint16_t pixels) { driver->RotateLeft(pixels); }
    void rotateFwd(uint16_t pixels) {  driver->RotateRight(pixels); }
    void rotateBack(float fraction) {  driver->RotateLeft(_fieldCount * fraction); }
    void rotateFwd(float fraction) {   driver->RotateRight(_fieldCount * fraction); }

    bool show() {
      if(!_isActive) return false;
      if(driver->CanShow()) {
        driver->SetBrightness(brightness);
        driver->Show();
        return true;
      }
      droppedFrames++;
      return false;
    }

    virtual bool ready() { return driver->CanShow(); }
    virtual uint16_t microsTilReady() {
      if(ready()) return 0;
      else {
        return (throughput * length() + 50) - (micros() - timeLastRun);
      }
    }

    virtual bool run() {
      memcpy(driver->Pixels(), buffers[0]->get(), buffers[0]->length());
      driver->Dirty();
      show();
    }

    void updateWithEnvelope(uint8_t* data, BlendEnvelope& e, float progress) { // XXX also pass fraction in case interpolating >2 frames
      int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
      float brightnessFraction = 255 / (brightness? brightness: 1); // ahah can't believe divide by zero got me

      for(int t = 0; t < length(); t += _fieldSize, pixel++) {
        uint8_t subPixel[_fieldSize];
        for(uint8_t i = 0; i < _fieldSize; i++) subPixel[i] = data[t+i];

        if(_fieldSize == 3) { // when this is moved to input->pixelbuffer stage there will be multiple configs: format of input (RGB, RGBW, HSL etc) and output (WS/SK). So all sources can be used with all endpoints.
          RgbColor color = RgbColor(subPixel[0], subPixel[1], subPixel[2]);
          RgbColor lastColor;
          getPixelColor(pixel, lastColor); // get from pixelbus since we can resolve dimmer-related changes
          bool brighter = color.CalculateBrightness() >
                          (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
          color = RgbColor::LinearBlend(color, lastColor, (brighter ? e.A(progress) : e.R(progress)));
          setPixelColor(pixel, color);

        } else if(_fieldSize == 4) {
          RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);
          RgbwColor lastColor;
          getPixelColor(pixel, lastColor);
          bool brighter = color.CalculateBrightness() >
                          (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
          color = RgbwColor::LinearBlend(color, lastColor, (brighter? e.A(progress): e.R(progress)));

        setPixelColor(pixel, color);
      }
    }
  }

  // virtual void setActive(bool state = true, int8_t bufferIndex = -1) { }
  iStripDriver& getDriver() { return *driver; }
  uint8_t* getBuffer()      { return driver->Pixels(); }

  Strip& mirror(bool state = true) {
    _mirror = state;
    initDriver(); // affects output size so yeah
    return *this;
  }
  Strip& fold(bool state = true) { _fold = state; return *this; }
  Strip& flip(bool state = true) { _flip = state; return *this; }
  Strip& gammaCorrect(bool state = true) {
    _gammaCorrect = state;
    if(state) colorGamma = new NeoGamma<NeoGammaTableMethod>;
    else delete colorGamma;
    return *this;
  }

  uint8_t ledsInStrip;
  uint8_t brightness;

private:
  iStripDriver* driver = nullptr; // iStripDriver& driver; // figure out later...  ""As pointed out, failing to make a dtor virtual when a class is deleted through a base pointer could fail to invoke the subclass dtors (undefined behavior).
  bool externalDriver = false;

  LEDS bytesPerLed;
  bool _mirror = false, _fold = false, _flip = false, _gammaCorrect = false;
  NeoGamma<NeoGammaTableMethod> *colorGamma;

  uint16_t maxFrameRate; // calculate from ledCount etc, use inherited common vars and methodds for that...

  uint8_t pixelBrightnessCutoff = 8, totalBrightnessCutoff = 6; //in lieu of dithering, have less of worse than nothing

  void initDriver() {
    ledsInStrip = _mirror? _fieldCount * 2: _fieldCount;

    if(!externalDriver) {
      if(driver) delete driver;

      if(_fieldSize == RGB)        driver = new StripRGB(ledsInStrip);
      else if(_fieldSize == RGBW)  driver = new StripRGBW(ledsInStrip);
    }
    driver->Begin();
  }

  virtual uint16_t getIndexOfField(uint16_t position) {
    if(position >= _fieldCount) position = _fieldCount-1; //bounds...
    else if(position < 0) position = 0;

    if(_fold) { // pixelidx differs from pixel recieved
      if(position % 2) position /= 2; // since every other pixel is from opposite end
      else position = _fieldCount-1 - position/2;
    }
    if(_flip) position = _fieldCount-1 - position;
    return position;
  }
  virtual uint16_t getFieldOfIndex(uint16_t field) { return field; } // wasnt defined = vtable breaks
};


class Blinky {
  public:
  Blinky(LEDS fieldSize, uint16_t ledCount):
    Blinky(new Strip("Strip tester", fieldSize, ledCount)) {
  }
  Blinky(Strip* s): s(s) { generatePalette(); }
  ~Blinky() { delete s; }

  std::map<String, RgbwColor> colors;

  void generatePalette() {
    colors["black"]  = RgbwColor(0, 0, 0, 0);
    colors["white"]  = RgbwColor(150, 150, 150, 255);
    colors["red"]    = RgbwColor(255, 15, 5, 8);
    colors["orange"] = RgbwColor(255, 40, 20, 35);
    colors["yellow"] = RgbwColor(255, 112, 12, 30);
    colors["green"]  = RgbwColor(20, 255, 22, 35);
    colors["blue"]   = RgbwColor(37, 85, 255, 32);
  }

  bool color(const String& name = "black") {
    if(colors.find(name) != colors.end()) {
        s->setColor(colors[name]);
          return true;
      }
      return false;
  }

  void gradient(const String& from = "white", const String& to = "black") {
    RgbwColor* one = colors.find(from) != colors.end()? &colors[from]: nullptr; //colors["white"];
    RgbwColor* two = colors.find(to)   != colors.end()? &colors[to]:   nullptr; //colors["black"];
    s->setGradient(one, two);
    // RgbwColor one = colors.find(from) != colors.end()? colors[from]: colors["white"];
    // RgbwColor two = colors.find(to)   != colors.end()? colors[to]:   colors["black"];
    // s->setGradient(&one, &two);
  }

  void test() {
    LN.logf(__func__, LoggerNode::DEBUG, "Run gradient test");
    color("black");
    gradient("black", "blue");
    gradient("blue", "green");
    gradient("green", "red");
    gradient("red", "orange");
    gradient("orange", "black");
    // homiedelay (and also reg delay? causing crash :/)
    // color("black"); homieDelay(50);
    // gradient("black", "blue"); homieDelay(300);
    // // color("blue");  homieDelay(100);
    // gradient("blue", "green"); homieDelay(300);
    // gradient("green", "red"); homieDelay(300);
    // gradient("red", "orange"); homieDelay(300);
    // gradient("orange", "black");
  }

  void blink(const String& colorName, uint8_t blinks = 1, bool restore = true, uint16_t numLeds = 0) {
    uint16_t originalLedCount = s->fieldCount();
    if(numLeds && numLeds != s->fieldCount()) s->setLedCount(numLeds); //so can do like clear to end of entire strip if garbage there and not using entire...

    uint16_t sBytes = s->fieldCount() * s->fieldSize();
    uint8_t was[sBytes];
    if(s->getBuffer() && restore) {
      memcpy(was, s->getBuffer(), sBytes);
    }

    for(int8_t b = 0; b < blinks; b++) {
      color(colorName);
      homieDelay(100);
      if(restore) {
        memcpy(s->getBuffer(), was, sBytes);
      } else {
        color("black");
      }
      homieDelay(50);
    }
    if(originalLedCount != s->fieldCount()) s->setLedCount(originalLedCount);
  }

  void fadeRealNiceLike() { //proper, worthy, impl of above...
    // scheduler->registerAnimation(lala); //nahmean
  }

  Strip* s;
};
