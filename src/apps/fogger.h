#pragma once

#include "config.h"

// AFA FOGGER: good poc thing
// particulate sensor in same unit can check fog air density
// works best if also put on some other units around space
// vibration sensor could ghetto recognize when tank empty
// but better, fluid level sensor, keeping track of tank. EASY!
// could kit some lights around nozzle as well, all the rage innit...

// BED PROJECT:
// 100w? power supply mounted internally
// 2-3 outlets each side - multi fit
// 2 usb
// maybe a little button or dimmer...
// lights mounted inside, usual plan...


// PixtolConfig fog("Smokr");
// fog.
//
// // or vibration sensor could ghetto recognize when tank empty haha
//
//                         // particulate sensor in same unit can check fog air density
// Sensor airParticulates; // better if also put on some other units around space, so register to get updates from any others on network with same type
// Sensor fluidLevel;      // keeping track of tank, warning then throttling and shutting off when nears empty. EASY!
// // Sensor is surely an Inputter with fieldCount 1 no?
// Inputter* artnet; //ArtnetInput
// // also want to build in normal standalone fog remote level/timer auto stuff
// // also controllable by mqtt -> app, siri, etc...
//
// Generator* standaloneMode;
//
// // possible outs:
// OutputPin fogActivator;
// Strip nozzleLighter;    // could kit some lights around nozzle as well, all the rage innit...
// DMXOutput artnetForwarder; // makes sense to use same unit to hook up other cabled DMX fixtures
