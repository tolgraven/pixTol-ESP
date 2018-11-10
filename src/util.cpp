#include "util.h"
#include "algorithm.h"

template<typename T>
T bound(T value, T min, T max) {
  return value < min? min: (value > max? max: value);
}

void homieYield() {
  yield();
  Homie.loop();
}

void homieDelay(unsigned long ms) {
  if(!ms) return;
  unsigned long start = millis();
  homieYield();
  while (millis() - start < ms) {
    homieYield();
  }
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
  return clamp( v, lo, hi, std::less<>() );
}
