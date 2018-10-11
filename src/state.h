#pragma once

#include <Arduino.h>
#include <LoggerNode.h>
#include <Ticker.h>
#include "config.h"
#include "battery.h"
#include "io/strip.h"
#include "renderstage.h"
#include "functions.h"


extern ConfigNode* cfg;
extern BatteryNode* battery;
extern Strip* s;
extern Functions* f;

extern Ticker* interTimer;
extern uint8_t interFrames; // ~40 us/led gives 5 ms for 1 universe of 125 leds. mirrored 144 rgb a 30 is 9 so 1-2 inters  should be alright.

extern int ctrl[25]; // 1-12 ctrl, 13-24 blend mqtt vs dmx on that ch, 25 tot blend?

extern uint8_t targetBuffer[512];
extern uint8_t buffers[3][512]; // prev, target, discard buffers

extern uint8_t universes;
extern uint8_t log_artnet;
extern int lastArtnetCode;
extern uint8_t dmxHz;
// uint8_t sourceBytes;
// uint16_t source

void initState();
