#pragma once

#include <Arduino.h>
#include <LoggerNode.h>
#include "config.h"
#include "io/strip.h"
#include "envelope.h"
#include "util.h"

enum CH: uint8_t {
 chDimmer = 0, chStrobe, chHue, chAttack, chRelease, chBleed, chNoise,
 chRotateFwd, chRotateBack, chDimmerAttack, chDimmerRelease, chGain
};

class FunctionChannel { //rename, or group as, Function, for effects with multiple params?
  private:
    virtual float _apply(float value, float progress, Strip& s, bool force = false) {
      return value; // fallback if ch is neither applying nor affecting value, acting only as storage...  } //should be PixelBuffer as target
    }
  public:
    FunctionChannel(const String& id, BlendEnvelope e = BlendEnvelope()):
      _id(id), e(e) {}
    FunctionChannel(const String& id, float a, float r):
      FunctionChannel(id, BlendEnvelope(_id)) { e.set(a, r); }

    void apply(float value, float progress, Strip& s) { //Buffer& b...
     // if(!b && !target) return;
     // if(!value) return; //wouldnt this result in never un-activating? also'd cut smooth off's
     // if(!b) checkforinternal
      target = e.interpolate(current, value, progress); //first generic envelope interpolation
      // ^ does most things actually want for this tho? gen wanna respond asap to cmds like.
      // esp if first ch interpolates, then effect of ch interpolated as well, slowish?
      current = _apply(target, progress, s); // apply using that, possibly returning further changed value
    }
    void apply(uint8_t value, float progress, Strip& s) {
      apply((float)value/255, progress, s);
    }
    void force(float value, float progress, Strip& s) { // how do this without fucking up time in _apply?
      if(_id == "Strobe" && value > 0)
        LN.logf(__func__, LoggerNode::DEBUG, "val %f", value);
      target = value;
      current = _apply(target, progress, s, true);
    }
    // FunctionChannel& operator+(float diff) {} //nahmean

    void setEnvelope(BlendEnvelope newEnv) { e = newEnv; }
    void setEnvelope(float a, float r) { e.set(a, r); }

    float get() { return current; }
    uint8_t getByte() { return (uint8_t)(current*255); }

    String _id;
    // Buffer* target; std::vector<Buffer*> targets;
    float target = 0, current = 0;
    BlendEnvelope e = BlendEnvelope(); //or pointer? so some can share, skip or w/e
    // ADSREnvelope adsr; //dis the goal
};

class Dimmer: public FunctionChannel {
  public:
  Dimmer(BlendEnvelope e): FunctionChannel("Dimmer", e) {}
  Dimmer(float a, float r): FunctionChannel("Dimmer", a, r) {}
};

class Shutter: public FunctionChannel { //this would later contain FunctionChannels Strobe & Dimmer, I guess...
  private:
  float hzMin, hzMax, hz;
  enum StrobeStage { OPEN = 0, FADING, CLOSED, TOTAL, PRE };
  int ms[5] = {0};
  uint8_t activeStage;
  float fraction[2] = {3.5, 7.0}; //OPEN, FADING
  int maxMs[2] = {120, 60};

  uint32_t keyFrameInterval;
  float lastProgress = 0;
  uint8_t eachFrameFade = 255 / 50; // for FADING, should be ms-based instead...

  // float _apply(float value, float progress, Strip& s, bool force = false); //this was why not being called... turned into different function without Strip arg
  void start(float value);
  void initStage(int debt);
  void tickStrobe(int passedMs);

  public:
  Shutter(uint16_t keyFrameInterval = MILLION/40, float hzMin = 1.0, float hzMax = 12.5):
   FunctionChannel("Strobe"), hzMin(hzMin), hzMax(hzMax),
   keyFrameInterval(keyFrameInterval), open(true), amountOpen(255) {};

  bool open; // could also have shutter as float not bool and fade using it. 0 closed, 1 open, between in transition
  int amountOpen;
};


class RotateStrip: public FunctionChannel {
 // AHEAD:
 // move to rotating freestanding buffer directly, once implemented
 // then make fancy version of buffer rotation with alpha control for both pre/post
 // then higher layer thing where you start a rotation animation
 // and fade those values between 1.0/0.0 0.0/1.0, for one rotation step
 // then can have sorta anti-aliased without actual
 // (tho need proper as well)
    bool forward;
    // float _apply(float value, float progress, Strip& s, bool force = false);
  public:
    RotateStrip(bool forward): FunctionChannel("Rotate Strip"), forward(forward) {}
};

class Bleed: public FunctionChannel {
  enum { BEHIND = 0, AHEAD, THIS, BLEND };
  // float _apply(float value, float progress, Strip& s, bool force = false);
  public:
  Bleed(): FunctionChannel("Bleed") {}
};

class HueRotate: public FunctionChannel {
    // float _apply(float value, Strip& s) {
  public:
    HueRotate(): FunctionChannel("Rotate Hue") {}
};

class Noise: public FunctionChannel { // ugh
  // float _apply(float value, float progress, Strip& s, bool force = false);
  public:
    int8_t subPixelNoise[125][4] = {{0}}; // cant do it like this anyways. put noise on hold yo...
    int8_t maxNoise = 64; //baseline
    Noise(): FunctionChannel("Noise") {}
};


class Functions { // receives control values and runs on-chip effect functions like dimmer, strobe, blend and rotation
  private:
  uint32_t keyFrameInterval;
  Shutter shutter; //might run multiple strips at some point I guess. Or split strip and run multiple shutters for effect fuckery...
  Bleed b; HueRotate h; RotateStrip rotFwd, rotBack;

  Strip* s = nullptr; //temp, for now Buffer buffer; //target for functions

  public:
  Functions(uint16_t keyFrameInterval, BlendEnvelope pixelEnvelope, BlendEnvelope dimmerEnvelope, Strip* s): //for now
    keyFrameInterval(keyFrameInterval), shutter(keyFrameInterval, 1.0, 10.0),
    rotFwd(true), rotBack(false), s(s), dimmer(dimmerEnvelope), e(pixelEnvelope) {

    for(uint8_t i=0; i < numChannels; i++) {
        chOverride[i] = -1; //dont seem to properly auto init from {-1} otherwise hmm
        blendOverride[i] = 0.5;
    }
  }

  void update(float progress, uint8_t* fun = nullptr); //void update(uint32_t elapsed, uint8_t* fun = nullptr) {

  Dimmer dimmer; uint8_t outBrightness;
  BlendEnvelope e; //remember attack and dimattack are inverted, fix so anim works same way...
  static const uint8_t numChannels = 12; //static for now
  uint8_t ch[numChannels] = {0}; int chOverride[numChannels] = {-1};
  float val[numChannels] = {0}, blendOverride[numChannels] = {0.5};
};
