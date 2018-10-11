#pragma once

#include <Homie.h>
#include <ArtnetnodeWifi.h>
#include <Ticker.h>
#include <ArduinoOTA.h>

#include "state.h"
#include "io/strip.h"
#include "config.h"
#include "debug.h"

struct Colors {
  RgbwColor black =   RgbwColor(0, 0, 0, 0);
  RgbwColor white =   RgbwColor(150, 150, 150, 255);
  RgbwColor red =     RgbwColor(255, 40, 20, 35);
  RgbwColor orange =  RgbwColor(255, 112, 14, 30);
  RgbwColor yellow =  RgbwColor(255, 255, 30, 45);
  RgbwColor green =   RgbwColor(40, 255, 55, 32);
  RgbwColor blue =    RgbwColor(37, 85, 255, 32);
  RgbColor  blueZ =   RgbColor(30, 100, 255);
}; const Colors colors;

extern HomieNode modeNode;
extern HomieNode statusNode;
extern HomieNode outputNode;
extern HomieNode inputNode;

extern ArtnetnodeWifi artnet;

void initHomie();
bool outputNodeHandler(const String& property, const HomieRange& range, const String& value);
bool inputNodeHandler(const String& property, const HomieRange& range, const String& value);
bool controlsHandler(const HomieRange& range, const String& value);
bool colorHandler(const HomieRange& range, const String& value);
bool settingsHandler(const HomieRange& range, const String& value);

void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*));

void testStrip(uint8_t bitDepth, uint16_t ledCount);
void blinkStrip(uint16_t numLeds, RgbwColor color, uint8_t blinks);
void blinkStatus(RgbColor color, uint8_t blinks);
void setupOTA(uint8_t numLeds);
// extern
bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value);
// extern
bool broadcastHandler(const String& level, const String& value);
// extern
void onHomieEvent(const HomieEvent& event); //extern since is passed rather than called in artnet_d1,cpp

