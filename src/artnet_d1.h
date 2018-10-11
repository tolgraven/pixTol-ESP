#pragma once

#include <Arduino.h> 	//needed by linters
#include <ArduinoOTA.h>
#include <Homie.h>
#include <LoggerNode.h>
#include <NeoPixelBrightnessBus.h>
#include <Ticker.h>
#include "state.h"
#include "util.h"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);

