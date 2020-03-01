#include "pixtol.h"
#include "smooth-playground.h"

std::unique_ptr<Device>    device;
std::unique_ptr<Scheduler> scheduler;

extern "C" { // ESP-IDF goes app_main. unix mock main...
#ifdef ESP_PLATFORM
void app_main() {
    setup();
    // App app{}; app.start();
}
#else
int main(int /*argc*/, char** /*argv*/) {
    // App app{}; app.start();
    return 0;
}
#endif
}


void setup() {
  String deviceId = "pixTol";
  device    = std::make_unique<Device>(deviceId);
  scheduler = std::make_unique<Scheduler>(deviceId);

#ifdef TOL_DEBUG /* REMEMBER FORCE REMOTE FOR GDB!!! */
  gdbstub_init();
  lg.dbg("GDB enabled, UART0 strip wont work!");
#endif

  scheduler->start();
}

void loop() { // ok I mean at some point just silly moving more stuff away from here...
  // lwd.feed(); //if so device loop must go first in loop
  device->loop();
  // lwd.stamp(); //close watchdog loop - must start next with same
}
