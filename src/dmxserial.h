#pragma once

#include "outputter.h"

class DmxSerial: public Outputter {
  public:
    DmxSerial(uint8_t pin); // or rather serial object
    void onKeyFrame(uint8_t* data, uint16_t length);
    bool flush();


  private:
    // uint8_t data[512];
    uint8_t* data;
    int frameLength;
    int channelOffset; // and/or more fancy remappings


};


#endif
