#pragma once
// read OPC n shit yo.
#include <OpcServer.h>
#include "renderstage.h"


class OPC: public Inputter {

  uint8_t channel = 1;  // channel (apart from 0 = broadcast) to listen to
  uint8_t maxClients = 1;  // can use multiple simultaneous clients/sources

  // uint16_t pixels = 125; // best performance if buffer same size as incoming packet size...
  // uint16_t bufferSize = pixels * 3 + OPC_HEADER_BYTES;

  WiFiServer* wifiServer;
  OpcClient* client[maxClients];
  // uint8_t buffer[bufferSize * maxClients];
  OpcServer* server;

  void setup() override {
    wifiServer = new WiFiServer(port);
    server = new OpcServer(server, channel, client, maxClients, buffer, bufferSize);
    server.setMsgReceivedCallback(cbOpcMessage);
    server.setClientConnectedCallback(cbOpcClientConnected);
    server.setClientDisconnectedCallback(cbOpcClientDisconnected);

    server.begin();
  }
  OPC(uint16_t pixels): Inputter("OPC", 3, pixels), port(7890) {}
  void update() override {
    server.process();
  }

  void cbOpcMessage(uint8_t channel, uint8_t command, uint16_t length, uint8_t* data) {
  }

  void cbOpcClientConnected(WiFiClient& client) {
    LN.logf(__func__, LoggerNode::INFO, "Client Connected: %d", client.remoteIP());
  }

  void cbOpcClientDisconnected(OpcClient& opcClient) {
    LN.logf(__func__, LoggerNode::INFO, "Client Disconnected: %d", opcClient.ipAddress);
  }
};

