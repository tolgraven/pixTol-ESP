#pragma once

#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <Homie.h>
#include "modulator.h"
#include "outputter.h"
 
// try using blah = whah instead, "Usings can be templatized while typedefs cannot"
typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>         DmaGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>         DmaGRBW;
typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart800KbpsMethod>   UartGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>   UartGRBW;
typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod>     BitBangGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>     BitBangGRBW;

class iStripDriver {
  public:
    iStripDriver() {}
    // virtual ~iStripDriver() {}
    virtual void Begin() = 0;
    virtual void Show() = 0;
    virtual void SetBrightness(uint16_t b) = 0;
    virtual void SetPixelColor(uint16_t pixel, RgbColor color) = 0;
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color) = 0;
    virtual void GetPixelColor(uint16_t pixel) = 0;
    virtual void ClearTo(RgbColor color) = 0;
    virtual void ClearTo(RgbwColor color) = 0;
};

class StripRGB: public iStripDriver {
  public:
    explicit StripRGB(uint16_t ledCount): bus(ledCount) {}
    virtual ~StripRGB() {}

    virtual void Begin() { bus.Begin(); }
    virtual void Show() { bus.Show(); }
    virtual void SetBrightness(uint16_t b) { bus.SetBrightness(b); }
    virtual void SetPixelColor(uint16_t pixel, RgbColor color) {
      bus.SetPixelColor(pixel, color);
    }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color) {
      bus.SetPixelColor(pixel, RgbColor(color.R, color.G, color.B));
    }
    virtual void GetPixelColor(uint16_t pixel) { bus.GetPixelColor(pixel); }
    virtual void ClearTo(RgbColor color) { bus.ClearTo(color); }
    virtual void ClearTo(RgbwColor color) {}
  private:
    DmaGRB bus;
};

class StripRGBW: public iStripDriver {
  public:
    explicit StripRGBW(uint16_t ledCount): bus(ledCount) {}
    virtual ~StripRGBW() {}

    virtual void Begin() { bus.Begin(); }
    virtual void Show() { bus.Show(); }
    virtual void SetBrightness(uint16_t b) { bus.SetBrightness(b); }
    virtual void SetPixelColor(uint16_t pixel, RgbColor color) {
      bus.SetPixelColor(pixel, RgbwColor(color.R, color.G, color.B, 0));
    }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color) {
      bus.SetPixelColor(pixel, color);
    }
    virtual void GetPixelColor(uint16_t pixel) { bus.GetPixelColor(pixel); }
    virtual void ClearTo(RgbColor color) {}
    virtual void ClearTo(RgbwColor color) { bus.ClearTo(color); }
  private:
    DmaGRBW bus;
};

// class Strip: public Outputter {
class Strip {
  public:
    enum PixelBytes: uint8_t {
      Invalid = 0, Single = 1, RGB = 3, RGBW = 4, RGBWAU = 6
    };

    Strip();
    explicit Strip(iStripDriver& d): driver(d) { driver.Begin(); }
    ~Strip() {}

    void SetColor(RgbColor color) {
      driver.ClearTo(color);
      driver.Show();
    }
    void SetColor(RgbwColor color) {
      driver.ClearTo(color);
      driver.Show();
    }
    // void SetColor(HslColor color);

    void SetBrightness(uint16_t b) { driver.SetBrightness(b); }

    // Strip(PixelBytes fieldSize, uint16_t ledCount);

    void emit(uint8_t* data, uint16_t* length) {
      // driver.SetBuffer(data, length);
      driver.Show();
    }

  private:
    iStripDriver& driver;

    int getPixelIndex(int pixel);
    int getPixelFromIndex(int idx);

    // void updatePixels(uint8_t* data);
    // void updateFunctions(uint8_t* functions, bool isKeyframe);

    PixelBytes bytesPerLed;
    uint8_t ledsInData;
    uint8_t ledsInStrip;

    bool beGammaCorrect = false;
    NeoGamma<NeoGammaTableMethod> *colorGamma;
    bool beMirrored = false;
    bool beFolded = false;
    bool beFlipped = false;

    uint16_t maxFrameRate; // calculate from ledCount etc

    uint8_t brightness, lastBrightness;
    bool shutterOpen = true;

  protected:
};

