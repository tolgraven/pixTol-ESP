#pragma once

// TODO
// decouple from Strip. Outputter could work, but best just Buffer...
#include <algorithm>
#include <map>
#include <vector>
#include <lib8tion.h>  // fastled math and shit
#include <Arduino.h>
#include "log.h"
#include "envelope.h"
#include "buffer.h"

#define MILLION 1000000
#define EPSILON 0.0001   // dont need crazy precision.. maybe revert to uint16 internally. at least for anything per-pixel.

enum CH: uint8_t {
 chDimmer = 0, chStrobe, chHue, chAttack, chRelease, chBleed, chNoise,
 chRotateFwd, chRotateBack, chDimmerAttack, chDimmerRelease, chGain
};

bool fIsZero(float value);
bool fIsEqual(float lhs, float rhs);

class FunctionChannel { //rename, or group as, Function, for effects with multiple params?
  private:
    virtual float _apply(float value, float progress) { //force should be a persistent flag, not fn arg
      return value; // fallback if ch is neither applying nor affecting value, acting only as storage...  } //should be PixelBuffer as target
    }
    String _id;
  protected:
    Buffer* b = nullptr;
    float origin = 0, target = 0, current = 0;
    /* std::tuple<float> range; */
    /* float min = 0, max = 1; // XXX scaling */
    BlendEnvelope* e = nullptr;
    // ADSREnvelope adsr; //dis the goal
  public:
    FunctionChannel(const String& id): _id(id) {}
    FunctionChannel(const String& id, BlendEnvelope* e): _id(id), e(e) {}
    FunctionChannel(const String& id, float a, float r):
      FunctionChannel(id, new BlendEnvelope(id)) { e->set(a, r); }
    virtual ~FunctionChannel() { if(e) delete e; }
    //prob have dedicated "end (curr run) and clean up" fn?

    void apply(float progress) {
      float value = !e? target: e->interpolate(origin, target, progress); //first generic envelope interpolation
      /* float value = target; */
      current = _apply(value, progress); // apply using that, possibly returning further changed value
      /* if(targetFn && !fIsZero(current)) targetFn(get()); //wait, would make dimmer0 never apply. ugh. not necessarily current(input), but result */
      if(targetFn) targetFn(get()); //not necessarily current(input), but result
      /* if(targetFn) targetFn(1.0); //not necessarily current(input), but result */
    } // so now strobe is calling this every time even off. but not really a prob i guess.


    void update(float value) {
      origin = current; target = value;
    }
    void setTarget(Buffer* targetBuffer) { b = targetBuffer; } //tho can manip anything taking a float (or properly wrapped), most common to just set a buffer?
    void setEnvelope(BlendEnvelope* newEnv) { if(e) delete e; e = newEnv; }
    void setEnvelope(float a, float r) { e->set(a, r); }

    virtual float get() { return current; }

    std::function<void(float)> targetFn; // can either call some fn each update, or just calc value and be polled
};



class Stage {
  enum: uint8_t { MIN = 0, MAX = 1 };
  String id;
  float origin, destination; // eg Open->closed, closed->open, but could do more better funner. min/max and some proper oscillators would make more sense tho lol - eg reuse that system here.
  uint32_t minTime, maxTime;
  /* std::tuple<float> resultRange; */
  /* std::tuple<uint32_t> timeRange; */
  float fractionOfTotal; // seems maybe more reasonable control from outside. but store here?
  uint32_t timeActive;
  // also need a priority thing in case req dont add up
  /* Easer* yada; */
  std::function<float(uint32_t)> fn = [this](uint32_t timePassed) {
    if(origin == destination) { //XXX need float comp
      return origin;
    /* } else if(easer) { */
    /*   return easer->get(timePassed / timeActive); */
    } else {
      return origin + (timePassed / timeActive) * (destination - origin);
    }
  }; //well most wont have one so cant have default but obvs default is straight interpolation from origin to destination

  public:
  Stage(const String& id, float origin, float destination, float minTime = 0, float maxTime = 0, float fractionOfTotal = 0):
   id(id), origin(origin), destination(destination), minTime(minTime), maxTime(maxTime), fractionOfTotal(fractionOfTotal) {
   }

  void init(uint32_t requestedTime) {
    if(fractionOfTotal) requestedTime /= fractionOfTotal;
    if(minTime) requestedTime = std::max(requestedTime, minTime);
    if(maxTime) requestedTime = std::min(requestedTime, maxTime);
    timeActive = requestedTime;
  }
  void tick(uint32_t totalTimePassed) { // total, for this stage, so dont have to keep internal count?
    fn(totalTimePassed);
  }
};

 // unit was crashing (and booting real quick, no homie..) causing slow opposite strobing, short time off then fade back on. looked p cool, fix!
class Strober: public FunctionChannel { //maybe more appropriate further general and with multiple main inputs (value = toprange, otherval = timing...)
  private:
  std::vector<Stage> stages =
    { Stage("open", 1.0, 1.0, MILLION/200, MILLION/15, 7.0),
      Stage("fading", 1.0, 0.0, MILLION/200, MILLION/7, 5.0),
      Stage("closed", 0.0, 0.0),
    };

  float hzMin, hzMax;
  enum StrobeStage: uint8_t { OPEN = 0, FADING, CLOSED, TOTAL, PRE };
  uint32_t timeInStage[5] = {0};
  int32_t timeRemaining = 0;
  uint8_t activeStage = PRE;
  float fraction[2] = {1/8.5, 1/6.0}; //OPEN, FADING
  uint32_t maxTime[2] = {(MILLION/20), (MILLION/8)}; //OPEN, FADING
  uint32_t minTime[2] = {(MILLION/200), (MILLION/100)}; //OPEN, FADING

  uint32_t lastTick = 0;
  float variance = 1.0; // use for more organic type thing...
  float minResult = 0.0;
  float maxResult = 1.0; //want positive modulation in some cases.
  float result = 1.0;

  void start(float value);
  void initStage(uint32_t debt = 0);
  void tick(uint32_t passedTime);

  float _apply(float value, float progress) override; //this was why not being called... turned into different function without Strip arg
  public:
  Strober(const String& id = "Strobe", float hzMin = 1.0, float hzMax = 12.5):
   FunctionChannel(id), hzMin(hzMin), hzMax(hzMax) {}

  float get() override { return result; }
};

class Dimmer: public FunctionChannel { // XXX point dimmer to dest shutter. boom.
  public:
  Dimmer(BlendEnvelope* e): FunctionChannel("Dimmer", e) {}
  Dimmer(float a, float r): FunctionChannel("Dimmer", a, r) {}
};

/* class Shutter: public FunctionChannel { */
class Shutter {
  public:
   Shutter() {}
  // dimmer + strobe...since former gotta scale by latter
  // could be a dummy ch until impl multiple inputs?
  Dimmer* dimmer;
  Strober* strobe;
};

/* template<typename T> */
/* class PassThrough: public FunctionChannel { */
/*     std::function<void(T)> targetFn; */
/*     float _apply(float value, float progress) { */
/*       // use whatever envelope etc... */
/*       targetFn(value); */
/*     } */
/*   public: */
/*     PassThrough(const String& id, std::function<void(T)> targetFn): */
/*      FunctionChannel(id), targetFn(targetFn) {} */
/* }; */

// tbh this one that just takes a nr and calls some fn doesnt need to be its own class lol
class Rotate: public FunctionChannel {
 // AHEAD:
 // move to rotating freestanding buffer directly, once implemented
 // then make fancy version of buffer rotation with alpha control for both pre/post
 // then higher layer thing where you start a rotation animation
 // and fade those values between 1.0/0.0 0.0/1.0, for one rotation step
 // then can have sorta anti-aliased without actual
 // (tho need proper as well)
    bool forward;
    float _apply(float value, float progress);
  public:
    Rotate(bool forward): FunctionChannel("Rotate Strip"), forward(forward) {}
};

class Bleed: public FunctionChannel {
  enum LOCATION: uint8_t { BEHIND = 0, AHEAD, THIS, BLEND };
  float _apply(float value, float progress);
  public:
  Bleed(): FunctionChannel("Bleed") {}
};

class RotateHue: public FunctionChannel {
    float _apply(float value, float progress);
  public:
    RotateHue(): FunctionChannel("Rotate Hue") {}
};

class Noise: public FunctionChannel { // ugh
  float _apply(float value, float progress);
  public:
    int8_t subPixelNoise[125][4] = {{0}}; // cant do it like this anyways. put noise on hold yo...
    int8_t maxNoise = 64; //baseline
    Noise(): FunctionChannel("Noise") {}
};

class Fill: public FunctionChannel { // good test case for one w multiple inputs? else need one per subcolor
  float _apply(float value, float progress);
  public:
    /* HslColor hsl; */
    Fill(): FunctionChannel("Fill") {}
};


class Functions { // receives control values and runs on-chip effect functions like dimmer, strobe, blend and rotation
  private:

  public:
  Buffer* b = nullptr;
  std::map<CH, FunctionChannel*> chan; //or just vector lol std::vector<FunctionChannel*> chan;

  Functions(Buffer& targetBuffer):
    b(&targetBuffer) {
    lg.dbg("Create Functions, targetBuffer: " + targetBuffer.id());
     
    for(uint8_t i=0; i < numChannels; i++) {
      chOverride[i] = -1; //dont seem to properly auto init from {-1} otherwise hmm
      blendOverride[i] = 0.5;
    }

    using namespace std::placeholders;
    lg.f("Functions", Log::DEBUG, "Buffer: %p, ptr: %p\n", b, b->get());
    chan[chDimmer] = new Dimmer(new BlendEnvelope("dimmerEnvelope", 1.2, 1.1));
    /* chan[chDimmer] = new Dimmer(new BlendEnvelope("dimmerEnvelope", 1.0, 1.0)); */
    chan[chDimmer]->targetFn = std::bind(&Buffer::setGain, b, _1);

    chan[chStrobe] = new Strober("Shutter", 0.5, 10.0);
    /* // or auto& buf = b; then capture buf */
    chan[chStrobe]->targetFn = [this](float value) { //somehow not doing the trick anymore. like captured scope no longer valid...
      if(value < 1.0 && this->b) {
        /* lg.dbg(String("Val, gain:") + value + " " + this->b->getGain()); */
        this->b->setGain(value * this->b->getGain());
      }
    }; //works, but then becomes dependent on dimmer being executed first, etc... figure out something clean.
    // i guess yeah "virtual" channel shutter is pointed to by dimmer + strobe,

    /* chan[chNoise] = new Noise(); */
    /* chan[chBleed] = new Bleed(); */
    /* chan[chRotateFwd] = new Rotate(true); */
    /* chan[chRotateBack] = new Rotate(false); */
    /* chan[chHue] = new RotateHue(); */
    for(auto c: chan) c.second->setTarget(b); //lock in strip s as our target...

    /* e.set(0.5, 0.5); //default not 0 so is nice for local sources if no input */
  }
  ~Functions() {} //remember for when got mult

  void apply(float progress) { //so this gets renamed run? but usually those are without args.
    for(auto c: chan)
      c.second->apply(progress);
  }

  void update(uint8_t* fun) { //rename update. or just set? and shouldnt need args - we'd already know where to fetch new data.
    for(uint8_t i=0; i < numChannels; i++) { // prep for when moving entirely onto float
      ch[i] = chOverride[i] < 0?
              fun[i]:
      ((uint8_t)chOverride[i] * blendOverride[i] + // Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
                ch[i] * (1.0f - blendOverride[i])); //but also allow (with prio/mode flag) override etc. and adjust behavior, why have anything at all hardcoded tbh
      val[i] = ch[i] / 255.0f;
    }

    e.set(val[chAttack], val[chRelease]); // high val can btw only slow things,
    // but low (=quick/high) should actually hype progress, at extreme completely cancel frame blending... or overshoot diff? haha
    chan[chDimmer]->setEnvelope(val[chDimmerAttack], val[chDimmerRelease]); //point straight instead let them update it

    for(auto c: chan)
      c.second->update(val[c.first]);
  }

  static const uint8_t numChannels = 12;
  uint8_t ch[numChannels] = {0};
  int chOverride[numChannels] = {-1};
  float val[numChannels] = {0}, blendOverride[numChannels] = {0.5};
  BlendEnvelope e = BlendEnvelope("pixelEnvelope", 1.6, 1.4); //remember attack and dimattack are inverted, fix so anim works same way...
};
