#pragma once

#include "buffer.h"

// dunno, better name? fairly specific thing so could be called just interpolator but yeah
// also, should be
// tol::render::Stage
// tol::render::Renderer
// etc for them. tho makes little sense for io actually hahah
// ::pipe?
namespace tol {

// privileged stuff (friends of Buffer) that can do low level internal operations to affect one
// dunno if will even have other similar stuff but wanted to extract it from Buffer
// doesnt fit with the templating etc and different mode of operation/thinking
// should also go in IRAM
class Effector {
  public:
  virtual ~Effector() {};

  virtual bool execute(Buffer& receiver, const Buffer& origin, const Buffer& target, float progress) = 0;
};

class Interpolator: public Effector { // 
  public:
  virtual ~Interpolator() {};
  // should it be set up with Buffers as o/t/c or pass each time? whatever
  
  virtual bool execute(Buffer& receiver, const Buffer& origin, const Buffer& target, float progress) override; // input buffers being interpolated, weighting (0-255) of second buffer in interpolation
// blabla doubling up strips for added resolution -> hell yeah
// but also, stay on 32 bits further.  use all the space. now  8 i/o, 8 weight, 8 scale.  go like 10 weight, 10 scale?
// also carry over scale error same way we do pixels? tho then when higher applied next time existing residual is all wrong.

// I forget what actual purpose this has. interpolate to next frame yes, but for example without taking heed
// of nonlinear response (2 to 3 vs 15 to 16) so error != error
// not feasible switching back and forth 2 to 3 without flicker.
// nothing smooth about the flickering 0 to 1. was never pretty, but for color reasons.
// now just does not look smooth at all.


};

}
