#pragma once

// #include <Homie.h>

/*  Effect class provides interface for taking stream of 8bit data and manipulating the
 *  actual pixel data within Strip object.
 *  Dimmer, strobe, all dem
 *  Should keep track of own state so Strip can just iterate over its Effects
 *  
 *  Strobe toggles shutter on and off, so Strip exposes that and modulator does callback etc
 *  no extra render callbacks, just flip it and it happens next possible frame
 *
 *  Dimmer simply brightness. But dimmer att/rls (baked into same modulator?) complicates
 *
 * Rotate and hue etc seem tricky from outside, but expose strip methods and eh
 *  */


class Modulator {
  public:
    Modulator(const char* name); // and like a function pointer? cause like subclassing for each effect seems overkill
    uint8_t valueLastKeyframe;
    uint8_t priority; // hmm prob gets decided at Strip or yet higher

  private:
};

