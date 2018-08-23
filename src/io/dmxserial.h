#pragma once

#include <Arduino.h>
#include "outputter.h"
#include "inputter.h"

class DmxSerial: public Outputter, public Inputter {
  public:
    DmxSerial(): Outputter("DMX Serial Out", 1, 512), Inputter("DMX Serial In", 1, 512) {
      ArduinoDmx0.attachRXInterrupt  (frame_received);  //set function called when all channels received
      ArduinoDmx0.set_control_pin (2);    // control pin for max485
      ArduinoDmx0.set_rx_address(1);      // set rx0 dmx start address
      ArduinoDmx0.set_rx_channels(512);     // number of DMX channel to receive
      ArduinoDmx0.init_rx(DMX512);        // starts universe 0 as rx, NEW Parameter DMX mode
      // Serial.begin();
    }
    DmxSerial(uint8_t serialPort); // Serial = 0, Serial1 = 1...

    virtual bool canEmit(); // always check before trying to emit...

    virtual void emit(uint8_t* data, uint16_t length) {
      Serial.begin(56700);
      Serial.write(0); // slowy nil makes for break
      delayMicroseconds(220);

      Serial.begin(250000, SERIAL_8N2);
      Serial.write(0); // begin frame
      for (int i=0; i < length; i++) {
        Serial.write(data[i]);
      }
      outBuffer = data;
      dataLength = length;
    }

    virtual void emit() {
      if(outBuffer && frameLength) {
        emit(outBuffer, frameLength); // if keeps the same
      }
    }

    virtual bool read() {

    }


  private:
    // int baud;
    uint8_t* data = nullptr;
    int channelOffset = 0; // and/or more fancy remappings

};

