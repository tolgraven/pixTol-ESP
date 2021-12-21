#include "pixtol.h"
#include "smooth-playground.h"

namespace tol {

extern "C" { // ESP-IDF goes app_main. unix mock main...
#ifdef ESP_PLATFORM
void app_main() {
  App app{};
  app.start();
#ifdef TOL_DEBUG
  // gdbstub_init(); lg.dbg("GDB enabled, UART0 strip wont work!");
#endif
}
#else
int main(int /*argc*/, char** /*argv*/) {
  App app{};
  app.start(); return 0;
}
#endif
}

}
