#include "pixtol.h"

void setup() {
  initDevice(); bootLog(doneBOOT); //should have a first anim thing here...
  initHomie();  bootLog(doneHOMIE);
  initScheduler();
  initState();  bootLog(doneMAIN);

  b->blink("black", 1, false, 288); // clear strip
  b->test();
}

void renderFrame(float progress) {
  s->updateWithEnvelope(targetBuffer->get(), f->e, progress); //gotta fix so env responds to passed time...
  f->update(progress, targetFunctions->get());
  s->show();
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  if(universe != cfg->startUni.get()) return; //in case some fucker is broadcasting... XXX support multiple universes duh..
  wasBuffer->set(targetBuffer.get());

  targetFunctions->set(data, f->numChannels);
  uint16_t destLen = cfg->stripLedCount.get() * cfg->stripBytesPerPixel.get(); //rather, targetBuffer...
  uint16_t stripDataLen = length - f->numChannels;
  uint16_t readLen = destLen <= stripDataLen? destLen: stripDataLen; //just to be sure
  targetBuffer->set(data + f->numChannels, readLen); //no offset arg, breaks
  gotFrameAt = micros();

  logDMXStatus(universe, data, length);
}

void loop() {
  lastArtnetOpCode = artnet->read(); // skip callback stuff and just use .read once move to class? prob makes more sense
  float progress = (micros() - gotFrameAt) / (float)interval; //remember to adjust stuff to interpolation can overshoot...
  uint16_t keyFrameCount = (micros() - start) / interval; //remember to adjust stuff to interpolation can overshoot...
  if(!(keyFrameCount % cfg->dmxHz.get())) {
    LN.logf(_DEBUG_, "Keyframe %d, Progress %f", keyFrameCount, progress);
  }

  renderFrame(progress);
  // static uint32_t updates = 0, priorSeconds = 0;
  // uint32_t seconds;
  // if((seconds = (t / MILLION)) != priorSeconds) { // 1 sec elapsed?
  //   LN.logf(__func__, LoggerNode::DEBUG, "%d updates/sec", updates);
  //   priorSeconds = seconds;
  //   updates = 0; // Reset counter
  // }

  ArduinoOTA.handle(); // redundant once homie's ota stops being a buggy pos...
	Homie.loop(); // XXX DUH DUH DAH, fucking save all networks ever configured and try them lol finally an IoT thing for the 90's

  static uint16_t wasOnlineAt;
  static bool firstConnect = true;
  if(Homie.isConnected()) { // kill any existing go-into-wifi-finder timer, etc
    if(firstConnect) {
      bootLog(doneONLINE);
      bootInfoPerMqtt();
      firstConnect = false;
    }
    wasOnlineAt = millis();
  } else { // stays stuck in this state for a while on boot... set a timer if not already exists, yada yada blink statusled for sure...
    if(!firstConnect) { //already been connected, so likely temp hickup, chill
      if(millis() - wasOnlineAt < 10000) //give it 10s
        return;
      else {
        static bool justDisconnected = true;
        if(justDisconnected) {
          LN.logf(_DEBUG_, "Wifi appears well down");
          justDisconnected = false;
        }
      }

    } else {
      // fade up some sorta boot anim I guess?

    }
  }
}
