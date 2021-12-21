#include "base.h"

namespace tol {

namespace {
UID generateUID() {
    static UID i = 0;
    return ++i;
}
}

Named::Named(const String& id, const String& type):
      _id(id), _type(type), _uid(generateUID()) {}


void ChunkedContainer::setSubFieldOrder(const uint8_t subFields[]) {
  if(!subFields) return;
  delete subFieldOrder;
  subFieldOrder = new uint8_t[fieldSize()];
  memcpy(subFieldOrder, subFields, fieldSize());
}


bool Runnable::run() {
  // lwd.stamp(id().hash32Lala()); // obvs.
  uint32_t now = ts.attempt = micros();
  count.attempt++;
  bool outcome = false;
  if(!ready() || !_run()) {
    checkAndHandleTimeOut();

  } else { // success
    count.run++;
    if(!active()) {
      setActive(true);
      _onRestored();
    }
    ts.run = ts.attempt; // not so precise, but trickier if run > attempt. go int64? stick w millis? only few things (strobe...) need micros.
    outcome = true;
  } // here, maybe on throttled and def averaging basis, track actual hz

  // static uint32_t timeSinceAdjustedKeyframeInterval = 0;
  // if(_lastKeyframe > 0) timeSinceAdjustedKeyframeInterval += now - _lastKeyframe; //log says takes 3s between frame 1 and 2. Makes no sense...
  // _lastKeyframe = now;
  // _numKeyFrames++;
  // if(timeSinceAdjustedKeyframeInterval > keyFrameAdjustmentInterval) {
  //   static uint32_t numKeyFramesAtLastAdjustment = 0;
  //   setKeyFrameInterval((_numKeyFrames - numKeyFramesAtLastAdjustment) / timeSinceAdjustedKeyframeInterval);
  //   numKeyFramesAtLastAdjustment = _numKeyFrames;
  //   timeSinceAdjustedKeyframeInterval = 0;
  // }
  count.totalTime += micros() - ts.attempt; // timeInTask += passedSince(TLastAttempt); //
  // should work round overflow by also overflowing, right?  still our numbers end up fucked...
  return outcome;
}

bool Runnable::ready() { // no const cause conditions actually get updated and shit
  return (enabled()
      && (microsTilReady() == 0)
      // && conditions.ready()    // I guess we mind this not being called since also updates conditions? Or only count "real" run-attempts??
      && _ready());
}
// should be int so -1 can mean n/a? or keep going for "hit me earlier next time ya fucker"
uint32_t Runnable::microsTilReady() const { // so this version can only know high-level / enforced stuff
  if(targetHz == 0) { // still need that HW info and stuff sometimes... but hence should again do private virtual impl fn and make this regular
    return 0;
  } else { // throttle Hz
    return (uint32_t)std::max((1000000 / targetHz) - (int32_t)passed(), (int32_t)0);
  }
}


void Runnable::checkAndHandleTimeOut() {
  if(active() && idleTimeout > 0
      && (ts.attempt - ts.run > idleTimeout * 1000)) {
      setActive(false);
      _onTimeout();
  }
}

}
