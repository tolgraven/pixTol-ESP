#include "color.h"


static float _CalcColor(float p, float q, float t) { //read up on exactly what this does and how mod to RGBW...
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;

    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;

    if (t < 0.5f) return q;

    if (t < 2.0f / 3.0f) return p + ((q - p) * (2.0f / 3.0f - t) * 6.0f);

    return p;
}
// fillGamma(2.7, 255, loR, hiR, fracR); // Initialize gamma tables to default values (can be overriden later).
// fillGamma(2.7, 255, loG, hiG, fracG);
// fillGamma(2.7, 255, loB, hiB, fracB);
// (err buffers don't need init, they'll naturally reach equilibrium)


// #define MAX_LEDS_TEMP_XXX 125 //temp
// uint8_t loR[256], hiR[256], fracR[256], errR[MAX_LEDS], // so, group as struct and malloc way to go?
//         loG[256], hiG[256], fracG[256], errG[MAX_LEDS],
//         loB[256], hiB[256], fracB[256], errB[MAX_LEDS],
//         loW[256], hiW[256], fracW[256], errW[MAX_LEDS];
//
// gamma/dither tables for color component. Gamma, max brightness (e.g. 2.7, 255),
// pointers to 'lo', 'hi' and 'frac' tables to fill. XXX Fix for RGBW...
// void fillGamma(float gamma, uint8_t maxBrightness, uint8_t *lo, uint8_t *hi, uint8_t *frac) { }


// ColorRGBW::ColorRGBW(ColorRGB from) {
  // RgbwColor& i = from.color;

  // float tM = max(i.R, max(i.G, i.B)); //Get the maximum between R, G, and B
  //
  // if(!tM) {
  //     color = RgbwColor(0, 0, 0, 0); return; //If the maximum value is 0, immediately return pure black.
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
