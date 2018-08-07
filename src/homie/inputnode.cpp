#include "inputnode.h"

// ArtnetnodeWifi artnet;

// InputNode::InputNode() {  // construct from cfg params
//   // ideally, get inputter subnodes registered here, then run through all their init funcs?
//   // initially, put all that shit in this class...
//   initArtnet("test - Artnet");
//
// }

bool InputNode::handleInput(const String& property, const HomieRange& range, const String& value) {
 	LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Got prop %s, value=%s", property.c_str(), value.c_str());

  // if(property.equals("artnet")) {
  //   if(value.equals("on")) {
  //     for(uint8_t i=0; i < cfg->universes.get(); i++) {
  //       artnet.enableDMXOutput(i);
  //     }
  //   } else {
  //     for(uint8_t i=0; i < cfg->universes.get(); i++) {
  //       artnet.disableDMXOutput(i);
  //   }
  // }
  // else if(property.equals("serial")) {
  //   if(value.equals("on")) {
  //
  //   } else {
  //
  //   }
  // }
  // else if(property.equals("mqtt")) {
  //   if(value.equals("on")) {
  //
  //   } else {
  //
  //   }
  // }
  // else if(property.equals("dmx")) {
  //   if(value.equals("on")) {
  //
  //   } else {
  //
  //   }
  // }

	return true;
}


void InputNode::initArtnet(const String& name) {
  // artnet.setName(name);
  // artnet.setNumPorts(cfg->universes.get());
	// artnet.setStartingUniverse(cfg->startUni.get());
	// for(uint8_t i=0; i < cfg->universes.get(); i++) artnet.enableDMXOutput(i);
  // // artnet.enableDMXOutput(0);
	// artnet.begin();
	// artnet.setArtDmxCallback(onArtnetFrame);
}

