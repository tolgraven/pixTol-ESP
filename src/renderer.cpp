#include "renderer.h"
#include "renderstage.h"

namespace tol {

Renderer::Renderer(const std::string& id, uint32_t keyFrameHz, uint16_t targetHz, const RenderStage& target):
  RenderStage(id + " renderer", target.fieldSize(),
              target.fieldCount(), target.buffers().size()),
  core::Task(id.c_str(), 4096, taskPrio,
             milliseconds{1000/targetHz},
             -1), // test pin cpu has to be dynamic or at least properly calculated heh
  Sub<PatchIn>(this),
  _keyFrameInterval(MILLION / keyFrameHz),
  targetFps(targetHz) {

  // guess we might want to pre-set size for these tho. then again objs themselves not big since consist of ptrs...
  for(int i=0; i < target.buffers().size(); i++) {
    auto& model = target.buffer(i);
    // lg.dbg((std::string)"Creating renderer buf " + std::to_string(i) + " for " + model.id() + ", size " + model.fieldSize() + "/" + model.fieldCount());
    // model.printTo(lg); // ln.print(model); // also works?? discontinue lol

    bs.push_back(std::vector<std::shared_ptr<Buffer>>());
    for(auto s: {"origin", "target", "out"}) {
      bs[i].emplace_back(new Buffer(id + " " + s + " " + std::to_string(i), model.fieldSize(), model.fieldCount()));
    }
    buffer(i).setDithering(true); // must for proper working. needs to be a setting tho!!

    lg.f(__func__, Log::DEBUG, "curr at %p ptr %p\n", &get(i, OUT), get(i, OUT).get());
  }
  if(auto order = target.buffer(0).getSubFieldOrder()) {
    setSubFieldOrder(order); // only sets it on "internal"/dest buffers so ORIGIN/TARGET not affected
    for(int i=0; i < bs.size(); i++) {
      // get(i, OUT).setSubFieldOrder(order);
      buffer(i).setSubFieldOrder(order);
    }
  }

  effectors.push_back(std::make_shared<Interpolator>());

  DEBUG("DONE");
}


void Renderer::updateTarget(const Buffer& mergeIn, int i, uint8_t blendType, bool mix) {
  if(i == 0) // is first buffer, so update counters
    _numKeyFrames++;

  Buffer toMerge(mergeIn); // need to adapt it to ours since source can be whatever
  // would be reasonable here, not interpolate, is where we actually utilize setSubFieldOrder
  // to shuffle stuff around. Related is transform to go from fo 3 to 4, 4 to 1 etc
  // but obviously auto transform not feasible since eg artnet has 1 as size but representing 4...
  toMerge.setFieldSize(get(i, TARGET).fieldSize());
  toMerge.setFieldCount(get(i, TARGET).fieldCount());

  get(i, ORIGIN).setCopy(get(i, TARGET)); // set old target as current origin.
  //  current is possibly scaled down so cant use. this good enough for now.
  if(mix)
    mixBuffers(get(i, TARGET), toMerge, blendType);
  else {
    // ideally if multiple Functions layers there'd be logic for prio, avg etc,
    // as far as envelopes...but for now
    BlendEnvelope* e = nullptr;
    if(!functions.empty())
      e = &functions[0]->e;
    get(i, TARGET).blendUsingEnvelopeB(get(i, ORIGIN), toMerge, 1.0, e); //very good, no effect unless A/R
  }
  if(i == 0) // is first buffer, so update counters
    _lastKeyframe = micros(); // maybe rather after ops...
}

// not sure why this is here yo. it's renderish I guess...
void Renderer::mixBuffers(Buffer& lhs, const Buffer& rhs, uint8_t blendType) {
  // gotta add optional progress + e to these.
    switch((BlendType)blendType) { //control via mqtt as first step tho
      case Avg_IgnoreZero:  lhs.avg(rhs, true); break; //this needs split sources to really work, or off-pixels never get set
      case Avg:             lhs.avg(rhs, false); break;
      case Add:             lhs.add(rhs); break;
      case Sub:             lhs.sub(rhs); break;
      case Htp:             lhs.htp(rhs); break;
      case Lotp_IgnoreZero: lhs.lotp(rhs, true); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
      case Lotp:            lhs.lotp(rhs, false); break;
  }
}

void Renderer::frame(float timeMultiplier) {
  uint64_t now = micros();
  static uint64_t timeLastRender = 0;
  static uint64_t timeSpentRendering = 0;
  
  float progress = (now - _lastKeyframe + timeLastRender) / (float)_keyFrameInterval; // we keep track of expected time for Buffer::interpolate and Strip flush and aim "later" by adding interpolation time...
  // but should probably use a running avg instead or single slow frame screws a bit with timing...
  // also shouldn't we try to send one "pure" frame directly maybe hmm. guess we're mostly waiting on strip anyways tho...
  
  // this only really makes sense in the incoming merge step, not here
  // what really ought to be happening is a Generator with low weight/alpha/whatever steps in and everything moves towards that.
  if(timeMultiplier > 0.0f)       // no data fall-back or far-apart frames...
    progress *= timeMultiplier;   // progress is then made up so no realism huhu
  
  if(progress > 1.0f) {
    progress = (timeMultiplier > 0.0f)? // bouncing only makes sense in slomo
               fabs(fmod(1.0f + progress, 2) - 1.0f):
               1.0f;
  } else if(progress < 0.0f)
    progress = 0.0f;

  std::vector<PatchOut> patches;
  for(int i=0; i < bs.size(); i++) { // well no, set a flag what's been updated or etc yada.
    auto& buf = buffer(i);
    auto& origin = get(i, ORIGIN);
    auto& target = get(i, TARGET);
    bool fullGain = buffer(i).getGain() > 0.95f; // skip interp for early frames to bust more out? such little time actually spent interp hehe
    if(false && fullGain && timeMultiplier == 0.0f) { // however need scaling elsewhere then if dimmer...
      // if(progress < 0.10f)      curr.setCopy(origin); // or eh what destructive we doing could maybe current temp pass through?
      // else if(progress > 0.90f) curr.setCopy(target);
      // else curr.interpolate(origin, target, progress);
      // I guess we're doing order shuffling in interpolate (good considering speed)
      // meaning original shite when straight copy = bad.
      // hence, do fieldOrder shuffle on copy not interp!
    } else {
      // curr.interpolate(origin, target, progress);
      for(auto& eff: effectors)
        eff->execute(buf, origin, target, progress);
    }
  }

  _numFrames++;
  if(!(_numFrames % 7500)) {
    lg.f(__func__, tol::Log::DEBUG, "Run: %lluus (avg %lluus) fps: %llu, key/frames %u %u\n",
          timeLastRender, timeSpentRendering / (_numFrames + 1),
          _numFrames / (1 + (_lastFrame / MILLION)),
          _numKeyFrames, _numFrames);
  }
  
  for(auto f: functions)
    f->apply(progress); // since these point to curr it doesn't matter bufs are cloned.

  for(int i=0; i < bs.size(); i++) {
    get(i, OUT).setCopy(buffer(i));       // double buffer so don't glitch when already rendering next by time event arrives
    patches.emplace_back(get(i, OUT), i); // real issue iirc was Strip being pointed straight to curr I think? but optimize later yo
  }
                                    
  
  for(auto& p: patches) { // TODO we are waiting on all in this case, so send one event hehe. Different in other scenarios..
    // ideally we would lock here and prevent next frame from doing anything until receivers have done their thing
    // since yet another round of copying seems wasteful, but will have to do that for now
    // (would passing a lock between threads even work?)
    ipc::Publisher<PatchOut>::publish(p); // publish "PatchOut" (poor name? not a one-time patch but like, current mapping, so...)
  }

  timeLastRender = micros() - now; // oh yeah will get wonky in case interpolate skips on own bc gain 0 etc...
  timeSpentRendering += timeLastRender;

  _lastFrame = micros();
  _lastProgress = progress;
}


}
