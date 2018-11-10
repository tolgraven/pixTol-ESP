#pragma once

#include <LoggerNode.h>
#include <ArtnetnodeWifi.h>

#include "config.h"
#include "battery.h"
#include "io/strip.h"
#include "renderstage.h"
#include "buffer.h"
#include "functions.h"
#include "device.h"

extern ConfigNode* cfg;
extern BatteryNode* battery;
extern Strip* s;
extern Blinky* b;
extern Functions* f;
extern Updater* ota;
extern Updater* homieUpdater;

extern uint16_t start;
extern uint32_t gotFrameAt, keyFrameInterval; // abs time (micros) last frame read

extern Buffer* targetFunctions;
// extern PixelBuffer* targetBuffer; extern PixelBuffer* wasBuffer;
extern PixelBuffer *targetBuffer, *wasBuffer, *outBuffer; // extern uint8_t buffers[3][512]; // prev, target, discard buffers

extern HomieEvent* lastEvent;
extern uint8_t disconnects;

extern ArtnetnodeWifi* artnet;
extern int lastArtnetOpCode;
void initArtnet(const String& name, uint8_t numPorts, uint8_t startingUniverse, void (*callback)(uint16_t, uint16_t, uint8_t, uint8_t*));
extern void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);

void initDevice();
void initUpdaters();
void initState();
void initScheduler();
