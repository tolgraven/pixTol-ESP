#pragma once

#include <Homie.h>
#include <LoggerNode.h>

#define FW_BRAND "tolgrAVen"
#define FW_NAME "pixTol"
#define FW_VERSION "1.0.28"

#define LED_PIN             5  // D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define LED_STATUS_PIN      2
#define RX_PIN              3
#define TX_PIN              1
#define SERIAL_BAUD     74880 // same rate as bootloader...

// typedef HomieSetting<> opt;
typedef HomieSetting<bool> optBool;
typedef HomieSetting<long> optInt;
typedef HomieSetting<double> optFloat;
typedef HomieSetting<const char*> optString;

class ConfigNode: public HomieNode {
 public:
    ConfigNode();
    // seems homie (now?) has a 10 settings limit. scale down settings, or patch homie?
    optInt logArtnet;

    optInt dmxHz;

    // optInt sourceBytesPerPixel;
    // optInt sourceLedCount;
    optInt stripBytesPerPixel;
    optInt stripLedCount;

    // optInt universes;
    optInt startUni;
    // optInt startAddr;

    optBool setMirrored;
    optBool setFolded;
    // optBool setFlipped;

 protected:
    virtual bool handleInput(const String& property, const HomieRange& range, const String& value);
    virtual void setup() override;
    // virtual void loop(); // relevant?
};
