#include "pixtol.h"

void setup() {
  initDevice(); bootLog(doneBOOT); //should have a first anim thing here...
  initHomie();  bootLog(doneHOMIE);
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

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  if(universe != cfg->startUni.get()) return; //in case some fucker is broadcasting... XXX support multiple universes duh..
  wasBuffer->set(targetBuffer->get());

  targetFunctions->set(data, f->numChannels);
  uint16_t destLen = cfg->stripLedCount.get() * cfg->stripBytesPerPixel.get(); //rather, targetBuffer...
  uint16_t stripDataLen = length - f->numChannels;
  uint16_t readLen = destLen <= stripDataLen? destLen: stripDataLen; //just to be sure
  targetBuffer->set(data + f->numChannels, readLen); //no offset arg, breaks
  gotFrameAt = micros();

  logDMXStatus(universe, data, length);
}

void loop() {
  for(auto& source: inputter) {
    if(source->run() && source->newData) { //or rather w callbacks etc should be OR?
      gotFrameAt = micros();
      uint16_t keyFrameCount = (micros() - start) / keyFrameInterval; //remember to adjust stuff to interpolation can overshoot...

      // buffer["origin"]->set(buffer["target"]->get());
      buffer["origin"]->set(buffer["current"]->get()); //dont jump on earlier than expected frames

      // Buffer tf = Buffer(source->get(), f->numChannels);
      Buffer tf = Buffer("temp functions", 1, f->numChannels, source->get().get());
      // f->setTarget(tf.get());
      // targetFunctions->htp(tf); //also needs frames synced up, so doesnt prevent newer from taking over...
      // targetFunctions->avg(tf);
      targetFunctions->add(tf);
      f->setTarget(targetFunctions->get());

      PixelBuffer pb = PixelBuffer(s->fieldSize(), s->fieldCount(), source->get().get(f->numChannels));
      switch(f->ch[2]) {
        case 0: buffer["target"]->avg(pb); break;
        case 1: buffer["target"]->add(pb); break;
        case 2: buffer["target"]->sub(pb); break;
        case 3: buffer["target"]->set(pb); break;
        case 4: buffer["target"]->htp(pb); break;
        case 5: buffer["target"]->ltp(pb); break;
        case 6: buffer["target"]->lotp(pb); break; //heheh oh yeah it'll never leave zero w/ current LoTP, duh. So gotta be "lowest non-0 takes precedence"
      }

      if(!(keyFrameCount % (10 * iot->cfg->dmxHz.get()))) {
        LN.logf(_DEBUG_, "%u %u %u %u %u %d %u %u %u %u %u %d", tf.get()[0], tf.get()[1], tf.get()[2],
            tf.get()[3], tf.get()[4], tf.get()[5], tf.get()[6], tf.get()[7], tf.get()[8], tf.get()[9], tf.get()[10], tf.get()[11]);
        LN.logf(_DEBUG_, "%u %u %u %u %u %d %u %u %u %u %u %d", targetFunctions->get()[0], targetFunctions->get()[1], targetFunctions->get()[2],
            targetFunctions->get()[3], targetFunctions->get()[4], targetFunctions->get()[5], targetFunctions->get()[6], targetFunctions->get()[7], targetFunctions->get()[8], targetFunctions->get()[9], targetFunctions->get()[10], targetFunctions->get()[11]);
        LN.logf(_DEBUG_, "%u %u %u %u %u %d %u %u %u %u %u %d", f->ch[0], f->ch[1], f->ch[2],
            f->ch[3], f->ch[4], f->ch[5], f->ch[6], f->ch[7], f->ch[8], f->ch[9], f->ch[10], f->ch[11]);
        LN.logf(_DEBUG_, "%.4f fake progress, %.4f/%.4f", 0.5, f->e.get(AT, 0.5), f->chan[chDimmer]->e->get(AT, 0.5));
        // LN.logf(_DEBUG_, "Keyframe %d", keyFrameCount);
      }
      iot->debug->logDMXStatus(2, source->get().get());
    }
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

  // ArduinoOTA.handle(); // redundant once homie's ota stops being a buggy pos...
	Homie.loop(); // XXX DUH DUH DAH, fucking save all networks ever configured and try them lol finally an IoT thing for the 90's

  static uint16_t wasOnlineAt;
  static bool firstConnect = true;
  // if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
  //   if(firstConnect) {
  //     bootLog(doneONLINE);
  //     bootInfoPerMqtt();
  //     firstConnect = false;
  //   }
  //   wasOnlineAt = millis();
  // } else { // stays stuck in this state for a while on boot... set a timer if not already exists, yada yada blink statusled for sure...
  //   if(!firstConnect) { //already been connected, so likely temp hickup, chill
  //     if(millis() - wasOnlineAt < 10000) //give it 10s
  //       return;
  //     else {
  //       static bool justDisconnected = true;
  //       if(justDisconnected) {
  //         LN.logf(_DEBUG_, "Wifi appears well down");
  //         justDisconnected = false;
  //       }
  //     }
  //   } else { // fade up some sorta boot anim I guess?
  //   }
  // }
}
