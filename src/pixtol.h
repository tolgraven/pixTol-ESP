#pragma once
#ifndef UNIT_TEST

#include <Arduino.h> //needed by linters
/* #include <Homie.h> */

#include "iot.h"
#include "device.h"
#include "scheduler.h"
#include "log.h"
#include "watchdog.h"

#include "debuglog.h"
#ifdef TOL_DEBUG
#include <GDBStub.h>
#endif

#endif
