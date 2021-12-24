#pragma once

#include <ArduinoOSC.h>
#include "renderstage.h"

//need to put all our exposable/exposed controls from modulators, settings etc in a structure
//that gets parsed and auto osc/mqtt/etc bindings created.

class Osc: public IO {

    OSC(const std::string& pathPrefix, uint8_t numPorts: hz(sourceHz), name(deviceName) {
    }

    void update() { osc.parse(); }

    void subscribe(const std::string& address, callback) { }

  WiFiUDP udp;
  ArduinoOSCWiFi osc;
  int recv_port = 10000;
  int send_port = 12000;
};

class OSCInput: public Inputter {
  public:
    OSCInput(const std::string& pathPrefix): Inputter() {
      osc.addCallback("/ard/aaa", &callback);
      osc.addCallback("/ard", &callback);
    }

    virtual bool read() {}

  private:
    // OscServer* server;
};

class OSCOutput: public Outputter {

  private:
    // OscClient* client;
};


void callback(OSCMessage& m)
{
    OSCMessage msg; //create new osc message
    msg.beginMessage(host, send_port);
    //set argument
    msg.setOSCAddress(m.getOSCAddress());

    msg.addArgInt32(m.getArgAsInt32(0));
    msg.addArgFloat(m.getArgAsFloat(1));
    msg.addArgString(m.getArgAsString(2));

    osc.send(msg); //send osc message
}
