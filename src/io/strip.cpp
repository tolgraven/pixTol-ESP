#include "strip.h"

namespace tol {

void Strip::setGradient(RgbwColor* from, RgbwColor* to) { // XXX do with N points/colors...
  if(fieldSize() == 3) {
    for(uint16_t pixel=0; pixel<fieldCount(); pixel++) {
      if(!from) getPixelColor(pixel, *to); // like, if either nullptr, instead blend with whatever current...
      if(!to)   getPixelColor(pixel, *from); // also use alpha once color lib a'go
      RgbColor color = RgbColor::LinearBlend(RgbColor(from->R, from->G, from->B), RgbColor(to->R, to->G, to->B), (float)pixel/fieldCount());
      setPixelColor(pixel, color);
    }
  } else {
    for(uint16_t pixel=0; pixel<fieldCount(); pixel++) {
      if(!from) getPixelColor(pixel, *to); // like, if either nullptr, instead blend with whatever current...
      if(!to)   getPixelColor(pixel, *from); // also use alpha once color lib a'go
      RgbwColor color = RgbwColor::LinearBlend(*from, *to, (float)pixel/fieldCount());
      setPixelColor(pixel, color);
    }
  }
}

void Strip::setPixelColor(uint16_t pixel, RgbwColor color) {
  pixel = getIndexOfField(pixel);
  if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
  if(_gammaCorrect) color = colorGamma->Correct(color);

  _driver[0]->SetPixelColor(pixel, color);
  if(_mirror) _driver[0]->SetPixelColor(ledsInStrip-1 - pixel, color);
}

void Strip::setPixelColor(uint16_t pixel, RgbColor color) {
  pixel = getIndexOfField(pixel);
  if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
  if(_gammaCorrect) color = colorGamma->Correct(color);

  _driver[0]->SetPixelColor(pixel, color);
  if(_mirror) _driver[0]->SetPixelColor(ledsInStrip-1 - pixel, color);
}

void Strip::adjustPixel(uint16_t pixel, String action, uint8_t value) {
  if(fieldSize() == 4) {
    RgbwColor color;
    getPixelColor(pixel, color);
    if(action == "lighten") {
      color.Lighten(value);
    } else if(action == "darken") {
      color.Darken(value);
    }
    setPixelColor(pixel, color);
  }
}

void Strip::rotateHue(float amount) {
  for(uint16_t pixel=0; pixel<sizeInBytes(); pixel+=fieldSize()) {
    RgbwColor rgbw;
    getPixelColor(pixel, rgbw);
    uint8_t   w = rgbw.W;
    HslColor hsl = HslColor(RgbColor(rgbw.R, rgbw.G, rgbw.B));
    float tot = hsl.H + amount;
    tot -= (int)tot;
    hsl.H = tot > 0? tot: 1.0 - tot;
    rgbw = RgbwColor(RgbColor(hsl));
    rgbw.W = w;
    setPixelColor(pixel, rgbw);
  }
}


bool Strip::_run() { //nuh getting shit working with this, guess cause have been applying fx (shift etc) to something which gets overwritten just before flush
  // static uint8_t lastBrightness = 255;
  // uint8_t brightness = getGain() * buffer().getGain() * 255L;
  // /* if(brightness == 0 && lastBrightness == 0) */
  // /*   return false; // skip flush when does nothing. but not like this, or like framecount would get a bit fucked either way since less time outputting, hmm. */
  // lastBrightness = brightness;
  // for(auto d: _driver) {
  // if(!active()) return false;
  bool outcome = false; // eh for now
  // for(int i=0; i < min((uint8_t)1, buffers().size()); i++) {
  // for(size_t i=0; i < std::min((size_t)1, buffers().size()); i++) {
  for(size_t i=0; i < buffers().size(); i++) {

    uint8_t* src = buffer(i).get();
    uint8_t* dest = _driver[i]->Pixels();

    /* if(swapRedGreen) { // OTOH this is surely better done at intermediate stage - per keyframe, not interpolating... */
    /*   // also so can keep using existing functionality if needed. obviously swapping at (pc side...) source even more sensible. */
    /*   for(auto p=0; p < fieldCount(); p++) { */
    /*     *dest++ = *(src + 1); // G */
    /*     *dest++ = *src; // R */
    /*     src+=2; */
    /*     *dest++ = *src++; */
    /*     if(fieldSize() == 4) *dest++ = *src++; */
    /*   } */
    if(src != dest) // may or may not be writing directly to pixelbuffer
      memcpy(dest, src, buffer(i).lengthBytes()); // tho ideally I mean they're just pointing to the same spot nahmean
                                                // and think that works for DMA, just not UART (alternates, tho could keep both handles in Buffer...)
    // if(rotateBackPixels) _driver[0]->RotateLeft(rotateBackPixels);
    // if(rotateFwdPixels)  _driver[0]->RotateRight(rotateFwdPixels); //might need to go after dirty??
    /* if(_mirror) {} //welp gotta handle this too... */
    _driver[i]->Dirty();
      // if(ready()) { //needs to actually calc how far from ready tho - stupid not to wait if it's a tiny amt
        // outcome = _driver[i]Show();
    if(_driver[i]->CanShow()) {
        // outcome & _driver[i]->Show(); //more reasonable/important to keep going w next right
        // outcome &= true;
        outcome |= true; // makes more sense, or well per-driver probably makes most sense....
        _driver[i]->Show(); //more reasonable/important to keep going w next right


      // } else {
      //   droppedFrames++; //tho like, guess should try even tempo strictly every other hehe
      // }
    // }
      // return true;
      // return false;
    }
  }
  // if(outcome) Outputter::run();
  return outcome;
}

void Strip::applyGain() { // XXX port offsets
  float gain = constrain(buffer().getGain() * this->getGain(), 0, 1.0);
  uint8_t brightness = gain * 255L;
  if(brightness <= totalBrightnessCutoff) brightness = 0; //until dithering...

  /* uint16_t scale = (((uint16_t)brightness << 8) - 1) / 255; */
  uint16_t scale = brightness;

  uint8_t* ptr = _driver[0]->Pixels();
  if (brightness == 0) {
    memset(ptr, 0x00, buffer(0).lengthBytes());
    return;
  } else if(brightness == 255) {
    return;
  }

  uint8_t* ptrEnd = ptr + _driver[0]->PixelsSize();
  while (ptr != ptrEnd) {
    uint16_t value = *ptr;
    *ptr++ = (value * scale) >> 8;
  }
}

void Strip::mirror(bool state) {
  _mirror = state;
  initDriver(); // affects output size so yeah
}

void Strip::gammaCorrect(bool state) {
  _gammaCorrect = state;
  if(state) colorGamma = new NeoGamma<NeoGammaTableMethod>;
  else delete colorGamma;
}

void Strip::initDriver() {
  ledsInStrip = _mirror? fieldCount() * 2: fieldCount();

  // if(!externalDriver) {
  //   if(_driver) delete _driver;
  //   if(fieldSize() == RGB)       _driver = new StripRGB(ledsInStrip);
  //   // else if(fieldSize() == RGBW) _driver = new StripRGBW(ledsInStrip);
  //   else if(fieldSize() == RGBW) _driver = new StripRGBW(ledsInStrip, port);
  // }
  // _driver.clear();
  // for(int port=0; port < min((uint8_t)1, bufferCount()); port++) {
  DEBUG("Init drivers");
  for(size_t port=0; port < buffers().size(); port++) {
    // DEBUG("Init driver: ", port, ", wazaa");
    DEBUG((String)"Init driver: " + port + ", wazaa");
    printTo(lg);
    if(!externalDriver) {
      // if(_driver[port]) delete _driver[port];
      // if(fieldSize() == RGB)       _driver[port] = new StripRGB(ledsInStrip);
      // else if(fieldSize() == RGBW) _driver[port] = new StripRGBW(ledsInStrip);
      // if(fieldSize() == RGBW) _driver[port] = new StripRGBW(ledsInStrip, port);
      // if(fieldSize() == RGB)       _driver.emplace_back(new StripRGB(ledsInStrip, port));
      if(fieldSize() == RGB)       {
        // _driver.emplace_back(new StripRGB(ledsInStrip));
        break; // dont hav multiple sorted yet so.
        // also why am i doing shit this way anyways? dont use/need the functions anymore so...
        // use Field / Color for RGB/RGBW agnostic code/on the fly conversion etc
        // just have light wrappers for strip.
      }
      else if(fieldSize() == RGBW) {
        _driver.emplace_back(new StripRGBW(ledsInStrip, port));
      }
    }
    _driver[port]->Begin();
  }
}

uint16_t Strip::getIndexOfField(uint16_t position) {
  if(position >= fieldCount()) position = fieldCount()-1; //bounds...
  else if(position < 0) position = 0;

  if(_fold) { // pixelidx differs from pixel recieved
    if(position % 2) position /= 2; // since every other pixel is from opposite end
    else position = fieldCount()-1 - position/2;
  }
  if(_flip) position = fieldCount()-1 - position;
  return position;
}

}
