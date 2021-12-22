#include "renderer.h"

namespace tol {

Renderer::Renderer(const String& id, uint32_t keyFrameInterval, const RenderStage& target):
  Runnable(id, "renderer"),
  core::Task(id.c_str(), 4096, taskPrio,
             milliseconds{1000/rendererBaseTargetFPS}, // got div by zero, guess makes sense not yet initialized...
             0), // test pin cpu has to be dynamic or at least properly calculated heh
  Sub<PatchIn>(this), // I suppose these of course should run under a different thread duh! or just, may not interrupt frame mid-render..
  Sub<PatchControls>(this),
  _keyFrameInterval(keyFrameInterval) {

  // guess we might want to pre-set size for these tho. then again objs themselves not big since consist of ptrs...
  for(int i=0; i < target.buffers().size(); i++) {
    auto& model = target.buffer(i);
    lg.dbg((String)"Creating renderer buf " + i + " for " + model.id() + ", size " + model.fieldSize() + "/" + model.fieldCount());
    model.printTo(lg); // ln.print(model); // also works?? discontinue lol

    bs.push_back(std::vector<std::shared_ptr<Buffer>>());
    for(auto s: {"origin", "target", "current"}) {
      bs[i].emplace_back(new Buffer(id + " " + s + " " + i, model.fieldSize(), model.fieldCount()));
    }
    get(i, CURRENT).setPtr(model); // in turn directly pointed to Strip buffer (for DMA anyways, handled internally anyways)
    get(i, CURRENT).setDithering(true); // must for proper working
    auto order = model.getSubFieldOrder();
    if(order) get(i, CURRENT).setSubFieldOrder(order);

    lg.f(__func__, Log::DEBUG, "curr at %p ptr %p\n", &*bs[i][CURRENT], &*bs[i][CURRENT]->get());
  }

  f = new Functions(get(0, CURRENT)); // current (w pixelbuf shared with Output) is target.  reason stands that here (or rather when creating here) is when we patch in fns so create a Functions, specify which, mapping, passing current as buffer target
  controls = new Buffer("Controls", 1, f->numChannels); // for merging fn sources

  DEBUG("DONE");
}

void Renderer::updateControls(const Buffer& controlData, bool blend) {
    if(blend) controls->avg(controlData, false);
    else      controls->set(controlData);
    f->update(controls->get());
}

void Renderer::updateTarget(const Buffer& mergeIn, int i, uint8_t blendType, bool mix) {
  lwd.stamp(PIXTOL_RENDERER_UPDATE_TARGET);
  _numKeyFrames++;

  Buffer toMerge(mergeIn); // need to adapt it to ours since source can be whatever
  // would be reasonable here, not interpolate, is where we actually utilize setSubFieldOrder
  // to shuffle stuff around. Related is transform to go from fo 3 to 4, 4 to 1 etc
  // but obviously auto transform not feasible since eg artnet has 1 as size but representing 4...
  toMerge.setFieldSize(get(i, TARGET).fieldSize());
  toMerge.setFieldCount(get(i, TARGET).fieldCount());

  get(i, ORIGIN).setCopy(get(i, TARGET)); // set old target as current origin.
  //  current is possibly scaled down so cant use. this good enough for now.
  if(mix) mixBuffers(get(i, TARGET), toMerge, blendType);
  else get(i, TARGET).blendUsingEnvelopeB(get(i, TARGET), toMerge, 1.0, &f->e); //very good, no effect unless A/R
  // else get(i, TARGET).setCopy(mergeIn);
  _lastKeyframe = micros(); // maybe rather after ops...
}

void Renderer::mixBuffers(Buffer& lhs, const Buffer& rhs, uint8_t blendType) {
  // gotta add optional progress + e to these.
    switch(blendType) { //control via mqtt as first step tho
      case 0: lhs.avg(rhs, true); break; //this needs split sources to really work, or off-pixels never get set
      case 1: lhs.avg(rhs, false); break;
      case 2: lhs.add(rhs); break;
      case 3: lhs.sub(rhs); break;
      case 4: lhs.htp(rhs); break;
      case 5: lhs.lotp(rhs, true); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
      case 6: lhs.lotp(rhs, false); break;
      case 7: lhs.htp(rhs); break;
  }
}


void Renderer::frame(float timeMultiplier) {
  uint32_t now = micros();
  static uint32_t timeLastRender = 0;
  static uint32_t timeSpentRendering = 0;
  
  float progress = (now - _lastKeyframe + timeLastRender) / (float)_keyFrameInterval; // we keep track of expected time for Buffer::interpolate and Strip flush and aim "later" by adding interpolation time...
  
  // this only really makes sense in the incoming merge step, not here
  // what really ought to be happening is a Generator with low weight/alpha/whatever steps in and everything moves towards that.
  if(timeMultiplier > 0.0f)       // no data fall-back or far-apart frames...
    progress *= timeMultiplier;   // progress is then made up so no realism huhu
  
  if(progress > 1.0f)
    progress = 1.0f; // bouncing is neat idea but prob means progress 0.3 0.6 0.9 0.8... when should be 1.0 in that case.
    // progress = fabs(fmod(1.0f + progress, 2) - 1.0f); /* progress = fabs(sin(progress * PI - 0.5*PI)); //oh yeah dont even want a sine lol, just triangle */
  else if(progress < 0.0f)
    progress = 0.0f;

  std::vector<PatchOut> patches;
  for(int i=0; i < bs.size(); i++) { // well no, set a flag what's been updated or etc yada.
    auto& curr = get(i, CURRENT);
    auto& origin = get(i, ORIGIN);
    auto& target = get(i, TARGET);
    bool fullGain = curr.getGain() > 0.95f; // skip interp for early frames to bust more out? such little time actually spent interp hehe
    if(false && fullGain && timeMultiplier == 0.0f) { // however need scaling elsewhere then if dimmer...
      if(progress < 0.10f)      curr.setCopy(origin); // or eh what destructive we doing could maybe current temp pass through?
      else if(progress > 0.90f) curr.setCopy(target);
      else curr.interpolate(origin, target, progress);
      // I guess we're doing order shuffling in interpolate (good considering speed)
      // meaning original shite when straight copy = bad.
      // hence, do fieldOrder shuffle on copy not interp!
    } else {
      curr.interpolate(origin, target, progress);
    }

    patches.emplace_back(curr, i); // at least curr RMT method we want these synchronized.
  }

  _numFrames++;
  if(!(_numFrames % 5000)) {
    lg.f(__func__, tol::Log::DEBUG, "Time last run: %u (avg %u), render fps: %u, keyFrames/frames %u %u\n",
          timeLastRender, timeSpentRendering / (_numFrames + 1), _numFrames / (1 + (_lastFrame / MILLION)), _numKeyFrames, _numFrames);
  }

  f->apply(progress); // XXX not going to affect all buffers yet
  
  for(auto& p: patches) { // TODO we are waiting on all in this case, so send one event hehe. Different in other scenarios..
    ipc::Publisher<PatchOut>::publish(p); // publish "PatchOut" (poor name? not a one-time patch but like, current mapping, so...)
  }

  timeLastRender = micros() - now; // oh yeah will get wonky in case interpolate skips on own bc gain 0 etc...
  timeSpentRendering += timeLastRender;

  _lastFrame = micros();
  _lastProgress = progress;
}


}
