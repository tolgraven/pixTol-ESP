#include "pixtol.h"

Device* device;
Scheduler* scheduler;
IOT* iot;
Blinky* b;

void initDevice() { // init Serial, random seed, some first boot animation? anything else pre-homie/state?
  device = new Device();
  iot = new IOT("pixTol"); //also inits Homie. REMEMBER no cfg before that!!!

  scheduler = new Scheduler(Homie.getConfiguration().deviceId, *iot);
  lg.dbg("Done scheduler");
  b = new Blinky(scheduler->output(0));
  iot->finishSetup(scheduler->output(0), *b, *scheduler->renderer().f); //baddd design...
  lg.dbg("Done state");

  device->finalizeBoot();
}

void setup() {
  initDevice(); //Debug::bootLog(Debug::doneBOOT); //should have a first anim thing here...
  // Debug::bootLog((Debug::BootStage)2);

  b->blink("black", 1, false, 288); // clear strip
  b->test(); //why does this only work some of the time?
  delay(100);
  b->color("blue");
  delay(100);

  // yield(); //try gamma thing before everything else instead? might just have enough time to dump out over serial...
  // gammaPtr = new GammaCorrection(s->fieldCount());
  // lg.logf(__func__, Log::DEBUG, "Made gamma");
  b->color("red");
  lg.dbg("Done setup");
  delay(50);
}

void loop() { // as well as watchdog, hold off actual setup of all crashy-esque stuff until a few seconds into loop...
  lwd.feed();
  device->loop(); // also (not currently) opens watchdog loop
  scheduler->loop();
  iot->loop();
  lwd.stamp(); //close watchdog loop
}
