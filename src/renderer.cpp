#include "renderer.h"


// update/expand for f updates etc...
void Renderer::updateTarget(const Buffer& mergeIn, uint8_t blendType, bool mix) {
  lwd.stamp(PIXTOL_RENDERER_UPDATE_TARGET);
  _numKeyFrames++;

  b[ORIGIN]->setCopy(*b[TARGET]); //current is possibly scaled down so cant use. this good enough for now.
  if(mix) mixBuffers(*b[TARGET], mergeIn, blendType); //shouldnt actually be here - blending has been done, now chase new target...
  else b[TARGET]->blendUsingEnvelopeB(*b[TARGET], mergeIn, 1.0, &f->e); //very good, no effect unless A/R
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

void Renderer::frame(float timeMultiplier) {
  lwd.stamp(PIXTOL_RENDERER_FRAME);
  uint32_t now = micros();
  static uint32_t timeToInterpolate = 0;
  float progress = (now - _lastKeyframe + timeToInterpolate) / (float)_keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...
  // we keep track of expected time for Buffer::interpolate and Strip flush and aim "later"
  if(timeMultiplier > 0) { // no data fall-back or far-apart frames...
    progress *= timeMultiplier;   // progress is then made up so no realism huhu
  }
  if(progress > 1.0) { // guess havent reflected on how rel little resolution needed / possible in progress anyways.  prob dont got much to win w float. nor to lose, oth...
    progress = fabs(fmod(1.0 + progress, 2) - 1.0); /* progress = fabs(sin(progress * PI - 0.5*PI)); //oh yeah dont even want a sine lol, just triangle */
  }
  // skip interp for early frames to bust more out?
  bool fullGain = (b[CURRENT]->getGain() > 0.95); // however need scaling elsewhere then if dimmer...
  if(fullGain && timeMultiplier == 0) {

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
  _lastFrame = micros();
  _lastProgress = progress;
  _numFrames++;
}
