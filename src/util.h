#pragma once

#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <Homie.h>
#include <LoggerNode.h>
#include <ArtnetnodeWifi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include "strip.h"
#include "config.h"

const extern RgbwColor black, white, red, orange, yellow, green, blue;
const extern RgbColor blueZ;

extern HomieNode modeNode;

extern HomieNode outputNode;
extern HomieNode inputNode;


extern ArtnetnodeWifi artnet;
extern WiFiUDP udp;

bool outputHandler(const String& property, const HomieRange& range, const String& value);
bool inputHandler(const String& property, const HomieRange& range, const String& value);
void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*));

void blinkStrip(uint8_t numLeds, RgbwColor color, uint8_t blinks);
void blinkStatus(RgbColor color, uint8_t blinks);
void setupOTA(uint8_t numLeds);
void logIfChanged(int8_t level, std::string topic, std::string msg);
extern bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value);
extern bool broadcastHandler(const String& level, const String& value);
extern void onHomieEvent(const HomieEvent& event); //extern since is passed rather than called in artnet_d1,cpp

