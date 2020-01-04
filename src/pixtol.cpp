#include "pixtol.h"

Device* device;
Scheduler* scheduler;
/* IOT* iot; */

void initDevice() { // init Serial, random seed, some first boot animation? anything else pre-homie/state?
  device = new Device(); //XXX refactor back Device and IOT together, Homie or w/e further away.
  // components held there off cfg, register with scheduler. Then Renderer does everything.
  // No one else touches outputs. No one sets Functions directly etc.
  // about time this shit stops sucking balls
  String deviceId = "pixTol"; //Homie.getConfiguration().deviceId
  /* iot = new IOT(deviceId, device); //inits Homie. REMEMBER no cfg before that!!! */
  scheduler = new Scheduler(deviceId, *device);
  device->finalizeBoot();
  device->debug->stackAvailableLog();
}

void setup() {
  initDevice(); //Debug::bootLog(Debug::doneBOOT); //should have a first anim thing here...
#ifdef TOL_DEBUG
  gdbstub_init();
  lg.dbg("GDB enabled, UART0 strip wont work!");
#endif
  // Debug::bootLog((Debug::BootStage)2);
  lg.dbg("Done setup");
  device->debug->stackAvailableLog();
}

void loopStart() {
  Serial.printf("Loop START\n");
  device->debug->stackAvailableLog();
}
void loopEnd() {
  Serial.printf("Loop END\n");
  device->debug->stackAvailableLog();
}

void loop() { // as well as watchdog, hold off actual setup of all crashy-esque stuff until a few seconds into loop...
  /* REMEMBER FORCE REMOTE FOR GDB!!! */
  /* loopStart(); */
  lwd.feed();
  device->loop(); // also (not currently) opens watchdog loop
  scheduler->loop();
  /* iot->loop(); */
  lwd.stamp(); //close watchdog loop

  /* Serial.printf("Heap: %d\n", ESP.getFreeHeap()); */
  /* while(1) { */
  /*   /1* Serial.printf("."); delay(5); *1/ */
  /*   /1* yield(); /2* esp_wdt_feed(); *2/ *1/ */
  /*   device->loop(); // also (not currently) opens watchdog loop */
  /*   scheduler->loop(); */
  /*   iot->loop(); */
  /*   ESP.wdtFeed(); */
  /* } */
  /* ESP.wdtFeed(); */
  /* loopEnd(); */
  /* loop(); */
  // so. All the freezing, wdt timing out, also weirdo lengthy bizarro stacktrace...
  // somehow Strip ISR shows up 17 times there despite no calls to run.
  // back up and try figure this out like an adult.
  // hence why decoupling process is so important gah. get rid of strip from iot and shit.
}
