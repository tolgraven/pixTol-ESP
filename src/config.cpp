#include "config.h"

//this prob makes no sense. Put debug/log in log node, dmx in that node, strip-related in strip node...
//could use class for the config management thing tho
//network down = leader esp starts hotspot, rest look for it, connecting and receiving new wifi cfg

// HomieSetting<const char*> ConfigNode::mode("mode", "what mode iz running?");
// more likely got InputNode, OutputNode, can toggle bunch of stuff on/off independently
// tho that'd need proper routing
// so manual = dmx input off, mqtt/http input on

ConfigNode::ConfigNode():
HomieNode("pixTol", "config"),

logArtnet("log_artnet", "log level for incoming artnet packets"),

dmxHz("dmx_hz", "dmx frame rate"),      // use to calculate strobe and interpolation stuff

stripBytesPerPixel("bytes_per_pixel", "3 for RGB, 4 for RGBW"),
stripLedCount("led_count", "number of LEDs in strip"), // rework for multiple strips
// sourceLedCount("source_led_count", "number of pixels in source animation data, will be mapped onto led_count"), // rework for multiple strips
// sourceBytesPerPixel("source_bytes_per_pixel", "3 for RGB, 4 for RGBW"),

// universes("universes", "number of DMX universes"),
startUni("starting_universe", "index of first DMX universe used"),
// startAddr("starting_address", "index of beginning of strip, within starting_universe."), // individual pixel control starts after x function channels offset (for strobing etc)

setMirrored("mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one"),
setFolded("folded_alternating", "alternating pixels")
// setFlipped("flip", "flip strip direction")
{
  logArtnet.setDefaultValue(0);

  dmxHz.setDefaultValue(40).setValidator([] (long val) {
			return (val > 0 && val < 176);}); // 4 x dmx max

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

    setMirrored.setDefaultValue(false);
    setFolded.setDefaultValue(false);
    // setFlipped.setDefaultValue(false);
}
// FIX somehow: dont drop into config mode off invalid settings, or at least reads rest of config and
// preselect that. Also event blink angry etc

bool ConfigNode::handleInput(const String& property, const HomieRange& range, const String& value) {
  // Homie.getLogger() << "Property: " << property << ", " << "value: " << value;
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
  // write to json or whatever, makes sense to can update straight mqtt no json-over-mqtt?
	return true;
}

void ConfigNode::setup() {
  // advertise("debug").settable(); //don't need this cause can update settings ocer mqtt anyways
  // ledCount.setValidator([] (long candidate) {
	// 		return (candidate > 0 && candidate <= bytesPerLed.get() * (universes.get() * ledCount.get()));}); // XXX check
}
