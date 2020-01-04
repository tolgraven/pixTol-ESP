#include "renderer.h"


// update/expand for f updates etc...
void Renderer::updateTarget(Buffer& mergeIn, uint8_t blendType, bool mix) {
  lwd.stamp(PIXTOL_RENDERER_UPDATE_TARGET);
  _numKeyFrames++;

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

  origin->setCopy(*target); //current is possibly scaled down so cant use. this good enough for now.

  if(mix) mixBuffers(*target, mergeIn, blendType); //shouldnt actually be here - blending has been done, now chase new target...
  else target->blendUsingEnvelopeB(*target, mergeIn, 1.0, &f->e); //very good.

  _lastKeyframe = micros(); // maybe rather after ops...
}

void Renderer::mixBuffers(Buffer& lhs, Buffer& rhs, uint8_t blendType) {
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
  float progress = (now - _lastKeyframe) / (float)_keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...

  if(timeMultiplier) {
    progress *= timeMultiplier; // shift fallback or far-apart frames...
  }
  if(progress > 1.0) { // guess havent reflected on how rel little resolution needed / possible in progress anyways.  prob dont got much to win w float. nor to lose, oth...
    /* progress = fabs(sin(progress * PI - 0.5*PI)); //oh yeah dont even want a sine lol, just triangle */
    progress = fabs(fmod(1.0 + progress, 2) - 1.0);
  }
  /* lg.f(__func__, Log::DEBUG, "%.3f \n", progress); */
  f->apply(progress); // this will have to go before interpolation so gain gets applied... but wait from last frame should be there? run f, which has current as a direct target - come to think of it why not pass buf each time rather than bind. maybe hmm
  current->interpolate(*origin, *target, progress, &f->e);

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
