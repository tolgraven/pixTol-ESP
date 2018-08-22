#pragma once

#include <Homie.h>
#include <LoggerNode.h>
#include <ArtnetnodeWifi.h>
#include <Ticker.h>
#include <ArduinoOTA.h>

#include "io/strip.h"
#include "config.h"

struct Colors {
  RgbwColor black =   RgbwColor(0, 0, 0, 0);
  RgbwColor white =   RgbwColor(50, 50, 50, 150);
  RgbwColor red =     RgbwColor(190, 30, 15, 25);
  RgbwColor orange =  RgbwColor(170, 85, 12, 28);
  RgbwColor yellow =  RgbwColor(160, 160, 20, 35);
  RgbwColor green =   RgbwColor(30, 190, 45, 25);
  RgbwColor blue =    RgbwColor(30, 70, 190, 25);
  RgbColor  blueZ =   RgbColor(25, 90, 220);
}; const Colors colors;

extern HomieNode modeNode;
extern HomieNode statusNode;
extern HomieNode outputNode;
extern HomieNode inputNode;

extern ArtnetnodeWifi artnet;

bool outputNodeHandler(const String& property, const HomieRange& range, const String& value);
bool inputNodeHandler(const String& property, const HomieRange& range, const String& value);
void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*));

void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks);
void blinkStatus(RgbColor color, uint8_t blinks);
void setupOTA(uint8_t numLeds);
extern bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value);
extern bool broadcastHandler(const String& level, const String& value);
extern void onHomieEvent(const HomieEvent& event); //extern since is passed rather than called in artnet_d1,cpp

