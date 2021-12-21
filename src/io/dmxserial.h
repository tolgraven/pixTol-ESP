#pragma once

#include <Arduino.h>
#include "renderstage.h"

class DmxInput public Inputter {
  public:
    DmxInput(): Inputter("DMX In", 1, 512) {
      //what lib?
      // ArduinoDmx0.attachRXInterrupt(frame_received);  //set function called when all channels received
      // ArduinoDmx0.set_control_pin(2);    // control pin for max485
      // ArduinoDmx0.set_rx_address(1);      // set rx0 dmx start address
      // ArduinoDmx0.set_rx_channels(512);     // number of DMX channel to receive
      // ArduinoDmx0.init_rx(DMX512);        // starts universe 0 as rx, NEW Parameter DMX mode
      // Serial.begin();
    }

    bool ready() { }
    DmxInput(uint8_t serialPort); // Serial = 0, Serial1 = 1...


  private:
};

class DmxOutput public Outputter { //gheto bid bangen
  public:
    DmxOutput(uint8_t serialPort): // XXX use. Serial = 0, Serial1 = 1...
      Outputter("DMX Out", 1, 512) {
      // Serial.begin();
    }
    DmxOutput(uint8_t serialPort);

    // virtual bool ready(); // always check before trying to emit...

    virtual void run() {
      uint8_t* data = buffer().get();

      Serial.begin(56700); //tho use Serial1 I guess?
      Serial.write(0);
      delayMicroseconds(220); // slowy nil makes for break

      Serial.begin(250000, SERIAL_8N2);
      Serial.write(0); // begin frame
      for(int i=0; i < length; i++)
        Serial.write(data[i]);
    }

  private:
    // int baud;
  int channelOffset = 0; // hmm would be through pre-modulator tho...
};


