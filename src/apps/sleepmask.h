#pragma once

/* #include <Homie.h> */
#include "renderstage.h"
#include "buffer.h"
/* #include "io/strip.h" */

// def one for ESP32, or whatever cheapest alt w ble is.
// OTOH, nah yeah you need bt.

// regish aviators with lcd tinting + super-light regular/uv-blocking
//  -low energy, none needed outside when changing adjustment
//  -tiny battery is then enough
// NO - reg sunglasses would suck
//  -BUT, have main unit with glass towards front, allowing whatever glasses or sunglasses to be worn, adding LCD dimmer and LEDs

// dock/case/puffy cover for sleepmask mode, providing
// larger battery/recharge
// LEDs for wake-up light
//   pushing glass away from face so can lay against (turning away from light after all being our prime antagonist...)
//   preventing ears from getting fucked up
//   hindering light leaking around eyes
//   headband so stays on...
//   some kind of cooling? ;) lol. also temåle massage and whispers real sensual like
// phone charger (duh)
//
// TECH:
// aviators
// LCD dim thing - learn.adafruit.com/3d-printed-electronic-sunglasses/overview adafruit.com/product/3330
// 100 mAh aux battery in small unit
// 2-3k mAh main battery in main unit
// also option to just connect ext battery/usb charge up a smaller one
//
//
//
// INTEGRATION:
// google cal et al bindings with no fiddle
//  =easy parse their standard parsing for flights!
// heart rate/sleep tracking wearables...
// start playback on android...
// also manual ifttt obvs


// TARGET MARKETS
// fucked up peeps...
// travelers
// couples
// ravers and other northwards people who go to bed after sun's up - lot easier than motorized blinds

// XXX how to dim once eyes open?
// def two versions
//  -traveler's - ironically much bulkier
//  -insomniac's - only wake-up light no auto glasses
