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
// debug("debug", "debug mode, extra logging, chance to reflash ota on boot"),
logArtnet("log_artnet", "log level for incoming artnet packets"),
// logToMqtt("log_to_mqtt", "whether to log to mqtt"),
// logToSerial("log_to_serial", "whether to log to serial"),

dmxHz("dmx_hz", "dmx frame rate"),      // use to calculate strobe and interpolation stuff
// strobeHzMin("strobe_hz_min", "lowest strobe frequency"),
// strobeHzMax("strobe_hz_max", "highest strobe frequency"),

clearOnStart("clear_on_start", "clear strip on boot"),

bytesPerLed("bytes_per_pixel", "3 for RGB, 4 for RGBW"),
interFrames("inter_frames", "how many frames of interpolation"),
ledCount("led_count", "number of LEDs in strip"), // rework for multiple strips
// sourceLedCount("source_led_count", "number of pixels in source animation data, will be mapped onto led_count"), // rework for multiple strips

// universes("universes", "number of DMX universes"),
startUni("starting_universe", "index of first DMX universe used"),
// startAddr("starting_address", "index of beginning of strip, within starting_universe."), // individual pixel control starts after x function channels offset (for strobing etc)


setMirrored("mirror_second_half", "whether to mirror strip back onto itself, for controlling folded strips >1 uni as if one"),
setFolded("folded_alternating", "alternating pixels")
// setFlipped("flip", "flip strip direction")
{
    setMirrored.setDefaultValue(false);
    setFolded.setDefaultValue(false);
    // setFlipped.setDefaultValue(false);
  // + like freeze("freeze", "stops all inputs and outputs") to try to flick if having a hard time flashing ota?
  // debug.setDefaultValue(true);
  logArtnet.setDefaultValue(2);
  // logToMqtt.setDefaultValue(true);
  // logToSerial.setDefaultValue(false);

  dmxHz.setDefaultValue(40);
  strobeHzMin.setDefaultValue(1);
  strobeHzMax.setDefaultValue(10);

  // strobeHzMin.setDefaultValue(1).setValidator([] (double val) {
  //     return (val > 0.1 && val < 20);});
  // strobeHzMax.setDefaultValue(10).setValidator([] (double val) {
	// 		return (val > 0.1 && val < 20);});

  ledCount.setDefaultValue(120).setValidator([] (long candidate) {
			return (candidate > 0 && candidate <= bytesPerLed.get() * (universes.get() * ledCount.get()));}); // XXX check

  bytesPerLed.setDefaultValue(4).setValidator([] (long val) {
			return (val > 0 && val < 7);});
  ledCount.setDefaultValue(120).setValidator([] (long val) {
			// return (val > 0 && val <= bytesPerLed.get() * (universes.get() * ledCount.get()));}); // XXX check
			return (val > 0 && val <= 288);});
  // sourceLedCount.setDefaultValue(120).setValidator([] (long val) {
	// 		return (val > 0 && val <= 288);});

  // universes.setDefaultValue(1).setValidator([] (long val) {
	// 		return (val > 0 && val <= 4);});
	startUni.setDefaultValue(1).setValidator([] (long val) {
			return (val > 0 && val <= 16);}); // for now, one artnet subnet...
  // startAddr.setDefaultValue(1).setValidator([] (long val) {
	// 		return (val > 0 && val <= 512);});

  interFrames.setDefaultValue(2);

  // advertise("debug").settable(); //don't need this cause can update settings ocer mqtt anyways
}

bool ConfigNode::handleInput(const String& property, const HomieRange& range, const String& value) {
  // Homie.getLogger() << "Property: " << property << ", " << "value: " << value;
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
  // write to json or whatever, makes sense to can update straight mqtt no json-over-mqtt?

}
