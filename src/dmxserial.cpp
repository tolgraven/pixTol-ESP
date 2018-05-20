#include "dmxserial.h"

DmxSerial::DmxSerial(uint8_t pin) {


}

void DmxSerial::onKeyFrame(uint8_t* data, uint16_t length) {
  // memcpy...
  // any other adjustments
  // flush(); called here or we let brain know we're ready to output

}

bool DmxSerial::flush() {
  
  Serial.begin(56700); Serial.write(0); // slowy nil makes for break
  delayMicroseconds(220);

  Serial.begin(250000, SERIAL_8N2); Serial.write(0); // begin frame
  for (int i=0; i < frameLength; i++) {
    Serial.write(data[i]);
  }
}
