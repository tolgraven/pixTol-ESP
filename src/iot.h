#pragma once

#include <Homie.h>
#include <LoggerNode.h>

#include "state.h"
#include "io/strip.h"
#include "config.h"

extern HomieNode modeNode, statusNode, outputNode, inputNode, colorNode;

void initHomie();

bool outputNodeHandler(const String& property, const HomieRange& range, const String& value);
bool inputNodeHandler(const String& property, const HomieRange& range, const String& value);
bool colorNodeHandler(const String& property, const HomieRange& range, const String& value);

bool controlsHandler(const HomieRange& range, const String& value);
bool blendHandler(const HomieRange& range, const String& value);
bool settingsHandler(const HomieRange& range, const String& value);

bool globalInputHandler(const HomieNode& node, const String& property, const HomieRange& range, const String& value);
bool broadcastHandler(const String& level, const String& value);
void onHomieEvent(const HomieEvent& event); //extern since is passed rather than called in artnet_d1,cpp
