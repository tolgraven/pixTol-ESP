#include "renderer.h"

void Renderer::updateControls(const Buffer& controlData, bool blend) {
    if(blend) controls->avg(controlData, false);
    else      controls->set(controlData);
    f->update(controls->get());
}

void Renderer::updateTarget(const Buffer& mergeIn, int index, uint8_t blendType, bool mix) {
  lwd.stamp(PIXTOL_RENDERER_UPDATE_TARGET);
  _numKeyFrames++;

  bs[index][ORIGIN]->setCopy(*bs[index][TARGET]); //current is possibly scaled down so cant use. this good enough for now.
  if(mix) mixBuffers(*bs[index][TARGET], mergeIn, blendType); //shouldnt actually be here - blending has been done, now chase new target...
  else bs[index][TARGET]->blendUsingEnvelopeB(*bs[index][TARGET], mergeIn, 1.0, &f->e); //very good, no effect unless A/R
  // could possibly apply f here to mergeIn directly? uh

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

void Renderer::frame(float timeMultiplier) { // this hack can be achieved other ways, then
  lwd.stamp(PIXTOL_RENDERER_FRAME);          // turn it run() / Runnable.
  uint32_t now = micros();
  _numFrames++;

  static uint32_t timeToInterpolate = 0;
  float progress = (now - _lastKeyframe + timeToInterpolate) / (float)_keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...
  // we keep track of expected time for Buffer::interpolate and Strip flush and aim "later"
  if(timeMultiplier > 0) { // no data fall-back or far-apart frames...
    progress *= timeMultiplier;   // progress is then made up so no realism huhu
  }
  if(progress > 1.0) { // guess havent reflected on how rel little resolution needed / possible in progress anyways.  prob dont got much to win w float. nor to lose, oth...
    progress = fabs(fmod(1.0 + progress, 2) - 1.0); /* progress = fabs(sin(progress * PI - 0.5*PI)); //oh yeah dont even want a sine lol, just triangle */
  }
  // bool fullGain = (b[CURRENT]->getGain() > 0.95); // skip interp for early frames to bust more out? such little time actually spent interp hehe
  // if(fullGain && timeMultiplier == 0) { // however need scaling elsewhere then if dimmer...
  //   if(progress < 0.10)      b[CURRENT]->setCopy(*b[ORIGIN]); // or eh what destructive we doing could maybe current temp pass through?
  //   else if(progress > 0.90) b[CURRENT]->setCopy(*b[TARGET]);

  // } else {
  for(int i=0; i < bs.size(); i++) { // well no, set a flag what's been updated or etc yada.
    bs[i][CURRENT]->interpolate(*bs[i][ORIGIN], *bs[i][TARGET], progress, &f->e);
    timeToInterpolate = micros() - now; // oh yeah will get wonky in case interpolate skips on own bc gain 0 etc...
  }
  // }

  if(!(_numFrames % 25000)) {
    lg.f(__func__, Log::DEBUG, "Time one run: %u, render fps: %u, keyFrames/frames %u %u, last %u\n",
          timeToInterpolate, _numFrames / (1 + (_lastFrame / MILLION)), _numKeyFrames, _numFrames, _lastFrame);
    // uhh beware div by 0 no matter where. but wtf no fn info or fucking anything but why didnt this cause crashing the entire time??
  }
  f->apply(progress); // XXX not going to affect all buffers yet

  // can also take fair bit of time dep on effect. points straight at *current...
  // now means we might inter but then f opens or closes shutter -> wait til next time...
  // but if apply before inter must be to something else or eg rotation lost...
  //
  // issue most fx make sense run only each dmx frame, plus is destructive
  // try various options.
  // YET: got interp/envelope on that side too, and Strobe must run each frame anyways.
  //
  // reason says Strobe keep pointing to *current, updates before inter
  // hence dimmer too (also due to env), bleed, noise run on incoming, rotate too for now (for speed reasons), tho with lfo driving and rescale/basic AA faster = cooler...

  _lastFrame = micros();
  _lastProgress = progress;
}

void Renderer::crapLogging() {
  // in frame...
  //
  /* uint8_t *oD = origin->get(), *tD = target->get(), *cD = current->get(); */
  /* for(uint16_t f=0; f < current->fieldCount(); f++) { */
  /*   for(uint8_t sub=0; sub < current->fieldSize(); sub++) { */
  /*   if(f == 30 && sub == 0) { */
  /*     int off = f * current->fieldSize(); */
  /*     lg.f(__func__, Log::DEBUG, */
  /*         "\t   o/t c:  %u\t%u \t %u\n", */
  /*         *(oD + off), *(tD + off), *(cD + off)); */
  /*   } */
  /*   } */
  /* } */

  // in updateTarget..
  /* uint8_t *oD = origin->get(), *tD = target->get(), *cD = current->get(), *mD = mergeIn.get(); */
  /* /1* for(uint16_t f=0; f < mergeIn.fieldCount(); f++) { *1/ */
  /*   /1* for(uint8_t sub=0; sub < mergeIn.fieldSize(); sub++) { *1/ */
  /*   /1* if(f == 0) { *1/ */
  /*     /1* int off = f * mergeIn.fieldSize(); *1/ */
  /*     lg.f(__func__, Log::DEBUG, */
  /*         "o/t c in:  %u %p \t%u %p \t %u %p   %u %p\n", */
  /*         *(oD + 3), oD, */
  /*         *(tD + 3), tD, */
  /*         *(cD + 3), cD, */
  /*         *(mD + 3), mD); */
  /*   /1* } *1/ */
  /*   /1* } *1/ */
  /* /1* } *1/ */
}
