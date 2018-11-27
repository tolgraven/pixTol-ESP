#include "debug.h"


bool Debug::sendIfChanged(const String& property, int value) {
  static std::map<String, int> propertyValues;
  if(propertyValues.find(property) != propertyValues.end()) {
    int last = propertyValues[property];
    if(value == last || (value > 0.90*last && value < 1.10*last)) //XXX settable tolerance...
      return false;
  }
  status->setProperty(property).send(String(value)); //new property or new value...
  propertyValues[property] = value;
  return true; //set and sent
}

void Debug::logFunctionChannels(uint8_t* dataStart, const String& id, uint8_t num) {
  if(dmxFrameCounter % (cfg->dmxHz.get()*10)) return;
  lg.logf(id, Log::DEBUG, "%u %u %u %u %u %d %u %u %u %u %u %d",
      dataStart[0], dataStart[1], dataStart[2], dataStart[3], dataStart[4], dataStart[5], dataStart[6],
      dataStart[7], dataStart[8], dataStart[9], dataStart[10], dataStart[11]);
}

void Debug::logDMXStatus(uint16_t universe, uint8_t* data) { // for logging and stuff... makes sense?
  if(!first) first = millis();
  dmxFrameCounter++;
  if(dmxFrameCounter % (cfg->dmxHz.get()*10)) return;  // every 10s (if input correct and stable)

  uint16_t totalTime = millis() - first;
  first = millis();
  // static const String keys[] = { "freeHeap", "heapFragmentation", "maxFreeBlockSize", "fps", "droppedFrames",
  //   "dimmer.base", "dimmer.force", "dimmer.out"};
  // static const std::function<int()> getters[] =
  // { ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize(),
  //   [=]() {dmxFrameCounter / (totalTime / 1000)},
  //   [s]() {return s->droppedFrames}, [f]() {return f->dimmer.getByte()}, [f]() {return f->chOverride[chDimmer]},
  // [f]() {return f->outBrightness}};
  // static map<String, string> table(make_map(keys, getters));

  sendIfChanged("freeHeap", ESP.getFreeHeap());
  sendIfChanged("heapFragmentation", ESP.getHeapFragmentation());
  sendIfChanged("maxFreeBlockSize", ESP.getMaxFreeBlockSize());
  sendIfChanged("fps", dmxFrameCounter / (totalTime / 1000));
  sendIfChanged("droppedFrames", s->droppedFrames);

  sendIfChanged("dimmer.base", f->chan[chDimmer]->getByte());
  sendIfChanged("dimmer.force", f->chOverride[chDimmer]); //whyyy this gets spammed when no change?
  sendIfChanged("dimmer.out", f->outBrightness);
  sendIfChanged("dimmer.strip", s->brightness);
  dmxFrameCounter = 0;
}

void Debug::logAndSave(const String& msg) { //possible approach...  log to serial + save message, then post by LN once MQTT up
  // cause now lose info if never connects...
}

int Debug::getBootDevice(void) {
  int bootmode;
  asm ("movi %0, 0x60000200\n\t"
      "l32i %0, %0, 0x118\n\t"
      : "+r" (bootmode) /* Output */
      : /* Inputs (none) */
      : "memory" /* Clobbered */);
  return ((bootmode >> 0x10) & 0x7);
}
void Debug::resetInfoLog() {
  switch (ESP.getResetInfoPtr()->reason) {
    // USUALLY OK ONES:
    case REASON_DEFAULT_RST:      lg.logf("Reset", Log::DEBUG, "Power on"); break; // normal power on
    case REASON_SOFT_RESTART:     lg.logf("Reset", Log::DEBUG, "Software/System restart"); break;
    case REASON_DEEP_SLEEP_AWAKE: lg.logf("Reset", Log::DEBUG, "Deep-Sleep Wake"); break;
    case REASON_EXT_SYS_RST:      lg.logf("Reset", Log::DEBUG, "External System (Reset Pin)"); break;
    // NOT GOOD AND VERY BAD:
    case REASON_WDT_RST:          lg.logf("Reset", Log::DEBUG, "Hardware Watchdog"); break;
    case REASON_SOFT_WDT_RST:     lg.logf("Reset", Log::DEBUG, "Software Watchdog"); break;
    case REASON_EXCEPTION_RST:    lg.logf("Reset", Log::DEBUG, "Exception"); break;
    default:                      lg.logf("Reset", Log::DEBUG, "Unknown"); break;
  }
}

void Debug::bootLog(BootStage bs) {
  Debug::ms[bs] = millis();
  Debug::heap[bs] = ESP.getFreeHeap();
  switch(bs) { // eh no need unless more stuff comes
    case doneBOOT: break;
    case doneHOMIE: break;
    case doneMAIN: break;
  }
}

// possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
// or keep dev one connected to another esp forwarding...
void Debug::bootInfoPerMqtt() {
  if(getBootDevice() == 1) lg.logf("SERIAL-FLASH-BUG", Log::WARNING, "%s\n%s", "First boot post serial flash. OTA won't work and first WDT will brick device until manually reset", "Best do that now, rather than later");
  resetInfoLog();
  lg.logf("Reset", Log::DEBUG, "%s", ESP.getResetInfo().c_str());
  lg.logf("Boot", Log::DEBUG, "Free heap post boot/homie/main/wifi: %d / %d / %d / %d",
                    heap[doneBOOT], heap[doneHOMIE], heap[doneMAIN], heap[doneONLINE]);
  lg.logf("Boot", Log::DEBUG, "ms for homie/main, elapsed til setup()/wifi: %d / %d,  %d / %d",
      Debug::ms[doneHOMIE] - Debug::ms[doneBOOT], Debug::ms[doneMAIN] - Debug::ms[doneHOMIE], Debug::ms[doneMAIN] - Debug::ms[doneBOOT], Debug::ms[doneONLINE] - Debug::ms[doneBOOT]);
}


// template<typename KeyType, typename ValueType, int N>
// class mapmaker {
//     std::pair<KeyType, ValueType>(&table)[N];
//     const KeyType(&keys)[N];
//     const ValueType(&vals)[N];
//     template<int pos> void fill_pair() {
//         table[pos].first = keys[pos];
//         table[pos].second = vals[pos];
//         fill_pair<pos-1>();
//     }
//     template<> void fill_pair<0>() {
//         table[0].first = keys[0];
//         table[0].second = vals[0];
//     }
// public:
//     mapmaker(std::pair<KeyType, ValueType>(&t)[N], const KeyType(&k)[N], const ValueType(&v)[N]):
//       table(t), keys(k), vals(v) {
//         fill_pair<N-1>();
//     }
// };
// template<typename KeyType, typename ValueType, int N>
// std::map<KeyType,ValueType> make_map(const KeyType(&keys)[N], const ValueType(&vals)[N]) {
//     std::pair<KeyType, ValueType> table[N];
//     mapmaker<KeyType, ValueType, N>(table, keys, vals);
//     return std::map<KeyType, ValueType>(table, table+N);
// }
