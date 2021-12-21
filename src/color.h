#pragma once

#include <algorithm>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <NeoPixelBrightnessBus.h>
#pragma GCC diagnostic pop

#include "log.h"
#include <field.h>

namespace tol {

class Color: public Field { //should intentionally redefine non-virtual fns, since lots of ops on Field = dont want overhead, and we only act color-aware when explicitly Color, so no surprises.
  public:
  Color(): Color(0, 0, 0) {}
  Color(uint8_t r, uint8_t g, uint8_t b):
    Field(new uint8_t[3]{r, g, b}, 3) { } //safe since no copy by Field default
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w):
    Field(new uint8_t[4]{r, g, b, w}, 4) { }
  /* Color(uint8_t r, uint8_t g, uint8_t b, float alpha = 1.0): */
  /*   Field(new uint8_t[3]{r, g, b}, 3) { } //safe since no copy by Field default */
  /* Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w, float alpha = 1.0): */
  /*   Field(new uint8_t[4]{r, g, b, w}, 4) { } */
  // Color(uint8_t* data, FieldSize size, float alpha = 1.0):
  //   Field(data, size) { }

  Color(const RgbColor& color, uint8_t subPixels);
  Color(const RgbwColor& color, uint8_t subPixels);
  Color(const HtmlColor& color, uint8_t subPixels);
  Color(const HslColor& color, uint8_t subPixels);
  Color(const HsbColor& color, uint8_t subPixels);

  enum SubPixel { RED=0, GREEN, BLUE, WHITE };

  // void value(float absolute) override {
/*   void value(float absolute) {
    for(uint8_t p = 0; p < _size; p++) {
      // as HS(L) 0.5 = middle 1.0 = fully white etc, formula?
      // so not turn auto grayscale bs...
    }
    //float c[_size];
    HslColor hsl = HslColor(RgbColor(data[0], data[1], data[2])); // fix proper...
    hsl.L = absolute;
    // then convert back...
    float r = data[0]/255.0f, g = data[1]/255.0f, b=data[2]/255.0f;
    float h = hsl.H, s = hsl.S, l = hsl.L;
    if (s == 0.0f || l == 0.0f) r = g = b = l; // achromatic or black
    // else {
    //     float q = l < 0.5f ? l * (1.0f + s) : l + s - (l * s);
    //     float p = 2.0f * l - q;
    //     r = _CalcColor(p, q, h + 1.0f / 3.0f);
    //     g = _CalcColor(p, q, h);
    //     b = _CalcColor(p, q, h - 1.0f / 3.0f);
    // }
    // R = (uint8_t)(r * 255.0f);
    // G = (uint8_t)(g * 255.0f);
    // B = (uint8_t)(b * 255.0f);
  } */

  Color& rotateHue(float rotations) { //or just -1.0/1.0 full circle back/fwd?
    // auto result = std::minmax_element(rgb.begin(), rgb.end());
    // std::cout << "min element at: " << (result.first - rgb.begin()) << '\n';
    // std::cout << "max element at: " << (result.second - rgb.begin()) << '\n';
    float r = data[0]/255.0f, g = data[1]/255.0f, b=data[2]/255.0f;

    float max = std::max({r, g, b}), min = std::min({r, g, b});
    float h, s, l = (max + min) / 2.0f;
    if(max == min) {
      h = s = 0.0f;
    } else {
      float d = max - min;
      s = (l > 0.5f)? d / (2.0f - (max + min)): d / (max + min);

      if(r>g && r>b) h = (g-b) / d + (g<b? 6.0f: 0.0f);
      else if(g>b)   h = (b-r) / d + 2.0f;
      else           h = (r-g) / d + 4.0f;
      h /= 6.0f;
    }
    // HslColor hsl = HslColor(RgbColor(data[0], data[1], data[2])); // fix proper...
    // float hue = hsl.H +
    return *this;
  }

  // RgbColor& getNeoRgb(bool useAlpha = false) {
  //   if(useAlpha) return *(new RgbColor(data[0] * alpha, data[1] * alpha, data[2] * alpha));
  //   else return *(new RgbColor(data[0], data[1], data[2]));
  // } //temp test to use curr neopixelbus strip put
  // RgbwColor& getNeoRgbw(bool useAlpha = false) {
  //   if(useAlpha) return *(new RgbwColor(data[0] * alpha, data[1] * alpha, data[2] * alpha, data[3] * alpha));
  //   else return *(new RgbwColor(data[0], data[1], data[2], data[3]));
  // }

  // NOT FINISHED. Also weird as a stub? Want to gen and return as other color no, not in-place? that'd fuck shit
  Color& withSubPixels(uint8_t numSubPixels) {
    if(numSubPixels == _size) return *this;
    switch(numSubPixels) {
      case 3: //convert RGBW -> RGB
        break;
      case 4: //convert RGB -> RGBW
        break;
    }

    delete[] data; // delete[subPixels] data;
    _size = numSubPixels;
    data = new uint8_t[_size];
    for(uint8_t p=0; p<_size; p++) {
    }
    return *this;
  }
};

// // tables computed at runtime, for gamma correction and dithering.  RAM used = 256 * 3 * 3 + ledsInStrip * 3|4 bytes.
#define GAMMA_TABLE_SIZE 256
struct ColorComponentGammaTable {
  ColorComponentGammaTable(uint16_t maxPixels):
    maxPixels(maxPixels),
    low(new uint8_t[GAMMA_TABLE_SIZE]), high(new uint8_t[GAMMA_TABLE_SIZE]), fraction(new uint8_t[GAMMA_TABLE_SIZE]),
    error(new uint8_t[maxPixels]) {
      for(auto i=0; i<maxPixels; i++) error[i] = 0;
      lg.f(__func__, tol::Log::DEBUG, "Make table, free heap %u", ESP.getFreeHeap());
    }
  ~ColorComponentGammaTable() { delete [] low; delete [] high; delete [] fraction; delete [] error; }
  uint16_t maxPixels;
  //uint8_t low[256], high[256], fraction[256];
  uint8_t *low, *high, *fraction;
  uint8_t* error;
};

class GammaCorrection {
  public:
  GammaCorrection(uint16_t maxPixels = 125, uint8_t subPixels = 3, float gamma = 2.4, uint8_t maxBrightness = 255):
    maxPixels(maxPixels), gamma(gamma), subPixels(subPixels), maxBrightness(maxBrightness) {
    generate();
  }
  // ~GammaCorrection() { delete[] table; }
  ~GammaCorrection() { table.clear(); }

  void generate() {
    lg.f(__func__, tol::Log::DEBUG, "Gamma generate table");
    // if(table) delete[] table;
    // table = new ColorComponentGammaTable(maxPixels)[subPixels];
    // table = new ColorComponentGammaTable[subPixels](maxPixels);
    for(auto i=0; i<subPixels; i++) {
        table.emplace_back(maxPixels);
    }
    lg.f(__func__, tol::Log::DEBUG, "Done alloc");

    uint16_t i, j, n;
    for(uint8_t p = 0; p < subPixels; p++) {
      lg.f(__func__, tol::Log::DEBUG, "Outer loop");
      for(i = 0; i < 256; i++) {
        n = (uint16_t)(pow((double)i / 255.0, gamma) * (double)maxBrightness * 256.0 + 0.5); //Calc 16-bit gamma-corrected level
        table[p].low[i] = n >> 8;  // Store as 8-bit brightness level
        table[p].fraction[i] = n & 0xFF; // 'dither up' probability (error term)
        lg.f(__func__, tol::Log::DEBUG, "Inner loop 1");
      }
      delay(0);
      /* yield(); */
      for(i = 0; i < 256; i++) {// Second pass, calc 'hi' level for each (based on 'lo' value)
        n = table[p].low[i];
        // for(uint16_t j = i; (j < 256) && (table[p].low[j] <= n); j++); //doesnt this ; fuck up inner loop below?
        for(uint16_t j = i; (j < 256) && (table[p].low[j] <= n); j++) { //test around
          table[p].high[i] = table[p].low[j];
        }
        lg.f(__func__, tol::Log::DEBUG, "Inner loop 2");
        delay(0);
      }
      /* yield(); */
      /* delay(0); */
      lg.f(__func__, tol::Log::DEBUG, "Outer loop end ");
    }
  }
  uint16_t maxPixels;
  float gamma;
  uint8_t subPixels, maxBrightness;
  // ColorComponentGammaTable* table = nullptr;
  std::vector<ColorComponentGammaTable> table;
};

}
