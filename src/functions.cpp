#include "functions.h"

float Shutter::_apply(float value, float progress, Strip& s, bool force) {
    if(value == 0.0) { //end effect
      // if(current) LN.logf(__func__, LoggerNode::DEBUG, "Strobe ending");
      open = true;
      current = 0;
      return current;
    }
    if(value != current) {
      start(value); // reset timer for new state
    } else {
      tickStrobe(keyFrameInterval * (progress - lastProgress)); // handle running strobe: decr/reset tickers, adjust shutter
    }
    lastProgress = progress;
    current = value;
    return current;
}

void Shutter::start(float value) {
  float hz = (hzMax-hzMin) * value + hzMin;
  micros[TOTAL]  = MILLION / hz;
  micros[PRE]    = 30000; //debounce
  micros[OPEN]   = std::min((uint32_t)(micros[TOTAL] * fraction[OPEN]), maxMicros[OPEN]);
  micros[FADING] = std::min((uint32_t)(micros[TOTAL] * fraction[FADING]), maxMicros[FADING]);
  micros[CLOSED] = micros[TOTAL] - micros[OPEN] - micros[FADING];
  activeStage = PRE;

  initStage();
  LN.logf(__func__, LoggerNode::DEBUG, "val %.2f, hz %.1f, period %d ms", value, hz, micros[TOTAL]/1000);
  LN.logf(__func__, LoggerNode::DEBUG, "%d/%d/%d/%d/%d OPEN/FADING/CLOSED/PRE",
      micros[0]/1000, micros[1]/1000, micros[2]/1000, micros[4]/1000);
}

void Shutter::initStage(uint32_t debt) {
  switch(activeStage) {
    case PRE:    open = false; amountOpen = 0;   break;
    case OPEN:   open = true;                    break;
    case FADING: open = false; amountOpen = 255; break;
    case CLOSED:               amountOpen = 0;   break;
  }
  microsStage = micros[activeStage] - debt;
}

void Shutter::tickStrobe(uint32_t passedMicros) {
  microsStage -= passedMicros; // use that as control counter instead
  if(microsStage <= 0) {
    activeStage++;
    if(activeStage > CLOSED) activeStage = OPEN; // roll over, skipping pre and (non-stage) TOTAL
    initStage(-microsStage); // pass on any debt incurred by late update
  }
  amountOpen -= (eachFrameFade < amountOpen? eachFrameFade: amountOpen);
}


float RotateStrip::_apply(float value, float progress, Strip& s, bool force) {
  if(forward) s.rotateFwd(value); // these would def benefit from anti-aliasing = decoupling from leds as steps
  else s.rotateBack(value); // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
  return value;
}


float Bleed::_apply(float value, float progress, Strip& s, bool force) {
  if(!value) return value;

  if(s.fieldSize() == 3) return value; // XXX would crash below when RGB and no busW...

  for(int pixel = 0; pixel < s.fieldCount(); pixel++) {
    RgbwColor color[4]; // RgbwColor color, colorBehind, colorAhead, blend; //= color; // same concept just others rub off on it instead of other way...
    s.getPixelColor(pixel, color[THIS]);
    uint8_t thisBrightness = color[THIS].CalculateBrightness();
    uint8_t brightnessForWeight = (thisBrightness > 0? thisBrightness: 1);
    float weight[2]; // float prevWeight, nextWeight;

    if(pixel-1 >= 0) {
      s.getPixelColor(pixel-1, color[BEHIND]);
      weight[BEHIND] = color[BEHIND].CalculateBrightness() / (float)brightnessForWeight;
      if(weight[BEHIND] < 0.3) { // skip if dark, test. Should use weight and scale smoothly instead...
        color[BEHIND] = color[THIS];
      }
    } else {
      color[BEHIND] = color[THIS];
      weight[BEHIND] = 1;
    }

    if(pixel+1 < s.fieldCount()) {
      s.getPixelColor(pixel+1, color[AHEAD]);
      weight[AHEAD] = color[AHEAD].CalculateBrightness() / (float)brightnessForWeight;
      if(weight[AHEAD] < 0.3) { // do some more shit tho. Important thing is mixing colors, so maybe look at saturation as well as brightness dunno
        color[AHEAD] = color[THIS];  // since we have X and Y should weight towards more interesting.
      }
    } else {
      color[AHEAD] = color[THIS];
      weight[AHEAD] = 1;
    }
    float amount = value/2;  //this way only ever go half way = before starts decreasing again

    color[BLEND] = RgbwColor::LinearBlend(color[BEHIND], color[AHEAD], 0.5); //test
    // (1 / (prevWeight + weight[AHEAD] + 1)) * prevWeight); //got me again loll
    // color[THIS] = RgbwColor::BilinearBlend(color[THIS], color[AHEAD], color[BEHIND], color[BLEND], amount, amount);
    color[THIS] = RgbwColor::BilinearBlend(color[THIS], color[AHEAD], color[BEHIND], color[THIS], amount, amount);

    s.setPixelColor(pixel, color[THIS]); // XXX handle flip and that
  }
  return value;
}


// float HueRotate::_apply(float value, Strip& s) {
//   if(!value) return value; // rotate hue around, simple. global desat as well?
//   // should be applying value _while looping around pixels and already holding ColorObjects.
//   // convert Rgbw>Hsl? can do Hsb to Rgbw at least...
// }


float Noise::_apply(float value, float progress, Strip& s, bool force) {
  if(!value) return value;

  maxNoise = (1 + value) / 4 + maxNoise; // CH_NOISE at max gives +-48
  uint8_t* data = s.getBuffer();

  static uint8_t iteration = 0;
  for(int pixel = 0; pixel < s.fieldCount(); pixel++) {
    uint8_t subPixel[s.fieldSize()];
    for(uint8_t i=0; i < s.fieldSize(); i++) {
      if(iteration == 1) subPixelNoise[pixel][i] = random(maxNoise) - maxNoise/2;
      else if(iteration >= 100) iteration = 0; //arbitrary value
      iteration++;

      subPixel[i] = data[pixel * s.fieldSize() + i]; //pointer
      if(subPixel[i] > 16) { //cutoff. XXX fix so bound works
        // subPixel[i] = bound<int>(subPixel[i] + subPixelNoise[pixel][i], 0, 255);
        // int tot = subPixel[i] + subPixelNoise[pixel][i];
        // subPixel[i] = tot < 0? 0: (tot > 255? 255: tot);
      }
    }
  }
  return value;
}



void Functions::update(float progress, uint8_t* fun) { //void update(uint32_t elapsed, uint8_t* fun = nullptr) {
  if(fun == nullptr) fun = ch; //use backup

  for(uint8_t i=0; i < numChannels; i++) { // prep for when moving entirely onto float
    ch[i] = chOverride[i] < 0? fun[i]:
  ((uint8_t)chOverride[i] * blendOverride[i] + // Generally these would be set appropriately (or from settings) at boot and used if no ctrl values incoming
            ch[i] * (1.0f - blendOverride[i])); //but also allow (with prio/mode flag) override etc. and adjust behavior, why have anything at all hardcoded tbh
    val[i] = (float)ch[i] / 255;
  }

  // shutter.apply(ch[chStrobe], *s);
  shutter.force(val[chStrobe], progress, *s); //no work

  dimmer.setEnvelope(val[chDimmerAttack], val[chDimmerRelease]);
  dimmer.apply(ch[chDimmer], progress, *s);

  e.set(val[chAttack], val[chRelease]);

  b.apply(ch[chBleed], progress, *s);
  h.apply(ch[chHue], progress, *s);
  // rotFwd.apply(val[chRotateFwd], *s);
  rotFwd.force(val[chRotateFwd], progress, *s); //work
  rotBack.apply(ch[chRotateBack], progress, *s); //no work

  if(shutter.open) {
    outBrightness = dimmer.getByte();
  } else { //either closed, or closing over coming frames
    uint8_t decreaseBy = 255 - shutter.amountOpen;
    outBrightness = decreaseBy >= dimmer.getByte()? 0: dimmer.getByte() - decreaseBy;
  }
  s->setBrightness(outBrightness);
}

