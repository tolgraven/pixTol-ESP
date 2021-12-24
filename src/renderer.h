#pragma once

#include <vector>
#include <memory>

#include "renderstage.h"
#include "buffer.h"
#include "effector.h"
#include "functions.h"
#include "log.h"
#include "watchdog.h"
#include "task.h"

// dunno to much afa this and events. cant go faster than output, but neither wait for it to be ready...
// rather should be done when output assumed to be ready.

namespace tol {

const int rendererBaseTargetFPS = 1000;
  

class Renderer: public RenderStage,
                public core::Task,
                public Sub<PatchIn> {

  public:
  enum BlendType: uint8_t { Avg_IgnoreZero = 0, Avg,
                            Add, Sub, Htp,
                            Lotp_IgnoreZero, Lotp };
  enum Role: uint8_t { ORIGIN = 0, TARGET, CURRENT }; // i reckon "result" for current? less ambigous
  uint32_t _numKeyFrames = 0, _numFrames = 0;
  float _lastProgress = 0;
  std::vector<std::shared_ptr<Functions>> functions;
  std::vector<std::shared_ptr<Effector>> effectors; // tho same concept could also be run on input especially if more intensive...

  Renderer(const std::string& id, uint32_t keyFrameInterval, uint16_t targetHz, const RenderStage& target);
  
  void addEffectManager(Functions* fun) {
    functions.emplace_back(fun);
  }

  void updateTarget(const Buffer& mergeIn, int index, uint8_t blendType = 0, bool mix = false);

  void onEvent(const PatchIn& in) override  { updateTarget(in.buffer, in.destIdx); }
  // should have FunctionsRunner (Functions not a good name in general for it tho. Effects? Transforms?)
  // which subscribes to this (with num mapping to registered FnChannels) as well as single-field events
  // which contains name and looks up that way.
  // BTW: multi param Functions, duh. would have to take a buf and do their own lookup so stays uniform
  // (= every Function gets a buffer, most are just single-field)
  // ALSO: a way to eg map two DMX fields to 16bit int for higher precision (since using float internally anyways)
  void tick() override { frame(); } // not that any of this is design but remember will have to adjust rate so very much cant be like this.

  void frame(float timeMultiplier = 0.0f);


  Buffer& getDestination(int i = 0) { return get(i, CURRENT); }
  Buffer& get(int index, Role role) {
    if(role == CURRENT)
      return buffer(index);
    return *bs[index][role];
  }
  void crapLogging();

  private:
  const static int taskPrio = 18;

  uint32_t _keyFrameInterval, _lastKeyframe = 0, _lastFrame = 0; // still need tracking expected interval dep on srces yada but go away these...
  int targetFps = rendererBaseTargetFPS;
  const uint32_t keyFrameAdjustmentInterval = 1000000;
  std::vector<std::vector<std::shared_ptr<Buffer>>> bs;

  static void mixBuffers(Buffer& lhs, const Buffer& rhs, uint8_t blendType = 0);
};

}
