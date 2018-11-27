#pragma once
// read OPC n shit yo.
#include <OpcServer.h>
#include "renderstage.h"

#define OPC_PORT_DEFAULT 7890

class OPC: public Inputter {
  public:
  uint8_t _channel = 1;  // channel (apart from 0 = broadcast) to listen to
  uint8_t maxClients = 1;  // can use multiple simultaneous clients/sources

  // uint16_t pixels = 125; // best performance if buffer same size as incoming packet size...
  // uint16_t bufferSize = pixels * 3 + OPC_HEADER_BYTES;
  // uint8_t buffer[125 * 1];

  WiFiServer* wifiServer;
  OpcClient* client;
  OpcServer* server;

  // OPC(uint16_t pixels): Inputter("OPC", 3, pixels), port(7890) {
  OPC(uint16_t pixels): Inputter("OPC", 3, pixels) {
    wifiServer = new WiFiServer(OPC_PORT_DEFAULT);
    // server = new OpcServer(*wifiServer, _channel, client, maxClients, buffers[0]->get(), length() + OPC_HEADER_BYTES);
    server = new OpcServer(*wifiServer, _channel, client, maxClients, buffers[0]->get(), length());
    // server = new OpcServer(*wifiServer, 1, client, 1, buffer, 125 + OPC_HEADER_BYTES);
    using namespace std::placeholders;
    server->setMsgReceivedCB(std::bind(&OPC::cbOpcMessage, this, _1, _2, _3, _4));
    server->setClientConnectedCB(std::bind(&OPC::cbOpcClientConnected, this, _1));
    server->setClientDisconnectedCB(std::bind(&OPC::cbOpcClientDisconnected, this, _1));

    server->begin();
  }

  bool ready() { return true; }
  bool run() { server->process(); }

  uint16_t length() { return RenderStage::length() + OPC_HEADER_BYTES; }

  void cbOpcMessage(uint8_t channel, uint8_t command, uint16_t length, uint8_t* data) {
    uint8_t index = channel - _channel; //XXX check proper
    if(index < _bufferCount) { buffers[index]->set(data, length); }
    newData = true;
  }

  void cbOpcClientConnected(WiFiClient& client) {
    LN.logf(__func__, LoggerNode::INFO, "Client Connected");
  }

  void cbOpcClientDisconnected(OpcClient& opcClient) {
    LN.logf(__func__, LoggerNode::INFO, "Client Disconnected");
  }
};
