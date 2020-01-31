#pragma once

#include <lib8tion.h>

#include "renderstage.h"
#include "buffer.h"
#include "functions.h"
#include "log.h"
#include "watchdog.h"


class Renderer {
  private:
    String _id;
    uint32_t _keyFrameInterval, _lastKeyframe = 0, _lastFrame = 0;
    const uint32_t keyFrameAdjustmentInterval = 1000000;
    std::vector<std::unique_ptr<Buffer>> b;

    static void mixBuffers(Buffer& lhs, const Buffer& rhs, uint8_t blendType = 0);
  public:
    enum BlendType: uint8_t {
      Avg_IgnoreZero = 0, Avg, Add, Sub, Htp, Lotp_IgnoreZero, Lotp
    };
    enum SubBuffer: uint8_t {
      ORIGIN = 0, TARGET, CURRENT // i reckon "result" for current? less ambigous
    };
    uint32_t _numKeyFrames = 0, _numFrames = 0;
    float _lastProgress = 0;
    Buffer* controls = nullptr;
    Functions* f = nullptr;

    Renderer(const String& id, uint32_t keyFrameInterval, const Buffer& model):
      _id(id), _keyFrameInterval(keyFrameInterval) {

      lg.dbg("Create renderer for " + model.id() + ", size " + model.fieldSize() + "/" + model.fieldCount());
      for(auto s: {"origin", "target", "current"}) {
        b.emplace_back(new Buffer(_id + " " + s, model.fieldSize(), model.fieldCount()));
      }

      b[CURRENT]->setPtr(model); // in turn directly pointed to Strip buffer (for DMA anyways, handled internally anyways)
      b[CURRENT]->setDithering(true); // must for proper working
      b[CURRENT]->setSubFieldOrder(model.getSubFieldOrder());

      lg.f(__func__, Log::DEBUG, "curr at %p ptr %p\n", &*b[CURRENT], &*b[CURRENT]->get());
      f = new Functions(*b[CURRENT]); // current (w pixelbuf shared with Output) is target.  reason stands that here (or rather when creating here) is when we patch in fns so create a Functions, specify which, mapping, passing current as buffer target
      controls = new Buffer("Controls", 1, f->numChannels); // for merging fn sources
    }
  void setKeyFrameInterval(uint32_t newInterval) { _keyFrameInterval = newInterval; }
  uint32_t getKeyFrameInterval() const { return _keyFrameInterval; }

  void updateTarget(const Buffer& mergeIn, uint8_t blendType = 0, bool mix = false);
  void frame(float timeMultiplier = 0);

  Buffer& getDestination() { return *b[CURRENT]; }
  void crapLogging();
};
