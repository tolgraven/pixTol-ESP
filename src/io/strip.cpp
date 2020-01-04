#include "strip.h"

void Strip::setGradient(RgbwColor* from, RgbwColor* to) { // XXX do with N points/colors...
  if(fieldSize() == 3) {
    for(uint16_t pixel=0; pixel<_fieldCount; pixel++) {
      if(!from) getPixelColor(pixel, *to); // like, if either nullptr, instead blend with whatever current...
      if(!to)   getPixelColor(pixel, *from); // also use alpha once color lib a'go
      RgbColor color = RgbColor::LinearBlend(RgbColor(from->R, from->G, from->B), RgbColor(to->R, to->G, to->B), (float)pixel/_fieldCount);
      setPixelColor(pixel, color);
    }
  } else {
    for(uint16_t pixel=0; pixel<_fieldCount; pixel++) {
      if(!from) getPixelColor(pixel, *to); // like, if either nullptr, instead blend with whatever current...
      if(!to)   getPixelColor(pixel, *from); // also use alpha once color lib a'go
      RgbwColor color = RgbwColor::LinearBlend(*from, *to, (float)pixel/_fieldCount);
      setPixelColor(pixel, color);
    }
  }
  show();
}

void Strip::setPixelColor(uint16_t pixel, RgbwColor color) {
  pixel = getIndexOfField(pixel);
  if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
  if(_gammaCorrect) color = colorGamma->Correct(color);

  _driver->SetPixelColor(pixel, color);
  if(_mirror) _driver->SetPixelColor(ledsInStrip-1 - pixel, color);
}

void Strip::setPixelColor(uint16_t pixel, RgbColor color) {
  pixel = getIndexOfField(pixel);
  if(color.CalculateBrightness() < pixelBrightnessCutoff) color.Darken(pixelBrightnessCutoff);
  if(_gammaCorrect) color = colorGamma->Correct(color);

  _driver->SetPixelColor(pixel, color);
  if(_mirror) _driver->SetPixelColor(ledsInStrip-1 - pixel, color);
}

void Strip::adjustPixel(uint16_t pixel, String action, uint8_t value) {
  if(_fieldSize == 4) {
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
  for(auto pixel=0; pixel<sizeInBytes(); pixel+=fieldSize()) {
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

bool Strip::show() {
  if(!active()) return false;
  /* tho otoh could rerun twice if not ready and yada? */
  /* if(ready()) { //needs to actually calc how far from ready tho - stupid not to wait if it's a tiny amt */
    _driver->Show();
    return true;
  /* } */
  droppedFrames++;
  return false;
}

  // PRODUCT IDEA / something for this class to do once Buffer does all, hurr..
  // instead of waiting for higher res cheapo chips (or i guess APA might be cheap/good enough dunno
  // reckon all these leds are too shit for pure rgb...)
  //
  // stuff two strips in one enclosure
  // one super diffused / dimmed.
  // sacrifice some output strength but use second to error correct first
  // and generally for very low outputs.
  // end up with 10-bit RGB as input.
  // or 8-bit but auto better dithering.
  // same driver for both but not reliant on speed because both output simultaneously.
  // off different pins - different strategy depending on output lvl...
  // mid-low just output residual div by how much weaker than main
  // low-low temporal dithering but colors less shit if eg 40 instead of 6...
  //
  // can likely get away with using rgb's for backup strip as well so even cheaper.
  // also could it be possible to delay second strip data latch so PWMs complement? heh.
  // and what about also simply driving second at lower voltage?
  // + having it taped to back and bouncing back on diffusing enclosure backplate.
  // = dimmed, but possible to crank up by switching back to 5+

bool Strip::run() { //nuh getting shit working with this, guess cause have been applying fx (shift etc) to something which gets overwritten just before flush
  static uint8_t lastBrightness = 255;
  uint8_t brightness = getGain() * buffer().getGain() * 255L;
  /* if(brightness == 0 && lastBrightness == 0) */
  /*   return false; // skip flush when does nothing. but not like this, or like framecount would get a bit fucked either way since less time outputting, hmm. */
  lastBrightness = brightness;

  uint8_t* src = buffer().get();
  uint8_t* dest = _driver->Pixels();
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
    memcpy(dest, src, buffer().lengthBytes()); // tho ideally I mean they're just pointing to the same spot nahmean
                                               // and think that works for DMA, just not UART (alternates, tho could keep both handles in Buffer...)
  if(rotateBackPixels) _driver->RotateLeft(rotateBackPixels);
  if(rotateFwdPixels)  _driver->RotateRight(rotateFwdPixels); //might need to go after dirty??
  /* if(_mirror) {} //welp gotta handle this too... */
  _driver->Dirty();
  bool outcome = show();
  if(outcome) Outputter::run();
  return outcome;
}

void Strip::applyGain() {
  float gain = constrain(buffer().getGain() * getGain(), 0, 1.0);
  uint8_t brightness = gain * 255L;
  if(brightness <= totalBrightnessCutoff) brightness = 0; //until dithering...

  /* uint16_t scale = (((uint16_t)brightness << 8) - 1) / 255; */
  uint16_t scale = brightness;

  uint8_t* ptr = _driver->Pixels();
  if (brightness == 0) {
    memset(ptr, 0x00, buffer().lengthBytes());
    return;
  } else if(brightness == 255) {
    return;
  }

  uint8_t* ptrEnd = ptr + _driver->PixelsSize();
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
  ledsInStrip = _mirror? _fieldCount * 2: _fieldCount;

  if(!externalDriver) {
    if(_driver) delete _driver;
    if(_fieldSize == RGB)       _driver = new StripRGB(ledsInStrip);
    else if(_fieldSize == RGBW) _driver = new StripRGBW(ledsInStrip);
  }
  _driver->Begin();
  show(); //try see if this helps initial alloc?

}

uint16_t Strip::getIndexOfField(uint16_t position) {
  if(position >= _fieldCount) position = _fieldCount-1; //bounds...
  else if(position < 0) position = 0;

  if(_fold) { // pixelidx differs from pixel recieved
    if(position % 2) position /= 2; // since every other pixel is from opposite end
    else position = _fieldCount-1 - position/2;
  }
  if(_flip) position = _fieldCount-1 - position;
  return position;
}
