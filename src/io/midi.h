#pragma once

#include "AppleMidi.h"
// easy networked MIDI in yo perfect for Functions on top of DMX oi
//
// Link other thing to get on ASAP. But need a concept of rhythm first heh.

// LOL looks fucking grim. just steal the basic impl crap and scale to/from floats.
APPLEMIDI_CREATE_DEFAULT_INSTANCE();

void setup() { // ...setup ethernet connection
  AppleMIDI.begin("test"); // 'test' will show up as the session name
}

void loop() {
  AppleMIDI.run();
  AppleMIDI.sendNoteOn(40, 55, 1); // Send MIDI note 40 on, velocity 55 on channel 1
}
