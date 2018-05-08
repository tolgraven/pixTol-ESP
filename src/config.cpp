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
debug("debug", "debug mode, extra logging, chance to reflash ota on boot"),
logArtnet("log_artnet", "log level for incoming artnet packets"),
logToSerial("log_to_serial", "whether to log to serial"),
logToMqtt("log_to_mqtt", "whether to log to mqtt"),
                                                                                                                                                                                
dmxHz("dmx_hz", "dmx frame rate"),      // use to calculate strobe and interpolation stuff
strobeHzMin("strobe_hz_min", "lowest strobe frequency"),
strobeHzMax("strobe_hz_max", "highest strobe frequency"),
                                                                                                                                                                                
clearOnStart("clear_on_start", "clear strip on boot"),
                                                                                                                                                                                
bytesPerLed("bytes_per_pixel", "3 for RGB, 4 for RGBW"),
ledCount("led_count", "number of LEDs in strip"), // rework for multiple strips
                                                                                                                                                                                
universes("universes", "number of DMX universes"),
startUni("starting_universe", "index of first DMX universe used"),
startAddr("starting_address", "index of beginning of strip, within starting_universe.") // individual pixel control starts after x function channels offset (for strobing etc)
{
  debug.setDefaultValue(true);
  logArtnet.setDefaultValue(2);
  logToMqtt.setDefaultValue(true);
  logToSerial.setDefaultValue(false);

  dmxHz.setDefaultValue(40);
  strobeHzMin.setDefaultValue(1);
  strobeHzMax.setDefaultValue(10);

  clearOnStart.setDefaultValue(false);

  bytesPerLed.setDefaultValue(4).setValidator([] (long candidate) {
			return (candidate > 0 && candidate < 7);});

  ledCount.setDefaultValue(120).setValidator([] (long candidate) {
			return (candidate > 0 && candidate <= bytesPerLed.get() * (universes.get() * ledCount.get()));}); // XXX check

  universes.setDefaultValue(1).setValidator([] (long candidate) {
			return (candidate > 0 && candidate <= 4);});

	startUni.setDefaultValue(1).setValidator([] (long candidate) {
			return (candidate > 0 && candidate <= 16);}); // for now, one artnet subnet...
  startAddr.setDefaultValue(1).setValidator([] (long candidate) {
			return (candidate > 0 && candidate <= 512);});

  universes.setDefaultValue(1);
	startUni.setDefaultValue(1);
  startAddr.setDefaultValue(1);

  // advertise("debug").settable(); //don't need this cause can update settings ocer mqtt anyways
}

bool ConfigNode::handleInput(const String& property, const HomieRange& range, const String& value) {
  // Homie.getLogger() << "Property: " << property << ", " << "value: " << value;
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());
  // write to json or whatever, makes sense to can update straight mqtt no json-over-mqtt?

}
