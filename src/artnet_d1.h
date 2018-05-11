#ifndef PIXTOL_ARTNET_D1
#define PIXTOL_ARTNET_D1

#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <LoggerNode.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>
#include <NeoPixelBrightnessBus.h>
#include <Ticker.h>
#include "util.h"
#include "config.h"
#include "strip.h"
// #include <ArdOSC.h>

#define FW_BRAND "tolgrAVen"
#define FW_NAME "pixTol"
#define FW_VERSION "1.0.11"

#define LED_PIN   D1  // D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define RX_PIN    3
#define TX_PIN    1
#define ARTNET_PORT     6454
#define SERIAL_BAUD    74880 // same rate as bootloader...

// these as enums instead? best if simply property of controlled thing, so fw then dynamically exposes correct fn chs
#define DMX_FN_CHS        12
#define CH_DIMMER          1
#define CH_STROBE          2 // maybe use for strobe curves etc as well? 1-100 prob enough considering now approach without timers and limited to when rendering happens...
#define CH_HUE             3 // hue shift instead of strobe curvbes...
#define CH_ATTACK          4
#define CH_RELEASE         5
#define CH_BLEED           6
#define CH_NOISE           7
#define CH_ROTATE_FWD      8   //combine to one? 128 back, 128 fwd, more than we need really...
#define CH_ROTATE_BACK     9
#define CH_DIMMER_ATTACK  10
#define CH_DIMMER_RELEASE 11
// more candidates:
// #define CH_GAIN
// #define CH_CHOPSHIT_INHALF_ASSEMBLE_BACKWARDS_THENAGAIN_ETC

#define CH_CONTROL        12
// FN_SAVE writes all current (savable) DMX settings to config.json
// then make cues in afterglow or buttons in max/mira to control settings per strip = no config file bs
// should also set up OSC support tho so can go that way = no fiddling with mapping out and translating shitty 8-bit numbers
// but stuff like flip could be both a config thing and performance effect so makes sense. anything else?
// bitcrunch, pixelate (halving resolution + zoom around where is grabbing)
#define FN_SAVE           1
#define FN_FLIP           2
#define FN_UNIVERSE_1     3 //within current subnet...
#define FN_UNIVERSE_2     4
#define FN_UNIVERSE_3     5
#define FN_UNIVERSE_4     6
#define FN_FRAMEGRAB_1    7
#define FN_FRAMEGRAB_2    8
#define FN_FRAMEGRAB_3    9
#define FN_FRAMEGRAB_4   10
#define FN_RESET        255
// then like 100-255 some nice enough curve driving x+y in a bilinear blend depending on amt of slots?
// BTW: Use bit in fun ch for some sort of midi clock equiv? Would potentially enable "loop last bar", "strobe 1/16" etc...

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);


#endif
