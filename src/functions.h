#pragma once

#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <optional>

#include <Arduino.h>

#include "log.h"
#include "envelope.h"
#include "buffer.h"
#include "renderstage.h"
#include "task.h"

#define MILLION 1000000
#define EPSILON 0.001f   // dont need crazy precision.. maybe revert to uint16 internally. at least for anything per-pixel.

namespace tol {

enum CH: uint8_t {
 chDimmer = 0, chStrobe, chHue, chAttack, chRelease, chBleed, chNoise,
 chRotateFwd, chRotateBack, chDimmerAttack, chDimmerRelease, chGain
};

bool fIsZero(float value);
bool fIsEqual(float lhs, float rhs);

// rework this as Runnable derived (derived). Need to incorporate Keyframe concept to that + generic update mechanism as well (tho better stick to show-where data is, go run...
class FunctionChannel { //rename, or group as, Function, for effects with multiple params?
  private:
    virtual float _apply(float value, float progress) { //force should be a persistent flag, not fn arg
      return value; // fallback if ch is neither applying nor affecting value, acting only as storage...  } //should be PixelBuffer as target
    }
    std::string _id;
  protected:
    RenderStage* rs;
    std::vector<Buffer*> targetBuffers; // need to be able to control multiple.
    float origin = 0.f, target = 0.f, current = 0.f;
    // bool running = false;
    /* std::tuple<float> range; */
    /* float min = 0.f, max = 1.f; // XXX scaling */
    std::optional<BlendEnvelope> e;
    // ADSREnvelope adsr; //dis the goal
  public:
    FunctionChannel(const std::string& id): _id(id) {}
    FunctionChannel(const std::string& id, std::optional<BlendEnvelope> e): _id(id), e(e) {}
    FunctionChannel(const std::string& id, float a, float r):
      FunctionChannel(id, BlendEnvelope(id)) { e->set(a, r); }
    virtual ~FunctionChannel() {}

    void apply(float progress) {
      float value = !e.has_value()? target: e->interpolate(origin, target, progress); //first generic envelope interpolation
      current = _apply(value, progress); // apply using that, possibly returning further changed value
      // running = fIsZero(current)? false: true;
      if(targetFn) targetFn(get()); // gotta run even if zero to get 0 well ya get...
    } // so now strobe is calling this every time even off. but not really a prob i guess.


    void update(float value) {
      origin = current; target = value;
    }
    void setTarget(RenderStage& targetRs) { rs = &targetRs; } //tho can manip anything taking a float (or properly wrapped), most common to just set a buffer?
    void setEnvelope(BlendEnvelope& newEnv) { e = newEnv; }
    void setEnvelope(float a, float r) { e->set(a, r); }

    virtual float get() { return current; }

    std::function<void(float)> targetFn; // can either call some fn each update, or just calc value and be polled
};



class Stage {
  enum: uint8_t { MIN = 0, MAX = 1 };
  std::string id;
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
  Stage(const std::string& id, float origin, float destination, float minTime = 0, float maxTime = 0, float fractionOfTotal = 0):
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
    { Stage("open", 1.0f, 1.0f, MILLION/200, MILLION/15, 7.0f),
      Stage("fading", 1.0f, 0.0f, MILLION/200, MILLION/7, 5.0f),
      Stage("closed", 0.0f, 0.0f),
    };

  float hzMin, hzMax;
  enum StrobeStage: uint8_t { OPEN = 0, FADING, CLOSED, TOTAL, PRE };
  uint32_t timeInStage[5] = {0};
  int32_t timeRemaining = 0;
  uint8_t activeStage = PRE;
  float fraction[2] = {1/8.5f, 1/6.0f}; //OPEN, FADING
  uint32_t maxTime[2] = {(MILLION/20), (MILLION/8)}; //OPEN, FADING
  uint32_t minTime[2] = {(MILLION/200), (MILLION/100)}; //OPEN, FADING

  uint32_t lastTick = 0;
  float variance = 1.0f; // use for more organic type thing...
  float minResult = 0.0f;
  float maxResult = 1.0f; //want positive modulation in some cases.
  float result = 1.0f;

  void start(float value);
  void initStage(uint32_t debt = 0);
  void tick(uint32_t passedTime);

  float _apply(float value, float progress) override; //this was why not being called... turned into different function without Strip arg
  public:
  Strober(const std::string& id = "Strobe", float hzMin = 1.0f, float hzMax = 12.5f):
   FunctionChannel(id), hzMin(hzMin), hzMax(hzMax) {}

  float get() override { return result; }
};

class Dimmer: public FunctionChannel { // XXX point dimmer to dest shutter. boom.
  public:
  Dimmer(BlendEnvelope e): FunctionChannel("Dimmer", e) {}
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
/*     PassThrough(const std::string& id, std::function<void(T)> targetFn): */
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


class Functions: public RenderStage,
                 public PureEvtTask,
                 public Sub<PatchControls> { // receives control values and runs on-chip effect functions like dimmer, strobe, blend and rotation
  private:

  public:
  RenderStage& target;
  std::map<CH, FunctionChannel*> chan; //or just vector lol std::vector<FunctionChannel*> chan;

  Functions(const std::string& id, RenderStage& target, uint8_t channelCount = 12):
    RenderStage(id + " functions", 1, channelCount),
    PureEvtTask((id + " functions updater").c_str(), 2),
    Sub<PatchControls>(this),
    target(target), numChannels(channelCount),
    chOverride("override", 1, channelCount),
    val("val", 1, channelCount), blendOverride("blendOverride", 1, 12) {
      
    lg.dbg("Create Functions, target Stage: " + target.id());
     
    chOverride.fill(-1);
    blendOverride.fill(0.5f);

    chan[chDimmer] = new Dimmer(BlendEnvelope("dimmerEnvelope", 1.2f, 1.1f));
    // using namespace std::placeholders;
    // chan[chDimmer]->targetFn = std::bind(&RenderStage::setGain, target, _1);
    chan[chDimmer]->targetFn = [this](float result) {
      this->target.setGain(result);
    }; // an exercise in pointlessness: the std::bind equivalent suddenly simply stopped working.
 
    // is it this done in ctor and passed target overrides member target?
    // but being an opaque ref to a still existing asshole shouldn't that not matter?
    // or at least error? :S
    chan[chDimmer]->update(1.0); // set to full so works if not actively set

    chan[chStrobe] = new Strober("Shutter", 0.5f, 10.0f);
    chan[chStrobe]->targetFn = [this](float result) { /* // or auto& buf = b; then capture buf */
      if(result < 2.0f) { // btw strobe should obviously overdrive (very first frames then quickly ramp down to regular...), will clip to 1 later
        // and gain > 1.0 further down should eventually whiten and shit right, cool
        this->target.setGain(result * this->target.getGain()); // NOTE assumes chDimmer runs prior and resets gain each tick
      }};

    chan[chNoise] = new Noise();
    chan[chBleed] = new Bleed();
    chan[chRotateFwd] = new Rotate(true);
    chan[chRotateBack] = new Rotate(false);
    chan[chHue] = new RotateHue();
    for(auto c: chan) c.second->setTarget(target); //lock in strip s as our target...

    /* e.set(0.5f, 0.5f); //default not 0 so is nice for local sources if no input */
  }
  
  ~Functions() {} //remember for when got mult

  void apply(float progress) { //so this gets renamed run? but usually those are without args.
    for(auto c: chan)
      c.second->apply(progress);
  }

// void Renderer::updateControls(const Buffer& controlData, bool blend) {
//     if(blend) controls->avg(controlData, false);
//     else      controls->set(controlData);
//     f->update(controls->get());
// }
// // doing some extra stuff ya ^
  void update(const Buffer& controls) { //rename update. or just set? and shouldnt need args - we'd already know where to fetch new data.
    buffer().setCopy(controls);
    for(uint8_t i=0; i < numChannels; i++) { // prep for when moving entirely onto float
      // *buffer().get(i) = *chOverride.get(i) < 0?
      //   *controls.get(i):
      //   ((uint8_t)*chOverride.get(i) * *blendOverride.get(i) + // Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
      //    *controls.get(i) * (1.0f - *blendOverride.get(i))); //but also allow (with prio/mode flag) override etc. and adjust behavior, why have anything at all hardcoded tbh
      *val.get(i) = std::clamp(*controls.get(i) / 255.0f, 0.0f, 1.0f);
      
    }

    e.set(*val.get(chAttack), *val.get(chRelease)); // high val can btw only slow things,
    // but low (=quick/high) should actually hype progress, at extreme completely cancel frame blending... or overshoot diff? haha
    chan[chDimmer]->setEnvelope(*val.get(chDimmerAttack), *val.get(chDimmerRelease)); //point straight instead let them update it

    for(auto c: chan)
      c.second->update(*val.get(c.first));
  }

  void onEvent(const PatchControls& controls) {
    update(controls.buffer);
  }

  uint8_t numChannels;
  BufferI chOverride;
  BufferF val, blendOverride;
  BlendEnvelope e = BlendEnvelope("pixelEnvelope", 1.2f, 1.15); //remember attack and dimattack are inverted, fix so anim works same way...
};

}
