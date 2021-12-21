#include "functions.h"

namespace tol {

bool fIsZero(float value) { return(value < EPSILON); }
bool fIsEqual(float lhs, float rhs) { return(std::abs(lhs - rhs) < EPSILON); }

float Strober::_apply(float value, float progress) {
  if(fIsZero(value)) { //end effect
    if(!fIsZero(current)) {
      /* lg.f(__func__, tol::Log::DEBUG, "Strobe ending"); */
      result = 1.0f;
    }
  } else {
    uint16_t intVal = value * 255, intCurr = current * 255;
    if(intVal != intCurr) {
      start(value); // reset timer for new state
    } else {
      tick(micros() - lastTick); // handle running strobe: decr/reset tickers, adjust shutter
    }
    lastTick = micros();
  }
  return value;
}

void Strober::start(float value) {
  float hz = (hzMax-hzMin) * value + hzMin;
  timeInStage[TOTAL]  = MILLION / hz;
  timeInStage[PRE]    = 50000; //debounce
  timeInStage[OPEN]   = std::min((uint32_t)(timeInStage[TOTAL] * fraction[OPEN]), maxTime[OPEN]);
  timeInStage[FADING] = std::min((uint32_t)(timeInStage[TOTAL] * fraction[FADING]), maxTime[FADING]);
  timeInStage[CLOSED] = timeInStage[TOTAL] - timeInStage[OPEN] - timeInStage[FADING];
  activeStage = PRE;
  // lastTick = micros();

  initStage();
  /* lg.f(__func__, tol::Log::TRACE, "val %.2f, hz %.1f, period %u ms", value, hz, timeInStage[TOTAL]/1000); */
  // ^ unsure why this was uncommented, still causing crashing hah! also super fucked
}

void Strober::initStage(uint32_t debt) {
  switch(activeStage) {
    case PRE:    result = 0.0f; break; //in some cases debounce = open might work better though...
    case OPEN:   result = 1.0f; break;
    case FADING: result = 1.0f; break;
    case CLOSED: result = 0.0f; break;
  }
  timeRemaining = timeInStage[activeStage] - debt;
  /* lg.f("Strober", tol::Log::TRACE, "activeStage: %u, current: %.2f, open: %.2f\n", activeStage, current, result); */
}

void Strober::tick(uint32_t passedMicros) {
  timeRemaining -= passedMicros; // use that as control counter instead
  if(timeRemaining <= 0) {
    activeStage++;
    if(activeStage > CLOSED) activeStage = OPEN; // roll over, skipping pre and (non-stage) TOTAL
    initStage(-timeRemaining); // pass on any debt incurred by late update
  }
  if(activeStage == FADING) {
    result -= std::min(passedMicros / (float)timeInStage[FADING], result);
  }
  // lastTick = micros();
}



float Rotate::_apply(float value, float progress) {
  if(fIsZero(value)) return 0.f;
  if(forward) b->rotate(value); // these would def benefit from anti-aliasing = decoupling from leds as steps
  else b->rotate(-value); // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
  return value;
}


float Bleed::_apply(float value, float progress) {
  if(fIsZero(value)) return 0.f;

  /* std::vector<Field> color; */
  /* Field color[4] = {b->fieldSize()}; */
  uint8_t fs = b->fieldSize();
  Field color[4] = {fs, fs, fs, fs};
  /* // well this needs fixed p math etc */

  for(int pixel = 0; pixel < b->fieldCount(); pixel++) {

    color[THIS] = b->getField(pixel); //this one could be in-place tho..
    auto thisBrightness = color[THIS].average(); //tho W oughta contribute more yeah
    auto brightnessForWeight = (thisBrightness > 0? thisBrightness: 1);
    float weight[2]; // float prevWeight, nextWeight;

    if(pixel-1 >= 0) { //could be tiny fn
      color[BEHIND] = b->getField(pixel-1);
      weight[BEHIND] = color[BEHIND].average() / (float)brightnessForWeight;
      if(weight[BEHIND] < 0.3f) { // skip if dark, test. Should use weight and scale smoothly instead...
        color[BEHIND] = color[THIS];
      }
    } else {
      color[BEHIND] = color[THIS];
      weight[BEHIND] = 1.f;
    }

    if(pixel+1 < b->fieldCount()) {
      color[AHEAD] = b->getField(pixel+1);
      weight[AHEAD] = color[AHEAD].average() / (float)brightnessForWeight;
      if(weight[AHEAD] < 0.3f) { // do some more shit tho. Important thing is mixing colors, so maybe look at saturation as well as brightness dunno
        color[AHEAD] = color[THIS];  // since we have X and Y should weight towards more interesting.
      }
    } else {
      color[AHEAD] = color[THIS];
      weight[AHEAD] = 1.f;
    }
    auto amount = value/2.f;  //this way only ever go half way = before starts decreasing again
    // how does progress enter picture?

    /* color[BLEND] = Field::blend(color[BEHIND], color[AHEAD], 0.5f); //test */
    /* color.insert(std::pair<LOCATION, Field>(BLEND, Field::blend(color[BEHIND], color[AHEAD], 0.5f))); //test */
    /* color.emplace(BLEND, Field::blend(color[BEHIND], color[AHEAD], 0.5f)); //test */
        // (1 / (prevWeight + weight[AHEAD] + 1)) * prevWeight); //got me again loll
        // color[THIS] = RgbwColor::BilinearBlend(color[THIS], color[AHEAD], color[BEHIND], color[BLEND], amount, amount);
    /* color[THIS] = Field::blend(color[THIS], color[AHEAD], color[BEHIND], color[THIS], amount, amount); */
    color[THIS].blend(color[AHEAD], color[BEHIND], color[THIS], amount, amount);
    /* color.insert(std::pair<LOCATION, Field>(THIS, Field::blend(color[THIS], color[AHEAD], color[BEHIND], color[THIS], amount, amount))); */
    /* color.emplace(THIS, Field::blend(color[THIS], color[AHEAD], color[BEHIND], color[THIS], amount, amount)); */

    b->setField(color[THIS], pixel); // XXX handle flip and that
  }
  return value;
}


float RotateHue::_apply(float value, float progress) {
  if(fIsZero(value)) return 0.f;
  // should be applying value _while looping around pixels and already holding ColorObjects.
  // convert Rgbw>Hsl? can do Hsb to Rgbw at least...
  /* b->rotateHue(value); //tho should use progress to interpolate also these kinds of things hey... */
  // tho that's happening through public apply() or?
  return value;
}


float Noise::_apply(float value, float progress) {
  if(fIsZero(value)) return 0.f;

  maxNoise = (1 + value) / 4 + maxNoise; // CH_NOISE at max gives +-48
  // uint8_t* data = s->getBuffer();
  /* auto* data = s->getBuffer(); */

  /* static uint8_t iteration = 0; */
  /* for(int pixel = 0; pixel < s->fieldCount(); pixel++) { */
  /*   uint8_t subPixel[s->fieldSize()]; */
  /*   for(uint8_t i=0; i < s->fieldSize(); i++) { */
  /*     if(iteration == 1) subPixelNoise[pixel][i] = random(maxNoise) - maxNoise/2; */
  /*     else if(iteration >= 100) iteration = 0; //arbitrary value */
  /*     iteration++; */

  /*     subPixel[i] = data[pixel * s->fieldSize() + i]; //pointer */
  /*     if(subPixel[i] > 16) { //cutoff. XXX fix so bound works */
  /*       // subPixel[i] = bound<int>(subPixel[i] + subPixelNoise[pixel][i], 0, 255); */
  /*       // int tot = subPixel[i] + subPixelNoise[pixel][i]; */
  /*       // subPixel[i] = tot < 0? 0: (tot > 255? 255: tot); */
  /*       // subPixel[i] = constrain(tot, 0, 255); */
  /*     } */
  /*   } */
  /* } */
  return value;
}

}
