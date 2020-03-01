#pragma once

#include <OpcServer.h>
#include "log.h"
#include "renderstage.h"

#define OPC_PORT_DEFAULT 7890

class OPC: public NetworkIn {
  public:
  uint8_t maxClients = 1;  // can use multiple simultaneous clients/sources

  std::unique_ptr<WiFiServer> wifiServer;
  std::unique_ptr<OpcClient> client;
  std::unique_ptr<OpcServer> server;

  OPC(const String& id, uint16_t startChannel, uint8_t numPorts, uint16_t pixels):
    NetworkIn(id, startChannel, numPorts, pixels),
    wifiServer(new WiFiServer(OPC_PORT_DEFAULT)),
    client(new OpcClient()),
    server(new OpcServer(*wifiServer, startChannel, client.get(), maxClients,
                         buffer().get(), sizeInBytes() + OPC_HEADER_BYTES)) {
    setType("opc");
    using namespace std::placeholders;
    server->setMsgReceivedCB(std::bind(&Inputter::onData, this, _1, _3, _4, true)); // _2 is "command", check what means...
    server->setClientConnectedCB(std::bind(&OPC::cbOpcClientConnected, this, _1));
    server->setClientDisconnectedCB(std::bind(&OPC::cbOpcClientDisconnected, this, _1));

    server->begin();
  }

  bool _run() override {
    server->process();
    return dirty(); // still bit shitty. lib already patched - fork and add flags etc?
  }

  void cbOpcClientConnected(WiFiClient& client) {
    lg.f("OPC", Log::INFO, "Client Connected\n");
  }

  void cbOpcClientDisconnected(OpcClient& opcClient) {
    lg.f("OPC", Log::INFO, "Client Disconnected\n");
  }
};
