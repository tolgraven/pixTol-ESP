#include "strip.h"

Strip::Strip(PixelBytes type, uint16_t ledCount):
    setMirrored("mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one"),
    setFolded("folded_alternating", "alternating pixels"),
    setFlipped("flip", "flip strip direction"),
    setGammaCorrect("gamma_correct",   "shitty gamma correction that sucks")
{
    bytes = type;
    ledsInData = ledCount;
    ledsInStrip = setMirrored.get()? 2*ledCount: ledCount;
    flipped = setFlipped.get(); // can also be changed in realtime, hence decoupled

    subPixelNoise[125][4] = {0};


    if(setGammaCorrect.get()) colorGamma = new NeoGamma<NeoGammaTableMethod>;

    bus = new DmaGRBW(ledsInStrip);
    bus->Begin();
}

int Strip::getPixelIndex(int pixel) { // XXX also handle matrix back-and-forth setups etc
    if(flipped) pixel = ledsInData-1 - pixel;
    int idx = pixel;
    if(setFolded.get()) { // pixelidx differs from pixel recieved
      if(pixel % 2) idx = pixel/2;  // since every other pixel is from opposite end
      else          idx = ledsInData-1 - pixel/2;
    }
    return idx;
    // XXX remember mirror must be handled in render loop
}

int Strip::getPixelFromIndex(int idx) {
    int pixel = idx;
    if(setFolded.get()) {
      if(idx % 2) pixel = idx*2;
      else        pixel = ledsInData-1 - idx*2;
    }
    if(flipped) pixel = ledsInData-1 - idx;
    return pixel;
    // XXX above aint fixed
}


void Strip::keyFrame(uint8_t* pixelData, uint8_t* functionData, bool updatePixels, bool updateFunctions) {

}

void Strip::updatePixels(uint8_t* data) { // XXX also pass fraction in case interpolating >2 frames
  // uint8_t* functions = &data[-1];
  // int data_size = bytes_per_pixel * cfg_count.get() + DMX_FN_CHS;
  //   int pixel = 0; //512/bytes_per_pixel * (universe - 1);  // for one-strip-multiple-universes
  // int pixelidx;
  // float brightnessFraction = 255 / (brightness ? brightness : 1);     // ahah can't believe divide by zero got me
  //
  // for(int t = DMX_FN_CHS; t < data_size; t += bytes_per_pixel) {
  //   pixelidx = getPixelIndex(pixel);
  //   
	// 	if(bytes_per_pixel == 3) {  // create wrapper possibly so dont need this repeat shit.
  //     // use ColorObject base class etc...
	// 		RgbColor color 	= RgbColor(data[t], data[t+1], data[t+2]);
  //     color = RgbColor::LinearBlend(buses[busidx].bus->GetPixelColor(pixelidx), color, attack);
  //     if(color.CalculateBrightness() < 12) color.Darken(12); // avoid bitcrunch
	// 		buses[busidx].bus->SetPixelColor(pixelidx, color); // TODO apply light dithering if resolution can sustain it, say brightness > 20%
	// 		if(mirror) buses[busidx].bus->SetPixelColor(led_count - pixel - 1, color);
  //
  //
	// 	} else if(bytes_per_pixel == 4) { 
  //     uint8_t maxNoise = 32; //baseline, 10 either direction
  //     if(functions[CH_NOISE]) {
  //       maxNoise = (1 + functions[CH_NOISE]) / 4 + maxNoise; // CH_NOISE at max gives +-48
  //     }
  //     uint8_t subPixel[4];
  //     for(uint8_t i=0; i < 4; i++) {
  //       if(iteration == 1) subPixelNoise[pixelidx][i] = random(maxNoise) - maxNoise/2;
  //       else if(iteration >= interFrames * 10) iteration = 0;
  //       iteration++;
  //
  //       subPixel[i] = data[t+i];
  //       if(subPixel[i] > 16) {
  //         int16_t tot = subPixel[i] + subPixelNoise[pixelidx][i];
  //         if(tot >= 0 && tot <= 255) subPixel[i] = tot;
  //         else if(tot < 0) subPixel[i] = 0;
  //         else subPixel[i] = 255;
	// 				// subPixel[i] = (tot < 0? 0: (tot > 255? 255: tot));
  //       }
  //     }
	// 		RgbwColor color = RgbwColor(subPixel[0], subPixel[1], subPixel[2], subPixel[3]);
  //     RgbwColor lastColor = busW->GetPixelColor(pixelidx);  // get from pixelbus since we can resolve dimmer-related changes
  //     bool brighter = color.CalculateBrightness() > (brightnessFraction * lastColor.CalculateBrightness()); // handle any offset from lowering dimmer
  //     // XXX need to actually restore lastColor to its original brightness before mixing tho...
  //     color = RgbwColor::LinearBlend(color, lastColor, (brighter ? attack : rls));
	// 		if(color.CalculateBrightness() < 6) { // avoid bitcrunch
  //       color.Darken(6); // XXX should rather flag pixel for temporal dithering yeah?
  //     }
  //
  //     if(cfg_gamma_correct.get()) color = colorGamma->Correct(color); // test. better response but fucks resolution a fair bit. Also wrecks saturation? and general output. Fuck this shit
  //
  //     // all below prob go in own func so dont have to think about them and can reuse in next loop...
  //     if(!flipped) busW->SetPixelColor(pixelidx, color);
  //     else         busW->SetPixelColor(led_count-1 - pixelidx, color);
	// 		buses[busidx].busW2->SetPixelColor(led_count-1 - pixelidx, color); //test, mirror so can see whether works...
  //
	// 		if(mirror) busW->SetPixelColor(led_count-1 - pixelidx, color);   // XXX offset colors against eachother here! using HSL even, (one more saturated, one lighter).  Should bring a tiny gradient and look nice (compare when folded strip not mirrored and loops back with glorious color combination results)
	// 	} pixel++;
	// }
  //
  // if(functions[CH_BLEED]) { // loop through again...
  //   for(pixel = 0; pixel < led_count; pixel++) {
	// 		RgbwColor color = busW->GetPixelColor(getPixelIndex(pixel));
	// 		uint8_t thisBrightness = color.CalculateBrightness();
  //     RgbwColor prevPixelColor = color; // same concept just others rub off on it instead of other way...
  //     RgbwColor nextPixelColor = color;
	// 		float weightPrev;
	// 		float weightNext;
  //     if(pixel-1 >= 0) {
	// 			prevPixelColor = busW->GetPixelColor(getPixelIndex(pixel-1));
	// 			weightPrev = prevPixelColor.CalculateBrightness() / thisBrightness+1;
	// 		}
  //     if(pixel+1 < led_count) {
	// 			nextPixelColor = busW->GetPixelColor(getPixelIndex(pixel+1));
	// 			weightNext = nextPixelColor.CalculateBrightness() / thisBrightness+1;
	// 			// if(nextPixelColor.CalculateBrightness() < thisBrightness * 0.7) // skip if dark, test
	// 			// 	nextPixelColor = color;
	// 				// do some more shit tho. Important thing is mixing colors, so maybe look at saturation as well as brightness dunno
	// 				// since we have X and Y should weight towards more interesting.
	// 		}
  //     float amount = (float)functions[CH_BLEED]/(256*2);  //this way only ever go half way = before starts decreasing again
  //     color = RgbwColor::BilinearBlend(color, nextPixelColor, prevPixelColor, color, amount, amount);
  //
  //     busW->SetPixelColor(getPixelIndex(pixel), color); // XXX handle flip and that
  //
  //   }
  // }
	// if(functions[CH_HUE]) { // rotate hue around, simple. global desat as well?
  //
	// }
}

void Strip::updateFunctions(uint8_t* functions, bool isKeyframe) {
  //
  // if(functions[CH_STROBE]) {
  //   if(functions[CH_STROBE] != ch_strobe_last_value) { // reset timer for new state
  //     float hz = ((hzMax - hzMin) * (functions[CH_STROBE] - 1) / (255 - 1)) + hzMin;
  //     strobePeriod = 1000 / hz;   // 1 = 1 hz, 255 = 10 hz, to test
  //     onTime = strobePeriod / onFraction; // arbitrary default val. Use as midway point to for period control >127 goes up, < down
  //     // XXX instead of timers, use counter and strobe on frames? even 10hz would just be 1 on 3 off, easy...  more precise then use inter frames, yeah?
  //     strobeTickerClosed = interFrames * strobePeriod / cfg_dmx_hz.get(); // take heed of interframes...
  //     strobeTickerOpen = interFrames * onTime / cfg_dmx_hz.get();
  //     shutterOpen = false;
  //   } else { // decr tickers
  //     if(shutterOpen) strobeTickerOpen--;
  //     else            strobeTickerClosed--;
  //     if(!strobeTickerClosed) {
  //       shutterOpen = true;
  //       strobeTickerClosed = strobePeriod / cfg_dmx_hz.get();
  //     } else if(!strobeTickerOpen) {
  //       shutterOpen = false;
  //       strobeTickerOpen = onTime / cfg_dmx_hz.get();
  //     }
  //   }
  // } else { // 0, clean up
  //   shutterOpen = true;
  // }
  // ch_strobe_last_value = functions[CH_STROBE];
  //
  // if(functions[CH_ROTATE_FWD]) { // if(functions[CH_ROTATE_FWD] && isKeyframe) {
	// 	if(buses[busidx].bus)        buses[busidx].bus->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
	// 	else if(buses[busidx].busW)  buses[busidx].busW->RotateRight(led_count * ((float)functions[CH_ROTATE_FWD] / 255));
  // }
  // if(functions[CH_ROTATE_BACK]) { // very cool kaozzzzz strobish when rotating strip folded, 120 long but defed 60 in afterglow. explore!!
	// 	if(buses[busidx].bus)        buses[busidx].bus->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
	// 	else if(buses[busidx].busW)  buses[busidx].busW->RotateLeft(led_count * ((float)functions[CH_ROTATE_BACK] / 255));
  // }
  //
  // switch(functions[CH_CONTROL]) {
  //   case FN_FRAMEGRAB_1 ... FN_FRAMEGRAB_4: break;
  //   case FN_FLIP: flipped = !flipped; break;
  // }
  //
  // //WTF: when not enough current and strips yellow, upping atttack makes them less yellow. Also happens in general, but less noticable?
  // if(functions[CH_ATTACK] != last_functions[CH_ATTACK]) {
  //   attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
  //   if(log_artnet >= 2) Homie.getLogger() << "Attack: " << functions[CH_ATTACK] << " / " << attack << endl;
  // }
  // if(functions[CH_RELEASE] != last_functions[CH_RELEASE])
  //   rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_RELEASE]/285);
  //
  // if(functions[CH_DIMMER_ATTACK] != last_functions[CH_DIMMER_ATTACK])
  //   dimmer_attack = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_ATTACK]/285); // dont ever quite arrive like this, two runs at max = 75% not 100%...
  // if(functions[CH_DIMMER_RELEASE] != last_functions[CH_DIMMER_RELEASE])
  //   dimmer_rls = blendBaseline + (1.00f - blendBaseline) * ((float)functions[CH_DIMMER_RELEASE]/285);
  //
  // bool brighter = brightness > last_brightness;
	// brightness = last_brightness + (functions[CH_DIMMER] - last_brightness) * (brighter ? dimmer_attack : dimmer_rls);
	// if(brightness < 4) brightness = 0; // shit resulution at lowest vals sucks. where set cutoff?
  // else if(brightness > 255) brightness = 255;
	// if(shutterOpen) {
	// 	if(buses[busidx].bus) buses[busidx].bus->SetBrightness(brightness);
	// 	else if(buses[busidx].busW) {
  //     buses[busidx].busW->SetBrightness(brightness);
  //     buses[busidx].busW2->SetBrightness(brightness);
  //   }
	// } else buses[busidx].busW->SetBrightness(0);
  //
  // last_brightness = brightness;
}

void Strip::renderInterFrame() {
  // updatePixels(last_data);
  // updateFunctions(last_functions, false); // XXX fix so interpolates!!
  // busW->Show();
}

void Strip::interCallback() {
  // if(interCounter > 1) { // XXX figure out on its own whether will overflow, so can just push towards max always
  //   timer_inter.once_ms((1000/interFrames / cfg_dmx_hz.get()), interCallback);
  // }
  // renderInterFrame(); // should def try proper temporal dithering here...
  // interCounter--;
}

// void Strip::setup() {
//     
// }
// void Strip::loop() {
//     
// }

