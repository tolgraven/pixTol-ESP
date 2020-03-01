#pragma once

#include <vector>
#include <memory>

// #include <lib8tion.h>
#include "renderstage.h"
#include "buffer.h"
#include "functions.h"
#include "log.h"
#include "watchdog.h"


class Renderer: public Runnable {
  private:
    uint32_t _keyFrameInterval, _lastKeyframe = 0, _lastFrame = 0; // soo, mapping this to runnable.  Or having some specialized where poll/sucess is rather inter/keyframe
    const uint32_t keyFrameAdjustmentInterval = 1000000;
    std::vector<std::vector<std::shared_ptr<Buffer>>> bs;

    static void mixBuffers(Buffer& lhs, const Buffer& rhs, uint8_t blendType = 0);
  public:
    // enum BlendType: uint8_t { Avg_IgnoreZero = 0, Avg, Add, Sub, Htp, Lotp_IgnoreZero, Lotp };
    enum SubBuffer: uint8_t { ORIGIN = 0, TARGET, CURRENT }; // i reckon "result" for current? less ambigous
    uint32_t _numKeyFrames = 0, _numFrames = 0;
    float _lastProgress = 0;
    Buffer* controls = nullptr;
    Functions* f = nullptr;

    Renderer(const String& id, uint32_t keyFrameInterval, const RenderStage& target):
      Runnable(id, "renderer"), _keyFrameInterval(keyFrameInterval) {

      auto&& model = target.buffer();
      lg.dbg("Create renderer for " + model.id() + ", size " + model.fieldSize() + "/" + model.fieldCount());
      model.printTo(lg);
      // ln.print(model); // also works??

      for(int i=0; i < target.buffers().size(); i++) {
        bs.push_back(std::vector<std::shared_ptr<Buffer>>());
        for(auto s: {"origin", "target", "current"}) {
          bs[i].emplace_back(new Buffer(id + " " + s, model.fieldSize(), model.fieldCount()));
        }
        bs[i][CURRENT]->setPtr(model); // in turn directly pointed to Strip buffer (for DMA anyways, handled internally anyways)
        bs[i][CURRENT]->setDithering(true); // must for proper working
        auto order = model.getSubFieldOrder();
        if(order) bs[i][CURRENT]->setSubFieldOrder(order);

        lg.f(__func__, Log::DEBUG, "curr at %p ptr %p\n", &*bs[i][CURRENT], &*bs[i][CURRENT]->get());
      }

      f = new Functions(*bs[0][CURRENT]); // current (w pixelbuf shared with Output) is target.  reason stands that here (or rather when creating here) is when we patch in fns so create a Functions, specify which, mapping, passing current as buffer target
      controls = new Buffer("Controls", 1, f->numChannels); // for merging fn sources
      DEBUG("DONE");
    }
  // void setKeyFrameInterval(uint32_t newInterval) { _keyFrameInterval = newInterval; }
  // uint32_t getKeyFrameInterval() const { return _keyFrameInterval; }

  void updateControls(const Buffer& controlData, bool blend);
  void updateTarget(const Buffer& mergeIn, int index, uint8_t blendType = 0, bool mix = false);
  void frame(float timeMultiplier = 0);
  bool _run() override { frame(); return true; }

  Buffer& getDestination(int i = 0) { return *bs[i][CURRENT].get(); }
  void crapLogging();
};
