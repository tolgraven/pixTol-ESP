#pragma once

#include <NeoPixelBrightnessBus.h>

class iColor {
  public:
    iColor() {}

    virtual uint8_t CalculateBrightness() = 0;
    // virtual void SetBrightness() = 0; // this build off global strip dimmer, just scale individual color ihn same way
    virtual void Darken(uint8_t delta) = 0;
    virtual void Lighten(uint8_t delta) = 0;

    static void LinearBlend(RgbColor& result, RgbColor left, RgbColor right, float progress) {
      result = RgbColor::LinearBlend(left, right, progress);
    }
    static void BilinearBlend(RgbColor& result, RgbColor c00, RgbColor c01, RgbColor c10, RgbColor c11, float x, float y) {
      result = RgbColor::BilinearBlend(c00, c01, c10, c11, x, y);
    }
    static void LinearBlend(RgbwColor& result, RgbwColor left, RgbwColor right, float progress) {
      result = RgbwColor::LinearBlend(RgbwColor(left), RgbwColor(right), progress);
    }
    static void BilinearBlend(RgbwColor& result, RgbwColor c00, RgbwColor c01, RgbwColor c10, RgbwColor c11, float x, float y) {
      result = RgbwColor::BilinearBlend(RgbwColor(c00), RgbwColor(c01), RgbwColor(c10), RgbwColor(c11), x, y);
    }
};

class ColorRGB: public iColor {
  public:
    explicit ColorRGB(uint8_t r, uint8_t g, uint8_t b): color(r, g, b) {}
    // explicit ColorRGB(const ColorRGB& from);
    virtual uint8_t CalculateBrightness() { return color.CalculateBrightness(); }
    virtual void Darken(uint8_t delta) { color.Darken(delta); }
    virtual void Lighten(uint8_t delta) { color.Lighten(delta); }
    // virtual ColorRGB operator+(int add) { return color.Lighten(add); }
    // virtual ColorRGB operator-(int sub) { return color.Darken(sub); }

  RgbColor color;
};

class ColorRGBW: public iColor {
  public:
    explicit ColorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w): color(r, g, b, w) {}
    // explicit ColorRGBW(ColorRGB from);
    virtual uint8_t CalculateBrightness() { return color.CalculateBrightness(); }
    virtual void Darken(uint8_t delta) { color.Darken(delta); }
    virtual void Lighten(uint8_t delta) { color.Lighten(delta); }

  RgbwColor color;
};

// ColorRGBW::ColorRGBW(ColorRGB from) {
  // RgbwColor& i = from.color;

  // float tM = Math.Max(i.R, Math.Max(i.G, i.B)); //Get the maximum between R, G, and B
  //
  // if(!tM) {
  //     color = RgbwColor(0, 0, 0, 0); //If the maximum value is 0, immediately return pure black.
  //     return;
  // }
  // //This section serves to figure out what the color with 100% hue is (?? nonsense statement)
  // float multiplier = 255.0f / tM;
  // float hR = Ri * multiplier;
  // float hG = Gi * multiplier;
  // float hB = Bi * multiplier;  
  // //This calculates the Whiteness (not strictly speaking Luminance) of the color
  // float M = Math.Max(hR, Math.Max(hG, hB));
  // float m = Math.Min(hR, Math.Min(hG, hB));
  // float Luminance = ((M + m) / 2.0f - 127.5f) * (255.0f/127.5f) / multiplier;
  // //Calculate the output values
  // int Wo = Convert.ToInt32(Luminance);
  // int Bo = Convert.ToInt32(Bi - Luminance);
  // int Ro = Convert.ToInt32(Ri - Luminance);
  // int Go = Convert.ToInt32(Gi - Luminance);
  //
  // Wo = Wo < 0? 0: Wo > 255? 255: Wo;
  // Bo = Bo < 0? 0: Bo > 255? 255: Bo;
  // Go = Go < 0? 0: Go > 255? 255: Go;
  // Ro = Ro < 0? 0: Ro > 255? 255: Ro;
  // color = RgbwColor(Ro, Go, Bo, Wo);
// }
