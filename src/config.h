#pragma once

#include <Homie.h>
#include <LoggerNode.h>

#define FW_BRAND "tolgrAVen"
#define FW_NAME "pixTol"
#define FW_VERSION "1.0.46"

#define LED_PIN             5  // D1=5, D2=4, D3=0, D4=2  D0=16, D55=14, D6=12, D7=13, D8=15
#define LED_STATUS_PIN      2
#define RX_PIN              3
#define TX_PIN              1
// #define SERIAL_BAUD     74880 // same rate as bootloader...
#define MILLION       1000000
// #define DEFAULT_HZ         40


// FIX somehow: dont drop into config mode off invalid settings, or at least reads rest of config and
// preselect that + default for wrong one. Also event blink angry etc

using optBool   = HomieSetting<bool>;
using optInt    = HomieSetting<long>;
using optFloat  = HomieSetting<double>;
using optString = HomieSetting<const char*>;
// typedef HomieSetting<bool> optBool;
// typedef HomieSetting<long> optInt;
// typedef HomieSetting<double> optFloat;
// typedef HomieSetting<const char*> optString;
//this prob makes no sense. Put debug/log in log node, dmx in that node, strip-related in strip node...
//could use class for the config management thing tho
// HomieSetting<const char*> ConfigNode::mode("mode", "what mode iz running?");
// more likely got InputNode, OutputNode, can toggle bunch of stuff on/off independently
// tho that'd need proper routing
// so manual = dmx input off, mqtt/http input on

// XXX sort own spiffs-writer/reader (or take theirs), allow persistent settings over RDM, mqtt...
class ConfigNode: public HomieNode { // seems homie (now?) has a 10 settings limit. scale down settings, or patch homie?
  public:
  ConfigNode(): HomieNode("pixTol", "config"),
    logArtnet("log_artnet", "log level for incoming artnet packets"),

    stripBytesPerPixel("bytes_per_pixel", "3 for RGB, 4 for RGBW"),
    stripLedCount("led_count", "number of LEDs in strip"), // rework for multiple strips
    // sourceLedCount("source_led_count", "number of pixels in source animation data, will be mapped onto led_count"), // rework for multiple strips
    // sourceBytesPerPixel("source_bytes_per_pixel", "3 for RGB, 4 for RGBW"),

    // universes("universes", "number of DMX universes"),
    startUni("starting_universe", "index of first DMX universe used"),
    // startAddr("starting_address", "index of beginning of strip, within starting_universe."), // individual pixel control starts after x function channels offset (for strobing etc)
    dmxHz("dmx_hz", "dmx frame rate"),      // use to calculate strobe and interpolation stuff

    mirror("mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one"),
    fold("folded_alternating", "alternating pixels")
    // flipped("flip", "flip strip direction")
  {
    logArtnet.setDefaultValue(0);

    stripBytesPerPixel.setDefaultValue(4).setValidator([] (long val) {
        return (val > 0 && val < 7);});
    stripLedCount.setDefaultValue(125).setValidator([] (long val) {
        return (val > 0 && val <= 288);});
    // sourceBytesPerPixel.setDefaultValue(4).setValidator([] (long val) {
    // 		return (val > 0 && val < 7);});
    // sourceLedCount.setDefaultValue(125).setValidator([] (long val) {
    // 		return (val > 0);});

    // universes.setDefaultValue(1).setValidator([] (long val) {
    // 		return (val > 0 && val <= 4);});
    startUni.setDefaultValue(1).setValidator([] (long val) {
        return (val > 0 && val <= 16);}); // for now, one artnet subnet...
    // startAddr.setDefaultValue(1).setValidator([] (long val) {
    // 		return (val > 0 && val <= 512);});
    dmxHz.setDefaultValue(40).setValidator([] (long val) {
        return (val > 0 && val < 176);}); // 4 x dmx max

    mirror.setDefaultValue(false);
    fold.setDefaultValue(false);
    // flipped.setDefaultValue(false);
  }
    optInt logArtnet;

    // optInt sourceBytesPerPixel;
    // optInt sourceLedCount;
    optInt stripBytesPerPixel;
    optInt stripLedCount;

    // optInt universes;
    optInt startUni;
    // optInt startAddr;
    optInt dmxHz;

    optBool mirror;
    optBool fold;
    // optBool flipped;

  protected:
  // virtual bool handleInput(const String& property, const HomieRange& range, const String& value) {
  //   // Homie.getLogger() << "Property: " << property << ", " << "value: " << value;
  //   LN.logf(__func__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
  //   // write to json or whatever, makes sense to can update straight mqtt no json-over-mqtt?
  //  return true;
  // }
  // virtual void setup() override {
  //   // advertise("debug").settable(); //don't need this cause can update settings ocer mqtt anyways
  // }
};
