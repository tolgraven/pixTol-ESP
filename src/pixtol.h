#pragma once

#include <Arduino.h> 	//needed by linters
#include <Homie.h>
#include <LoggerNode.h>
#include "state.h"
#include "util.h"
#include "iot.h"
#include "debug.h"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */
