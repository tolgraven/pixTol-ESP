#pragma once

#include <lib8tion.h>

#include "renderstage.h"
#include "buffer.h"
#include "functions.h"
#include "log.h"
#include "watchdog.h"

/* class Timed { */
/*   uint32_t start = 0, now = 0; */
/*   uint32_t updates = 0, runs = 0; */
/* }; */

class Renderer {
  private:
    String _id;
    uint32_t _keyFrameInterval, _lastKeyframe = 0, _lastFrame = 0;
    const uint32_t keyFrameAdjustmentInterval = 1000000;

    static void mixBuffers(Buffer& lhs, Buffer& rhs, uint8_t blendType = 0);
  public:
    uint32_t _numKeyFrames = 0, _numFrames = 0;
    float _lastProgress = 0;
    Buffer* controls = nullptr;
    Functions* f = nullptr;
    Buffer *origin, *target, *current;

    Renderer(const String& id, uint32_t keyFrameInterval, Buffer& model):
      _id(id), _keyFrameInterval(keyFrameInterval) {

      lg.dbg("Create renderer for " + model.id() + ", size " + model.fieldSize() + "/" + model.fieldCount());
      current = new Buffer(_id + " current", model.fieldSize(), model.fieldCount());
      origin = new Buffer(_id + " origin", model.fieldSize(), model.fieldCount());
      target = new Buffer(_id + " target", model.fieldSize(), model.fieldCount());
      lg.f(__func__, Log::DEBUG, "curr at %p ptr %p\n", current, current->get());
      f = new Functions(*current); // current (w pixelbuf shared with Output) is target
      // reason stands that here (or rather when creating here) is when we patch in fns so create a Functions, specify which, mapping, passing current as buffer target
      controls = new Buffer("Controls", 1, f->numChannels); // for merging fn sources
    }
  void setKeyFrameInterval(uint32_t newInterval) { _keyFrameInterval = newInterval; }
  uint32_t getKeyFrameInterval() { return _keyFrameInterval; }

  void updateTarget(Buffer& mergeIn, uint8_t blendType = 0, bool mix = false);
  void frame(float timeMultiplier = 0);

  Buffer& getDestination() { return *current; }
};
