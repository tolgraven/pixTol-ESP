#include "debuglog.h"


namespace tol {

bool Debug::sendIfChanged(const std::string& property, int value) {
  static std::map<std::string, int> propertyValues;
  if(propertyValues.find(property) != propertyValues.end()) {
    int last = propertyValues[property];
    if(value == last || (value > 0.90*last && value < 1.10*last)) //XXX settable tolerance...
      return false;
  }
  lg.f(property, tol::Log::INFO, " %i \n", value);
  /* lg.flushProperty(property, std::string(value)); */
  propertyValues[property] = value;
  return true; //set and sent
}

void Debug::logFunctionChannels(uint8_t* dataStart, const std::string& id, uint8_t expectedHz, uint8_t num) {
  /* if(dmxFrameCounter % (expectedHz * flushEverySeconds)) return; */

  /* Serial.println(""); */
  /* for(uint16_t b = 0; b < num; b++) { */
  /*   Serial.printf("%u ", data[b]); */
  /* } Serial.println(""); */
  lg.f(id, tol::Log::DEBUG, "%u %u %u %u %u %d %u %u %u %u %u %d\n",
      dataStart[0], dataStart[1], dataStart[2], dataStart[3], dataStart[4], dataStart[5], dataStart[6],
      dataStart[7], dataStart[8], dataStart[9], dataStart[10], dataStart[11]);
}

void Debug::registerToLogEvery(Buffer& buffer, uint16_t seconds) {
  if(buffers.find(&buffer) != buffers.end()) buffers[&buffer] = seconds;

}

void Debug::run() {
  for(auto buf: buffers) {
    if(!((startTime / 1000000) % buf.second)) {
      Serial.println((buf.first->id() + ":").c_str());
      for(uint16_t b = 0; b < buf.first->lengthBytes(); b++) {
        Serial.printf("%u ", buf.first->get()[b]);
      } Serial.println("");
    }
  }
}

void Debug::logDMXStatus(uint8_t* data, const std::string& id) { // for logging and stuff... makes sense?
  dmxFrameCounter++;
  uint32_t now = millis(), passed = now - lastFlush;
  if(passed / 1000 < flushEverySeconds) return;
  if(!startTime) startTime = now;

  /* if(dmxFrameCounter % (cfg->dmxHz.get()*10)) return;  // every 10s (if input correct and stable) */
  uint32_t totalTime = now - startTime;
  lg.f("Shit", tol::Log::DEBUG, "fps: %u, now: %u, lastFlush: %u, heap: %u, stack: %u \n",
      dmxFrameCounter / (passed / 1000), now, lastFlush, ESP.getFreeHeap(), stackUsed());
  lastFlush = now;
  logFunctionChannels(data, id);

  // Serial.println(id + ":");
  for(uint16_t b = 12; b < 64; b++) {
    Serial.printf("%u ", data[b]);
  } Serial.println("");

  // static const std::string keys[] = { "freeHeap", "heapFragmentation", "maxFreeBlockSize", "fps", "droppedFrames",
  //   "dimmer.base", "dimmer.force", "dimmer.out"};
  // static const std::function<int()> getters[] =
  // { ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize(),
  //   [=]() {dmxFrameCounter / (totalTime / 1000)},
  //   [s]() {return s->droppedFrames}, [f]() {return f->dimmer.getByte()}, [f]() {return f->chOverride[chDimmer]},
  // [f]() {return f->outBrightness}};
  // static map<std::string, string> table(make_map(keys, getters));

  /* sendIfChanged("freeHeap", ESP.getFreeHeap()); */
  /* sendIfChanged("heapFragmentation", ESP.getHeapFragmentation()); */
  /* sendIfChanged("maxFreeBlockSize", ESP.getMaxFreeBlockSize()); */
  /* sendIfChanged("fps", dmxFrameCounter / (totalTime / 1000)); */

  /* sendIfChanged("droppedFrames", s->droppedFrames); */

  /* sendIfChanged("dimmer.base", f->chan[chDimmer]->getByte()); */
  /* sendIfChanged("dimmer.force", f->chOverride[chDimmer]); //whyyy this gets spammed when no change? */
  /* sendIfChanged("dimmer.out", f->outBrightness); */
  /* sendIfChanged("dimmer.strip", s->brightness); */
  dmxFrameCounter = 0; //illegal instruction this lol what
}

void Debug::logAndSave(const std::string& msg) { //possible approach...  log to serial + save message, then post by LN once MQTT up
  // cause now lose info if never connects...
}


void Debug::bootLog(BootStage bs) {
  Debug::ms[bs] = millis();
  Debug::heap[bs] = ESP.getFreeHeap();
  switch(bs) { // eh no need unless more stuff comes
    case doneBOOT: break;
    case doneHOMIE: break;
    case doneMAIN: break;
    case doneONLINE: break;
  }
}

// possible to get entire stacktrace in memory? if so mqtt -> decoder -> auto proper trace?
// or keep dev one connected to another esp forwarding...
void Debug::bootInfoPerMqtt() {
  if(!lwd.softResettable())
    lg.f("SERIAL-FLASH-BUG", tol::Log::WARNING, "%s\n%s", "First boot post serial flash. OTA won't work and first WDT will brick device until manually reset", "Best do that now, rather than later");

  lg.f("Reset", tol::Log::DEBUG, "%s\n", lwd.getResetReason().c_str());
  // lg.f("Reset", tol::Log::DEBUG, "%s\n", ESP.getResetInfo().c_str());
  lg.f("Boot", tol::Log::DEBUG, "Free heap post boot/homie/main/wifi: %d / %d / %d / %d\n",
                    heap[doneBOOT], heap[doneHOMIE], heap[doneMAIN], heap[doneONLINE]);
  lg.f("Boot", tol::Log::DEBUG, "ms for homie/main, elapsed til setup()/wifi: %d / %d,  %d / %d\n",
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

}
