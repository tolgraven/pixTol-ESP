#pragma once

#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <LoggerNode.h>
#include <NeoPixelBrightnessBus.h>
#include <Ticker.h>
#include "util.h"
#include "battery.h"
#include "config.h"
#include "io/strip.h"
/* #include "color.h" */
#include "buffer.h"
#include "renderstage.h"
#include "renderer.h"

#define FW_BRAND "tolgrAVen"
#define FW_NAME "pixTol"
#define FW_VERSION "1.0.24"

#define LED_PIN             5  // D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define LED_STATUS_PIN      2
#define RX_PIN              3
#define TX_PIN              1
#define ARTNET_PORT      6454
#define SERIAL_BAUD     74880 // same rate as bootloader...


#define DMX_FN_CHS         12 // these going soon, when modulators etc
#define CH_DIMMER           1
#define CH_STROBE           2 // maybe use for strobe curves etc as well? 1-100 prob enough considering now approach without timers and limited to when rendering happens...
#define CH_HUE              3 // hue shift instead of strobe curvbes...
#define CH_ATTACK           4
#define CH_RELEASE          5
#define CH_BLEED            6
#define CH_NOISE            7
#define CH_ROTATE_FWD       8   //combine to one? 128 back, 128 fwd, more than we need really...
#define CH_ROTATE_BACK      9
#define CH_DIMMER_ATTACK   10
#define CH_DIMMER_RELEASE  11
#define CH_GAIN            12

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);

