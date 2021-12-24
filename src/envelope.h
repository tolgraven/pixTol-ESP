#pragma once

#include <Arduino.h>
#include "log.h"
// #include <NeoPixelAnimator.h>

// actual elope generator for impulse response
class ADSREnvelope {
	enum { ATTACK, DECAY, SUSTAIN, RELEASE, IDLE };
};


class EnvelopeStage {
 EnvelopeStage(const std::string& id, uint32_t maxDuration, float fraction):
  _id(id), maxDuration(maxDuration), fraction(fraction) {}
 std::string _id;
 uint32_t duration, maxDuration;
 float start, end, current; //starting, ending, and current value
 float fraction = -1; //fraction of total animation this stage should run for, 0 = container decides
}; // contained in eg Shutter.
   // OR in a full class and Shutter inherits both Animation and FunctionChannel?

class Envelope { // base class for envelopes and "envelopes" like blendfactors.
    std::string _id;
    bool straight; // no invert = high-is-low
    std::vector<EnvelopeStage*> stage;
    virtual Envelope& setLevel(const EnvelopeStage& stage, float level);
    virtual Envelope& setTime(const EnvelopeStage& stage, float time); //ms? s?
};


enum EnvStage: uint8_t { AT = 0, RE }; //should be dynamic and that...
// template<typename T>
class BlendEnvelope { // simple "envelope" to control how much an incoming value should affect the current
  private:
    std::string _id;
    bool straight = false; // default is actually inverted (since used for progress), so straight is unmodified high-attack high-number...
    float divisor[RE+1] = {1.0}; // float attackDivisor = 1.0, releaseDivisor = 1.0; //XXX fix for float
    float _baseline = 1.0; //0.5 // default no effect. the value at 0 attack/release. Later move away from envelope to renderer?
    // float lastProgress = 0; // will need 16bit to work tho
    float value[RE+1] = {0.0}; // float attack = 0.0, release = 0.0; // now as fraction. later fix so can take ms, seconds or whatever
    // float alpha = 1.0; // to scale total effect at once?

    void adjustForDirection() {
      if(!straight)
        for(uint8_t es = AT; es <= RE; es++)
          value[es] = constrain(1.0 - value[es], 0.0, 1.0);
    }
  public:
    BlendEnvelope(const std::string& id = "BlendEnvelope", float atDivisor = 1.0, float reDivisor = 1.0,
        bool straight = false, float baseline = 1.0):
      _id(id), straight(straight), _baseline(baseline) {
        setDivisor(AT, atDivisor);
        setDivisor(RE, reDivisor);
        adjustForDirection();
    }

    void setDivisor(EnvStage es, float value) {
        divisor[es] = max(value, (float)1.0);
    }
    void set(float at = 0.0, float re = 0.0) {
      value[AT] = at; value[RE] = re;
      for(uint8_t es = AT; es <= RE; es++)
        // value[es] = _baseline - _baseline * (value[es] / divisor[es]); // dont ever quite arrive like this, two runs at max = 75% not 100%...
        value[es] = value[es] / divisor[es]; // dont ever quite arrive like this, two runs at max = 75% not 100%...
      adjustForDirection(); //XXX XXX figure out at later point why the fuuuuck values are coming in already inverted??? bizarre. oh well...
    }
    float A(float progress) const { return get(AT, progress); } //lol i was dividing...
    float R(float progress) const { return get(RE, progress); }
    float get(EnvStage es, float progress) const {
      return value[es] * progress; //wouldnt work for straight tho?
    }

    float interpolate(float origin, float target, float progress, float min = 0, float max = 1) const {
      if(origin == target) return origin;
      float scaler = target > origin? A(progress): R(progress);
      float result = origin + ((target - origin) * scaler);
      return constrain(result, min, max);
    }
    // this should be templatized obvs? uint8_t yo
    // float interpolate(uint32_t micros, float current, float target, float min = 0, float max = 1) {
    //   // switch(currentStage) // for impulse one... something like mozzi
    //   if(current == target) return current;
    //   bool higher = target > current;
    //   float progress = micros / keyFrameInterval; // so, can overshoot... but again that best done by calc delta and just applying? depends on buffer size
    //
    //   float result = current + (target - current) * (higher? A(): R());
    //   return result > max? max: result < min? min: result;
    // }
};


// AnimEaseFunction moveEase =
//      NeoEase::Linear;
//      NeoEase::QuadraticInOut;
//      NeoEase::CubicInOut;
// NeoEase::
     // NeoEase::QuinticInOut;
//      NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;
//some easing methods to use?
// find elsewhere:
// inElastic outElastic inOutElastic
// inBack outBack inOutBack
// inBounce outBounce inOutBounce

// struct Easer {
//   enum Direction { IN = 0, OUT, INOUT };
//   // enum Type { LINEAR = 0, QUAD, CUBIC, QUART, QUINT, SINE, EXP, CIRC };
//   enum Type { POW, SINE, EXP, CIRC };
//   enum pow_t: uint8_t { LINEAR = 1, QUAD = 2, CUBIC = 3, QUART = 4, QUINT = 5 };
//   Direction direction = IN;
//   Type type = QUAD;
//   pow_t raised = LINEAR;
//   // std::function func =
//   // std::pair<std::function*, std::function*> funcs;
//   std::function<float(float)> funcs[3] = {nullptr};
//   // float Ease(float t, float b, float c, float d) { t curr time, b start val, c change in val, d duration
//   const Easer& setType(Type type, pow_t raisedTo = LINEAR) {
//     // funcs = type==QUAD? {[](float p) {return p * p};
//     // funcs = type==QUAD? {[](float p) {return pow(p, 2)};
//     //                      [](float p) {return -p * (p - 2.0f)} }:
//     //         type==CUBIC? {[](float p) {}
//     //                       [](float p) {}}
//     // funcs[IN] = [](float p) {return pow(p, raisedTo); }
//     using namespace std::placeholders;
//     funcs[IN] = std::bind(pow, _1, (float)raisedTo); //then just call yeah?
//     // funcs[OUT] =std::bind(pow, _1, (float)raisedTo); //then just call yeah?
//     // funcs[OUT] = [](float p) -> float { p -= 1;  } //fix
//
//     funcs[INOUT] = [funcs](float p) -> float {return (p < 0.5? funcs[IN](p): funcs[OUT](p))};
//   }
//   float Ease(float p) {
//     // switch(type) {
//     //   case LINEAR:
//     //     return p; break;
//     //   case QUAD:
//     //     inFunc  = [](float p) {return p * p};
//     //     outFunc = [](float p) {return -p * (p - 2.0f)};
//     //     break;
//     // }
//     // func = direction == IN?     inFunc:
//     //        direction == OUT?    outFunc:
//     //        direction == INOUT?  p < 0.5? inFunc: outFunc;
//   }
// };


// float Linear(float p) { return p; }
//
// float QuadraticIn(float p) { return p * p; }
// float QuadraticOut(float p) { return (-p * (p - 2.0f)); }
// float QuadraticInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (0.5f * p * p);
//         } else { p -= 1.0f; return (-0.5f * (p * (p - 2.0f) - 1.0f)); } }
//
// float CubicIn(float p) { return (p * p * p); }
// float CubicOut(float p) { p -= 1.0f; return (p * p * p + 1); }
// float CubicInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (0.5f * p * p * p);
//         } else { p -= 2.0f; return (0.5f * (p * p * p + 2.0f)); } }
//
// float QuarticIn(float p) { return (p * p * p * p); }
// float QuarticOut(float p) { p -= 1.0f; return -(p * p * p * p - 1); }
// float QuarticInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (0.5f * p * p * p * p);
//         } else { p -= 2.0f; return (-0.5f * (p * p * p * p - 2.0f)); } }
//
// float QuinticIn(float p) { return (p * p * p * p * p); }
// float QuinticOut(float p) { p -= 1.0f; return (p * p * p * p * p + 1.0f); }
// float QuinticInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (0.5f * p * p * p * p * p);
//         } else { p -= 2.0f; return (0.5f * (p * p * p * p * p + 2.0f)); } }
//
// float SinusoidalIn(float p) { return (-cos(p * HALF_PI) + 1.0f); }
// float SinusoidalOut(float p) { return (sin(p * HALF_PI)); }
// float SinusoidalInOut(float p) { return -0.5 * (cos(PI * p) - 1.0f); }
//
// float ExponentialIn(float p) { return (pow(2, 10.0f * (p - 1.0f))); }
// float ExponentialOut(float p) { return (-pow(2, -10.0f * p) + 1.0f); }
// float ExponentialInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (0.5f * pow(2, 10.0f * (p - 1.0f)));
//         } else { p -= 1.0f; return (0.5f * (-pow(2, -10.0f * p) + 2.0f)); } }
//
// float CircularIn(float p) {
//         if (p == 1.0f) { return 1.0f;
//         } else { return (-(sqrt(1.0f - p * p) - 1.0f)); } }
// float CircularOut(float p) { p -= 1.0f; return (sqrt(1.0f - p * p)); }
//
// float CircularInOut(float p) {
//         p *= 2.0f;
//         if (p < 1.0f) { return (-0.5f * (sqrt(1.0f - p * p) - 1)); }
//         else { p -= 2.0f; return (0.5f * (sqrt(1.0f - p * p) + 1.0f)); } }
//
// float Gamma(float p) { return pow(p, 1.0f / 0.45f); }
//



// float linear(t, b, c, d) { return c*t/d + b; } // simple linear tweening - no easing, no acceleration
//
// float easeInQuad(t, b, c, d) { // quadratic easing in - accelerating from zero velocity
// 	t /= d;
// 	return c*t*t + b;
// }
// float easeOutQuad(t, b, c, d) { // quadratic easing out - decelerating to zero velocity
// 	t /= d;
// 	return -c * t*(t-2) + b;
// }
// float easeInOutQuad(t, b, c, d) { // quadratic easing in/out - acceleration until halfway, then deceleration
// 	t /= d/2;
// 	if (t < 1) return c/2*t*t + b;
// 	t--;
// 	return -c/2 * (t*(t-2) - 1) + b;
// }
//
// float easeInCubic(t, b, c, d) { // cubic easing in - accelerating from zero velocity
// 	t /= d;
// 	return c*t*t*t + b;
// }
// float easeOutCubic(t, b, c, d) { // cubic easing out - decelerating to zero velocity
// 	t /= d;
// 	t--;
// 	return c*(t*t*t + 1) + b;
// }
// float easeInOutCubic(t, b, c, d) { // cubic easing in/out - acceleration until halfway, then deceleration
// 	t /= d/2;
// 	if (t < 1) return c/2*t*t*t + b;
// 	t -= 2;
// 	return c/2*(t*t*t + 2) + b;
// }
//
// float easeInQuart(t, b, c, d) { // quartic easing in - accelerating from zero velocity
// 	t /= d;
// 	return c*t*t*t*t + b;
// }
// float easeOutQuart(t, b, c, d) { // quartic easing out - decelerating to zero velocity
// 	t /= d;
// 	t--;
// 	return -c * (t*t*t*t - 1) + b;
// }
// float easeInOutQuart(t, b, c, d) { // quartic easing in/out - acceleration until halfway, then deceleration
// 	t /= d/2;
// 	if (t < 1) return c/2*t*t*t*t + b;
// 	t -= 2;
// 	return -c/2 * (t*t*t*t - 2) + b;
// }
//
// float easeInQuint(t, b, c, d) { // quintic easing in - accelerating from zero velocity
// 	t /= d;
// 	return c*t*t*t*t*t + b;
// }
// float easeOutQuint(t, b, c, d) { // quintic easing out - decelerating to zero velocity
// 	t /= d;
// 	t--;
// 	return c*(t*t*t*t*t + 1) + b;
// }
// float easeInOutQuint(t, b, c, d) { // quintic easing in/out - acceleration until halfway, then deceleration
// 	t /= d/2;
// 	if (t < 1) return c/2*t*t*t*t*t + b;
// 	t -= 2;
// 	return c/2*(t*t*t*t*t + 2) + b;
// }
//
// float easeInSine(t, b, c, d) { // sinusoidal easing in - accelerating from zero velocity
// 	return -c * float cos(t/d * (float PI/2)) + c + b;
// }
// float easeOutSine(t, b, c, d) { // sinusoidal easing out - decelerating to zero velocity
// 	return c * float sin(t/d * (float PI/2)) + b;
// }
// float easeInOutSine(t, b, c, d) { // sinusoidal easing in/out - accelerating until halfway, then decelerating
// 	return -c/2 * (float cos(float PI*t/d) - 1) + b;
// }
//
// float easeInExpo(t, b, c, d) { // exponential easing in - accelerating from zero velocity
//   return c * float pow( 2, 10 * (t/d - 1) ) + b;
// }
// float easeOutExpo(t, b, c, d) { // exponential easing out - decelerating to zero velocity
// 	return c * ( -float pow( 2, -10 * t/d ) + 1 ) + b;
// }
// float easeInOutExpo(t, b, c, d) { // exponential easing in/out - accelerating until halfway, then decelerating
// 	t /= d/2;
// 	if (t < 1) return c/2 * float pow( 2, 10 * (t - 1) ) + b;
// 	t--;
// 	return c/2 * ( -float pow( 2, -10 * t) + 2 ) + b;
// }
//
// float easeInCirc(t, b, c, d) { // circular easing in - accelerating from zero velocity
//   t /= d;
// 	return -c * (float sqrt(1 - t*t) - 1) + b;
// }
// float easeOutCirc(t, b, c, d) { // circular easing out - decelerating to zero velocity
// 	t /= d;
// 	t--;
// 	return c * float sqrt(1 - t*t) + b;
// }
// float easeInOutCirc(t, b, c, d) { // circular easing in/out - acceleration until halfway, then deceleration
// 	t /= d/2;
// 	if (t < 1) return -c/2 * (float sqrt(1 - t*t) - 1) + b;
// 	t -= 2;
// 	return c/2 * (float sqrt(1 - t*t) + 1) + b;
// }

