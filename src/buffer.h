#pragma once

#include "renderstage.h"

class Buffer: public RenderStage {
  public:
    void update(uint8_t* data, uint16_t length=512, uint16_t offset=0);
    void finalize(); // lock in keyframe, at that point can be flushed or interpolated against
    // hopefully ok performance - workflow becomes keep two
    // buffer objects, "current" and "target", and continously pass target to current
    // to interpolate? Or do one, save diff and reapply
    // Also save buffers continously and mix them together while rotating and manipulating each continousy... =)
    uint8_t* get(uint16_t length=0, uint16_t offset=0); // length=0 should automatically get full buffer

  private:
    uint8_t* data;
}
