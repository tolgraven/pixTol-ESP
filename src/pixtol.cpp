#include "pixtol.h"

void setup() {
  initDevice(); //Debug::bootLog(Debug::doneBOOT); //should have a first anim thing here...
  initIOT();  //Debug::bootLog((Debug::BootStage)1);
  initScheduler();
  initState();  bootLog(doneMAIN);
  initUpdaters();

  // b->blink("black", 1, false, 288); // clear strip
  // b->test();
}

void renderFrame(float progress) {
  if(progress > 1.0) return; //for now...
  // outBuffer->interpolate(wasBuffer, buffer["target"], progress, f->e); //i guess
  // s->updateWithEnvelope(buffer["target"]->get(), f->e, progress); //gotta fix so env responds to passed time...
  buffer["current"]->blendUsingEnvelope(*buffer["origin"], *buffer["target"], f->e, progress);
  f->update(progress);
  // s->get(), set(buffer["current"]);
  s->setBuffer(*buffer["current"]);
  s->run();
  // LN.logf(_DEBUG_, "%.4f %.4f/%.4f", progress, f->e.get(AT, progress), f->chan[chDimmer]->e->get(AT, progress));
  // f->update(progress, targetFunctions->get());
  // s->show();
}

void renderFrame() {
  float progress = (micros() - gotFrameAt) / (float)keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...
  if(progress > 1.0) progress = (int)progress + 1.0f - progress; //make go back and forth instead! so 2.5 = 0.5...
  //should do further ultra slomo sorta, explore mini interpo more...

  buffer["current"]->blendUsingEnvelope(*buffer["origin"], *buffer["target"], f->e, progress);
  // buffer["current"]->interpolate(buffer["origin"], buffer["target"], progress, gammaPtr);

  // s->setBuffer(*buffer["current"]); //test setting per-pixel in buffer for now
  for(auto pixel=0; pixel < s->fieldCount(); pixel++) { //guess this needed for rotate, brightness etc to work. why tho??
    uint8_t* data = buffer["current"]->get(pixel * s->fieldSize());
    RgbwColor color = RgbwColor(data[0], data[1], data[2], data[3]);
    s->setPixelColor(pixel, color);
  }
  f->update(progress);
  s->show(); // s->run();
}

void setTarget(PixelBuffer& currentTarget, PixelBuffer& mergeIn, uint8_t value) {
  switch(value) { //control via mqtt as first step tho
    case 0: currentTarget.avg(mergeIn, true); break; //this needs split sources to really work, or off-pixels never get set
    case 1: currentTarget.avg(mergeIn, false); break;
    case 2: currentTarget.add(mergeIn); break;
    case 3: currentTarget.sub(mergeIn); break;
    case 4: currentTarget.htp(mergeIn); break;
    case 5: currentTarget.lotp(mergeIn, true); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
    case 6: currentTarget.lotp(mergeIn, false); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
    case 7: currentTarget.htp(mergeIn); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
  }
}

void handleDMX() {
  for(auto& source: inputter) {
    if(source->run() && source->newData) { //or rather w callbacks etc should be OR?
      // if(universe != iot->cfg->startUni.get()) return; //in case some fucker is broadcasting... tho should be handled at inputter lvel
      gotFrameAt = micros();

      buffer["origin"]->set(buffer["current"]->get()); //["target"]->get()); //dont jump on earlier than expected frames
      // sourceData[source->id()]->set(source->get(), source->get().get(f->numChannels), gotFrameAt);

      Buffer tf = Buffer(source->get(), f->numChannels);
      targetFunctions->avg(tf, true);
      f->setTarget(targetFunctions->get());

      PixelBuffer pb = PixelBuffer(s->fieldSize(), s->fieldCount(), source->get().get(f->numChannels));
      setTarget(*buffer["target"], pb, iot->blendMode);

      iot->debug->logDMXStatus(2, source->get().get());
      iot->debug->logFunctionChannels(tf.get(), "RAW"); // iot->debug->logFunctionChannels(targetFunctions->get(), "MIX");
      iot->debug->logFunctionChannels(f->ch, "FUN");
    }
  }
  // mergeDMX();
}

  float progress = (micros() - gotFrameAt) / (float)keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...
  renderFrame(progress);
  // static uint32_t updates = 0, priorSeconds = 0;
  // uint32_t seconds;
  // if((seconds = (t / MILLION)) != priorSeconds) { // 1 sec elapsed?
  //   LN.logf(__func__, LoggerNode::DEBUG, "%d updates/sec", updates);
  //   priorSeconds = seconds;
  //   updates = 0; // Reset counter
  // }

  iot->loop();
}
