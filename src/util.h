#ifndef ARTNET_UTIL
#define ARTNET_UTIL

#include <NeoPixelBrightnessBus.h>
#include <Homie.h>
#include <ArduinoOTA.h>

const extern RgbwColor black;
const extern RgbwColor white;
const extern RgbwColor red;
const extern RgbwColor orange;
const extern RgbwColor yellow;
const extern RgbwColor green;
const extern RgbwColor blue;


void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks);
void setupOTA(uint8_t numLeds);

#endif
