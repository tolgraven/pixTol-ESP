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
    virtual void ClearTo(RgbColor color, uint16_t first, uint16_t last) = 0;
    virtual void ClearTo(RgbwColor color, uint16_t first, uint16_t last) = 0;
    virtual bool CanShow() const = 0;                                                                 
    virtual bool IsDirty() const = 0;                                                                 
    virtual void Dirty() = 0;                                                                          
    virtual void ResetDirty() = 0;                                                                     
    virtual uint8_t* Pixels() = 0;                                                                         
    virtual size_t PixelsSize() const = 0;                                                               
    virtual size_t PixelSize() const = 0;                                                                
    virtual uint16_t PixelCount() const = 0;                                                               
    virtual void RotateLeft(uint16_t rotationCount) = 0;                                               
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last) = 0;                  
    virtual void ShiftLeft(uint16_t shiftCount) = 0;
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last) = 0;
    virtual void RotateRight(uint16_t rotationCount) = 0;
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) = 0;
    virtual void ShiftRight(uint16_t shiftCount) = 0;
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last) = 0;

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
    virtual void ClearTo(RgbwColor color) { ClearTo(RgbColor(color.R, color.G, color.B)); }
    virtual void ClearTo(RgbColor color, uint16_t first, uint16_t last) { bus.ClearTo(color, first, last); }
    virtual void ClearTo(RgbwColor color, uint16_t first, uint16_t last) { ClearTo(RgbColor(color.R, color.G, color.B), first, last); }
    virtual bool CanShow() const { return bus.CanShow(); }
    virtual bool IsDirty() const { return bus.IsDirty(); }
    virtual void Dirty() { bus.Dirty(); }
    virtual void ResetDirty() { bus.ResetDirty(); }
    virtual uint8_t* Pixels() { return bus.Pixels(); }
    virtual size_t PixelsSize() const { return bus.PixelsSize(); }
    virtual size_t PixelSize() const { return bus.PixelSize(); }
    virtual uint16_t PixelCount() const { return bus.PixelCount(); }
    virtual void RotateLeft(uint16_t rotationCount) { bus.RotateLeft(rotationCount); }
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last) { bus.RotateLeft(rotationCount, first, last); }
    virtual void ShiftLeft(uint16_t shiftCount) { bus.ShiftLeft(shiftCount); }
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last) { bus.ShiftLeft(shiftCount, first, last); }
    virtual void RotateRight(uint16_t rotationCount) { bus.RotateRight(rotationCount); }
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) {  bus.RotateRight(rotationCount, first, last);  }
    virtual void ShiftRight(uint16_t shiftCount) { bus.ShiftRight(shiftCount); }
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last) {  bus.ShiftRight(shiftCount, first, last);  }
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
      bus.SetPixelColor(pixel, RgbwColor(color));
    }
    virtual void SetPixelColor(uint16_t pixel, RgbwColor color) {
      bus.SetPixelColor(pixel, color);
    }
    virtual void GetPixelColor(uint16_t pixel) { bus.GetPixelColor(pixel); }
    virtual void ClearTo(RgbColor color) { ClearTo(RgbwColor(color)); }
    virtual void ClearTo(RgbwColor color) { bus.ClearTo(color); }
    virtual void ClearTo(RgbColor color, uint16_t first, uint16_t last) { ClearTo(RgbwColor(color), first, last); }
    virtual void ClearTo(RgbwColor color, uint16_t first, uint16_t last) { bus.ClearTo(color, first, last); }
    virtual bool CanShow() const { return bus.CanShow(); }
    virtual bool IsDirty() const { return bus.IsDirty(); }
    virtual void Dirty() { bus.Dirty(); }
    virtual void ResetDirty() { bus.ResetDirty(); }
    virtual uint8_t* Pixels() { return bus.Pixels(); }
    virtual size_t PixelsSize() const { return bus.PixelsSize(); }
    virtual size_t PixelSize() const { return bus.PixelSize(); }
    virtual uint16_t PixelCount() const { return bus.PixelCount(); }
    virtual void RotateLeft(uint16_t rotationCount) { bus.RotateLeft(rotationCount); }
    virtual void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last) { bus.RotateLeft(rotationCount, first, last); }
    virtual void ShiftLeft(uint16_t shiftCount) { bus.ShiftLeft(shiftCount); }
    virtual void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last) { bus.ShiftLeft(shiftCount, first, last); }
    virtual void RotateRight(uint16_t rotationCount) { bus.RotateRight(rotationCount); }
    virtual void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last) {  bus.RotateRight(rotationCount, first, last);  }
    virtual void ShiftRight(uint16_t shiftCount) { bus.ShiftRight(shiftCount); }
    virtual void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last) {  bus.ShiftRight(shiftCount, first, last);  }
  private:
    DmaGRBW bus;
};

// class Strip: public Outputter {
class Strip {
  public:
    enum PixelBytes {
      Invalid = 0, Single = 1, RGB = 3, RGBW = 4, RGBWAU = 6
    };

    Strip();
    explicit Strip(iStripDriver* d): driver(d) { driver->Begin(); }
    explicit Strip(PixelBytes fieldSize, uint16_t ledCount) {
      if(fieldSize == RGB) {
          driver = new StripRGB(ledCount);;
      } else if(fieldSize == RGBW) {
          driver = new StripRGBW(ledCount);;
      }
      driver->Begin();
    };
    ~Strip() {}

    void SetColor(RgbColor color) {
      driver->ClearTo(color);
      driver->Show();
    }
    void SetColor(RgbwColor color) {
      driver->ClearTo(color);
      driver->Show();
    }
    // void SetColor(HslColor color);

    void SetBrightness(uint16_t b) { driver->SetBrightness(b); }


    void emit(uint8_t* data, uint16_t* length) {
      // driver->SetBuffer(data, length);
      driver->Show();
    }

    iStripDriver* driver; // iStripDriver& driver; // figure out later...  ""As pointed out, failing to make a dtor virtual when a class is deleted through a base pointer could fail to invoke the subclass dtors (undefined behavior).

  private:

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

