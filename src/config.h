#pragma once

#ifdef HOMIE_SHIT_EH
// #include <Homie.h>
#endif

// #include <ConfigManager.h> //tho at least parts of layout should be standalone from this
#include "log.h"


#define MILLION       1000000

struct ConfigSkeletor {
  char hostName[20];
    char name[20];
    bool enabled;
    int8_t hour;
    char password[20];
} config;

struct Metadata {
    int8_t version;
} meta;

// homie structore decent beginning
// struct Confu {
//   name = "pixtol-proto",
//   device_id = "pixtol-proto",
//   device_stats_interval = 300,
//   "wifi" = {
//     ssid = "sawa",
//     password = "lappisdudes"
//   },
//   "mqtt" = {
//     host = "192.168.1.32",
//     base_topic = "pixtol/",
//     auth = true,
//     username = "paj",
//     password = "razziamutan"
//   },
//   "ota" = {
//     host = "nu",
//     enabled = true,
// 		port = 9080,
//     path = "/ota"
//   },
//   "settings" = {
//     log_artnet = 0,
//     dmx_hz = 40,
// 		mirror_second_half = true,
//     folded_alternating = false,
// 		led_count = 120,
// 		bytes_per_pixel = 4,
// 		starting_universe = 2
//   }
// }

// class Config {
//   ConfigManager* cfgMan;

//   Config() {
//     cfgMan = new ConfigManager();
//     // dunno these
//     // cfgMan.setAPName("Demo");
//     // cfgMan.setAPFilename("/index.html");

//     // it recs using a common struct but could also
//     // link to data straight in place where needed...
//     // so could be a nice sorta flip of consumer not fetching
//     // but pointing what they need populated.
//     // tho scattered data ffa obvs sucks for anything with several consumers
//     // also exposes REST.
//     cfgMan->addParameter("name", config.name, 20);
//     cfgMan->addParameter("enabled", &config.enabled);
//     cfgMan->addParameter("hour", &config.hour);
//     cfgMan->addParameter("password", config.password, 20, set);
//     cfgMan->addParameter("version", &meta.version, get);
//     cfgMan->begin(config);
//   }

// };



#ifdef HOMIE_SHIT_EH
// FIX somehow: dont drop into config mode off invalid settings, or at least reads rest of config and
// preselect that + default for wrong one. Also event blink angry etc
// OI! store at least dimmer val, in spiffs, and restore on reboot if actively set
using optBool   = HomieSetting<bool>;
using optInt    = HomieSetting<long>;
using optFloat  = HomieSetting<double>;
using optString = HomieSetting<const char*>;

// XXX sort own spiffs-writer/reader (or take theirs), allow persistent settings over RDM, mqtt...
class ConfigNode: public HomieNode { // seems homie (now?) has a 10 settings limit. scale down settings, or patch homie? *1/
// class ConfigNode { // seems homie (now?) has a 10 settings limit. scale down settings, or patch homie?
  public:
  ConfigNode(): HomieNode("pixTol", "config"),
  // ConfigNode():
    stripBytesPerPixel("bytes_per_pixel", "3/4"),
    stripLedCount("led_count",      "nr LEDs"), // rework for multiple strips
    // sourceBytesPerPixel("source_bytes_per_pixel", "3 for RGB, 4 for RGBW"),

    // universes("universes",       "number of DMX universes"),
    startUni("starting_universe",   "first uni"),
    // startAddr("starting_address", "index of beginning of strip, within starting_universe."), // individual pixel control starts after x function channels offset (for strobing etc)
    dmxHz("dmx_hz",                 "dmx frame rate"),      // use to calculate strobe and interpolation stuff

    mirror("mirror_second_half",    "mirror strip back on itself"),
    // flip("flip", "flip strip direction")
    fold("folded_alternating",      "alternating pixels")
    {

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
    // flip.setDefaultValue(false);
  }
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
    // optBool flip;

  protected:
  // virtual bool handleInput(const String& property, const HomieRange& range, const String& value) {
  //   // Homie.getLogger() << "Property: " << property << ", " << "value: " << value;
  //   lg.logf(__func__, tol::Log::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
  //   // write to json or whatever, makes sense to can update straight mqtt no json-over-mqtt?
  //  return true;
  // }
  // virtual void setup() override {
  //   // advertise("debug").settable(); //don't need this cause can update settings ocer mqtt anyways
  // }
};

#endif

// struct PixtolConfig {
//   PixtolConfig(const String& id): id(id) {}
//   String type, id; //name of app/thing, id of individual unit

//   struct Port {
//     Port(const String& id, uint16_t port): id(id), port(port) {}
//     String id;
//     uint16_t port; //universe or equiv, network port, physical pin/port id, w/e
//     // should this contain kind, or is that for owner to know?
//     // prob go in here somehow? but like from above types, not "artnet" or w/e anyways bc we also want
//     // bool exclusive; //do we require exclusive access? yes for network binds, physical... wait, everything, nvm
//   };
//   struct TransformConfig {
//     bool mirror = false, fold = false, flip = false; //for now. look in NPB geometry for insp...
//   };
//   enum DataUnit { BYTE, DOUBLEBYTE, FLOAT };
//   struct IOConfig {
//     IOConfig(const String& id, uint8_t fieldSize, uint16_t fieldCount, DataUnit dataType = BYTE):
//       id(id), fieldSize(fieldSize), fieldCount(fieldCount), dataType(dataType) {
//       }
//     IOConfig& addPort(const String& id, uint16_t number) {
//       ports.push_back(Port(id, number));
//       return *this;
//     }
//     String id; //eg "ArtNet". use to look up from list of available...
//     uint8_t fieldSize;
//     uint16_t fieldCount;
//     DataUnit fieldType; // uint16_t size; //size, in bytes, required for each port buffer
//     std::vector<Port> ports;
//     uint8_t maxPorts; //eg a Strip can only have 1
//     uint16_t targetFrameRate = 40; // 0 = n/a
//     uint16_t throughput = 0; //us per byte or whatever, 0 = n/a
//     std::vector<TransformConfig> transforms; //adaptations to be applied directly on/right before RX/TX (eg strip layout shite)
//   };
//   struct InputConfig: public IOConfig {}; //anything specific?
//   struct OutputConfig: public IOConfig {
//     uint16_t powerDraw; //mA per field?
//     uint16_t powerCap; //watts per unit?
//   };
//   std::vector<InputConfig> inputs;
//   std::vector<OutputConfig> outputs;

//   struct PatchConfig {
//     // Something* from; //nah that's for the Patch, not its cfg
//     Source from; //Source is Port or w/e, or an intermediate pool buffer config
//     std::vector<TransformConfig> transforms;
//     Target to; //wait so is Target. how mirror RenderStage here...
//   };
//   std::vector<PatchConfig> patches;
// };

// PixtolConfig pixtolCfg("pixTol");
// // PixtolConfig::InputConfig art = PixtolConfig::InputConfig("ArtNet", 1, 512);
// PixtolConfig::InputConfig art("ArtNet", 1, 512);
// art.addPort("Universe 10", 10).addPort("Universe 11", 11);
// pixtolCfg.inputs.push_back(art); //emplace_back instead maybe
//
// {
//   "id": "Module",
//   "fieldType": "BYTE",
//   "fieldSize": 1,
//   "fieldCount": 1,
//   "types": [{
//     "id": "inputters",
//       "types": [{
//         "id": "NetworkIn", //NetworkBuffer maybe better name? DmxDerived?
//         "fieldCount": 512,
//         "targetframerate": 40,
//         "types": [
//           {"id": "ArtNet", "maxPorts": 4},
//           {"id": "sACN", "maxPorts": 32}, //correct? think read...
//           {"id": "OPC"}]
//    }, {
//      "id": "MQTT",
//      "fieldType": "FLOAT"
//    }, {
//      "id": "Sensor",
//      "fieldType": "FLOAT",
//      "types": [
//        {"id": "Voltage"}, {"id": "Fluid"}, {"id": "Air"},
//        {"id": "Beacon"}, {"id": "Signal Strength"}]
//    }]},
//      {"id": "outputters",
//       "types": [{
//         "id": "Strip",
//         "types": [{
//           "id": "SK6812",
//           "fieldSize": 4, //default RGBW, but also available RGB, single color etc...
//           "throughput": 10,
//           "types": [{
//             "id": "RGBW",
//             "types": [
//               {"id": "1m144 125", "fieldCount": 125},
//               {"id": "1m60 120", "fieldCount": 120 }]
//           }]
//           }. {
//             "id": "WS2812b",
//             "fieldSize": 3,
//             "types": [
//             {"id": "1m144 144", "fieldCount": 144},
//             {"id": "1m60 150", "fieldCount": 150 }]
//           }]
//       }, {
//      "id": "Pin"
//    }]},
//      {"id": "modulators",
//       "types": [{ //dunno name
//         "id": "dimmer",
//         "response": "InOut" //["attack", "release"]
//         }, { //or base "gain", subtype "dimmer" which only goes down. cause need up-gain too, and so similar...
//         "id": "Strobe", //or like Shutter, use of which is a strobe, when, targeting dimmer, (cant just use dimmer itself, since not abs)
//         // and strobe effect itself just basic generator that should be patchable onto anything
//         }, {
//         "id": "rotate", //buffer
//       }]},
//      {"id": "easing",
//       "kinds": [{"id": "None"}, {"id": "In"}, {"id": "Out"}, {"id": "InOut"}], //ehh yeah figure out
//       "types": [
//        {"id": "Cubic",
//          "functions": ["in-formula", "out-formula", "in-out-formula"],
//          {"id": "JoefancyMaz",
//          "functions": ["one", "2", "3"]}}]
//      }, {
//    "id": "generators", //animations etc
//    "types": [{
//      "id": "stagedAnimation", //or well this is only 1-param anim hehu
//      "stages" [{
//        "id": "animationStage", //eh name
//        "maxDuration": 0,
//        "minDuration": 0,
//        "maxFraction": 0,
//        "onComplete": "NEXT",
//
//        "actions": ["None"], //better name?
//
//        "types": [ //actual default implementations
//        {"id": "Pre", //debouncer stage, never repeats
//          "minDuration": 50 //rest auto, so this becomes exactly duration...
//        }, {
//          "id": "Lock", //so like, OPEN and CLOSED in shutter's stages
//          "actions": ["Level"] //"setAndLockLevel"
//          // impl cfg then has actual target... scaler?
//        }, {
//          "id": "Fade",
//          "actions": ["Level", "Saturation"], //think yellowing strobe ye
//          "bounds": [ 0.0, 1.0 ], //just for default...
//          "easing": "None" //for main anim progression/time response. actions surely have their own...
//          }],
//        }]
//    }, {
//      "id": "Fill", //put shit inna buffer. Prob a color
//      "action": "RepeatField",
//    }]
//    }]
// }

// { //wip actual end-user cfg
//   "type": "pixTol Stripper",
//   "id": "pixTol-1",
//   "inputs": [{
//     "id": "ArtNet", //should merge with base cfg with defaults for id, so only need setting ports, optionally framerate...
//     "ports": [{ "id": "Universe 10", "port": "10"}],
//     "targetframerate": 40,
//   }, {
//     "id": "sACN",
//     "ports": [{ "id": "Universe 10", "port": "10"}],
//     "targetframerate": 40,
//   }], //and so on, opc mqtt whatevs
//   "outputs": [{
//     "id": "SK6812", //again, look up against further cfg with defaults (so no need set fieldType, maxPorts... + 4 as default for fieldCount, etc)
//     "fieldCount": 125,
//     "ports": [{ "id": "Strip 1", "port": "RX"}],
//     "throughput": 40, //us. but add stop pause overhead etc hmm...
//     "transforms": [{ "mirror": false, "fold": false, "flip": true }]
//   }]
// }
//
// PixtolConfig::OutputConfig strip = PixtolConfig::OutputConfig("SK6812", 4, 125);
// art.addPort("Strip 1", RX); //should then handle method automatically depending on what's available
// // "RX", "D4" etc should obviously be abstracted, incl on hardware.
// pixtolCfg.inputs.push_back(strip);
