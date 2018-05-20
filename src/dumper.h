#ifndef PIXTOL_DUMPER
#define PIXTOL_DUMPER

#include "outputter.h"

class Dumper: public Outputter {
  public:
    Dumper();
    // enum Destination {
    //   SERIAL, MQTT or LOG like, 
    //   webpage whatever
    // };
    // void onKeyframe(uint8_t* data, uint16_t length);
    bool flush();

    // in this instance modulator can be simply a text formatter
    // log throttler
    // or run same mods as some actual lights and get full dump of
    // how this actually affects the data
    // 
    bool addModulator(Modulator mod);
    bool addDestination(void* callback);
  private:
    uint8_t* data;
    int frameLength;

    // Modulator[] effects;

};


#endif
