#pragma once

#include "Arduino.h"
#include <LoggerNode.h>

enum CH: uint8_t {
 chDimmer = 1, chStrobe, chHue, chAttack, chRelease, chBleed, chNoise,
 chRotateFwd, chRotateBack, chDimmerAttack, chDimmerRelease, chGain
};

class Envelope {
  private:
    // uint8_t lastAttackRaw, lastReleaseRaw;
    bool invert = false; // no invert = high-is-low
    void adjustDirection() {
      if(invert) {
        attack = 1.0 - attack;
        release = 1.0 - release;
      }
    }
  public:
    float blend; // the value at 0 attack/release. Later move away from envelope to renderer?
    float attack = 0.0;
    /* float hold = 0.0; */
    /* float decay = 0.0; //yeeehh */
    /* float sustain = 1.0; */
    float release = 0.0; // now as fraction. later fix so can take ms, seconds or whatever
    uint8_t attackDivExtra, releaseDivExtra;

    Envelope(float blend = 0.5, uint8_t attackDivExtra = 0, uint8_t releaseDivExtra = 0, bool invert = false):
      blend(blend), attackDivExtra(attackDivExtra), releaseDivExtra(releaseDivExtra), invert(invert) {
        adjustDirection();
      }

    void set(uint8_t a, uint8_t r) {
      // if(a != lastAttackRaw) { // not really necessary
        attack = blend - blend * ((float)a/(255 + attackDivExtra)); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      // }
      // if(r != lastAttackRaw) {
        release = blend - blend * ((float)r/(255 + releaseDivExtra)); // dont ever quite arrive like this, two runs at max = 75% not 100%...
      // }
      adjustDirection();

      // lastAttackRaw = a;
      // lastReleaseRaw = r;
    }
};

class Functions {
  private:

  class Dimmer {
    private:
    uint8_t lastBrightness = 0;
    uint8_t cutoff = 6; // in lieu of dithering...
    Envelope e;

    public:
    Dimmer(Envelope e): e(e) {}

    uint8_t brightness;

    void update(uint8_t value) {
      bool brighter = value > lastBrightness; //sooo never true since we haven't updated it yet hahh
      float diff = (value - lastBrightness) * (brighter ? e.attack: e.release);

      if(diff > 0.1f && diff < 1.0f){
        diff = 1.0f;
      } else if(diff < -0.1f && diff > -1.0f){
        diff = -1.0f;
      }

      brightness = lastBrightness + diff; // crappy workaround, store dimmer internally as 16bit or float instead.
      if(brightness < cutoff) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
      else if(brightness > 255) brightness = 255;

      lastBrightness = brightness;

    }
    void update(uint8_t value, uint8_t a, uint8_t r) {
      e.set(a, r);
      update(value);
    }

  };

  class Shutter { //test if sep classes make sense
    private:
    uint8_t lastStrobe;

    float hzMin, hzMax;
    int targetTickMs, onTime, strobePeriod, fadeTime;
    uint8_t eachFrameFade;
    float strobeHz;
    static constexpr float onFraction = 3.5;
    static const int onMaxMs = 120;
    static constexpr float fadeFraction = 7; //starting to look decent
    static const int fadeMaxMs = 60;

    Envelope e; // incorporate

    public:
    Shutter(uint16_t targetFPS = 200, float hzMin = 1.0, float hzMax = 12.5):
     hzMin(hzMin), hzMax(hzMax), targetTickMs(1000/targetFPS),
     open(true), amount(255), lastStrobe(0) {};

    bool open; // could also have shutter as float not bool and fade using it. 0 closed, 1 open, between in transition
    int amount;

    void initStrobeState(uint8_t value) {

      strobeHz = (hzMax-hzMin) * (value-1) / (255-1) + hzMin;
      strobePeriod = 1000 / strobeHz;   // 1 = 1 hz, 255 = 10 hz, to test

      resetOnTime();
      resetFadeTime();

      eachFrameFade = 255 / 30; // strobeTickerFading; //XXX
      amount = 255;

      LN.logf("Strobe", LoggerNode::DEBUG, "strobeHz %f, strobePeriod %d, onTime %d", strobeHz, strobePeriod, onTime);

      open = false;
    }

    void resetOnTime() { //these would be in maps so only need one function
      onTime = strobePeriod / onFraction;
      if(onTime > onMaxMs) onTime = onMaxMs;
    }

    void resetFadeTime() {
      fadeTime = strobePeriod / fadeFraction;
      if(fadeTime > fadeMaxMs) fadeTime = fadeMaxMs;
    }

    void tickStrobe(int passedMs) {
      strobePeriod -= passedMs; //always count down to next reset

      if(open) {
        onTime -= passedMs;
      } else { //closed, count down fade and off-tickers
        fadeTime -= passedMs; //wait have to keep orig val around for below tho. dupl...

        amount -= (eachFrameFade < amount? eachFrameFade: amount);
      }

      if(strobePeriod <= 0) { //reset for new iteration
        open = true;
        strobePeriod = 1000 / strobeHz;
        amount = 255;
      } else if(onTime <= 0) { // here should be option for curve out...
        open = false;
        resetOnTime();
        resetFadeTime();
      } else if(fadeTime <= 0) {
        amount = 0;
      }

    }

    void updateStrobe(uint8_t value) { //also report millis/micros since last frame so can move from guessing...
      if(!value) {
        open = true; // 0, clean up
        lastStrobe = 0;
        return;
      }
      int passedMs = targetTickMs; //test value

      if(value != lastStrobe) { // reset timer for new state
        initStrobeState(value);
      } else { // handle running strobe: decr/reset tickers, adjust shutter
        tickStrobe(passedMs);
      }
      lastStrobe = value;
    }
  };

  Shutter shutter; //might run multiple strips at some point I guess. Or split strip and run multiple shutters for effect fuckery...

  public:
  Functions(uint16_t targetFPS, Envelope pixelEnvelope, Envelope dimmerEnvelope):
    e(pixelEnvelope), dimmer(dimmerEnvelope) { }

  void update(uint8_t* functions) {
    shutter.updateStrobe(functions[chStrobe]);
    dimmer.update(functions[chDimmer], functions[chDimmerAttack], functions[chDimmerRelease]);
    e.set(functions[chAttack], functions[chRelease]);

    if(shutter.open) {
      outBrightness = dimmer.brightness;
    } else {
      uint8_t decreaseBy = 255 - shutter.amount;
      if(decreaseBy >= dimmer.brightness) outBrightness = 0;
      else outBrightness = dimmer.brightness - decreaseBy;
    }

  }

  // uint8_t rotateFwd, rotateBack;
  Dimmer dimmer;
  /* uint8_t outBrightness; */
  uint8_t outBrightness;
  Envelope e; //remember attack and dimattack are inverted, fix so anim works same way...
  // uint8_t* last_functions = &last_data[-1];  // XXX

}; //then just change all vals to use float internally, add setters for various input formats etc
// actually step towards modulators, change dimmer class to "value" so have envelopes for all
// prob good idea esp for rotate... also helps sorta auto-merge/blend eg incoming dimmer 255 + 0
// then (maybe) move on to actual plan of modulator bs...
