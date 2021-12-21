#pragma once

#include <functional>
#include <artnet4real.h>
#include <AsyncUDP.h>

#include "smooth/core/logging/log.h"

#include "log.h"
#include "renderstage.h"
#include "task.h"


namespace tol {

using namespace smooth;
using namespace smooth::core::ipc;

// using namespace anfr;
namespace artnet = anfr;

class ArtnetDriver: public IEventListener<IPAddress>, public core::Task { // or just wrap this after?
  // const int pollReplyInterval = 4000; // XXX figure out how the fuck this doesnt work as supposed to
  // milliseconds{4000} to Task works. milliseconds{const int} turns ns or some shit,
  // static const + constexpr milliseconds works. But passing ints works everywhere else.
  static const int pollReplyInterval = 4000; // well actually it's 2500-3000 and only for controllers. 4s is resend stale frame, but 0.8-1.0s recommended "for sacn compat"
  // static constexpr milliseconds pollReplyTick = milliseconds{pollReplyInterval};
  static constexpr auto pollReplyTick = milliseconds{pollReplyInterval};
  std::unique_ptr<artnet::Driver> artnet;
	AsyncUDP socket; //fix more flexibility so ethernet etc tho! maybe template like applemidi lib but so much boilerp
  bool started = false;

  public:
  ArtnetDriver(const String& id):
    core::Task("artnet-driver", 4096, core::APPLICATION_BASE_PRIO + 2, pollReplyTick),
    artnet(new artnet::Driver((id + " ArtNet").c_str())) {
  }
  // template<class Fun, class... Args> typename std::result_of<Fun&&(Args&&...)>::type
  // template<class Fun, class... Args>
  //   auto operator()(Fun&& f, Args&&... args) { artnet->(f)(std::forward<Args>(args)...); }
  artnet::Driver* operator*() { return get(); }
  artnet::Driver* operator->() { return get(); }
  artnet::Driver* get() { return artnet.get(); }
  template<typename ...Args, typename T>
  void operator()(T (artnet::Driver::* function)(Args...), Args&& ...args) {
    return (artnet->*function)(std::forward<Args>(args)...);
  }

  void init(const IPAddress& ip, const IPAddress& sub, uint8_t* mac) { // this should be called in response to NetworkEvent.
    if(!started) {
      lg.dbg("ARTNET INIT"); // BAD ERROR
      if(ip != INADDR_NONE)
        artnet->init(ip, sub, mac);
      else
        artnet->init();

      artnet->setPacketSendFn([this](IPAddress dest, uint8_t* data, uint16_t length,
                                     uint16_t port = artnet::def::defaultUdpPort) {
            AsyncUDPMessage packet{length};
            packet.write(data, length);
            auto res = this->socket.sendTo(packet, dest, port);
            // lg.dbg("ARTNET SEND PACKET");
          });

      if(!socket.listen(artnet::def::defaultUdpPort)) { // Start listening for UDP packets
        lg.dbg("ARTNET BAD CANT LISTEN"); // BAD ERROR
      } else {
        // socket.onPacket([this](AsyncUDPPacket& packet) { // INFO copy not ref due to AsyncUDP bug (pbuf_free thing, copy-ctor PR exists but is stale and might have issues(=))
        socket.onPacket([this](AsyncUDPPacket packet) { // INFO copy not ref due to AsyncUDP bug (pbuf_free thing, copy-ctor PR exists but is stale and might have issues(=))
                                                        // fucking or not? #3290 it is then.
            // this (asyncudp) was acting up again (stack overflow if lucky, thread stuck if unlucky)
            // move to smooth equivalent asap... looks a bit fiddly tho
            this->artnet->onPacket(packet.remoteIP(), packet.data(), packet.length());
            // lg.dbg("ARTNET RECEIVE PACKET");
          });
      }
      artnet->begin();
      started = true;
    }
  }

  void stop() { started = false; socket.close(); }

  void event(const IPAddress& ip) override {
    DEBUG("Artnet got IP event");
    artnet->setIP(ip); //etc. update config. more to come.
  }

  void tick() override {     //only on start and response. unless controller. or controllers poll says yada...
    // lg.dbg("ARTNET driver tick - send pollreply");
    artnet->sendPollReply(); // ola runs as node tho so welp gotta spam.
  }

  void cbRDM() {} // get on so can start using
};

class ArtnetIn: public NetworkIn {
  std::shared_ptr<ArtnetDriver> driver;
  uint32_t runs = 0, attempts = 0;
  Buffer bigData;

  public:
  ArtnetIn(uint16_t startUni, uint8_t numPorts, std::shared_ptr<ArtnetDriver> driver):
    NetworkIn("artnet in", startUni, numPorts), driver(driver),
    bigData(Buffer("Artnet Chunk", 1, numPorts * 512)) {
      setType("input");

      (*driver)->setupBulkInputs(artnet::Universe(startUni, 0, 0), bigData.get(), numPorts * 512);
      auto off = bigData.get();
      for(auto& b: buffers()) {
        b->setPtr(off);
        off += 512;
      }

      driver->get()->setArtDMXCallback(
          [this](uint8_t group, uint8_t port, uint8_t* data, uint16_t length, bool defer) {
            if(data) { // DEBUG("DMX DATA INCOMING");
              // this->onData(port, length, data, !defer);
              this->onData(group * 4 + port, length, data, !defer);
              this->getCounts().run = ++runs; // XXX TEMP haha
              this->getCounts().attempt = ++attempts;
            }
      });
      driver->get()->setArtSyncCallback([this] {
            for(auto&& buf: this->buffers())
              buf->setDirty(true); // TODO just store incoming on internal queue then post all on sync
      });
  }
};

class ArtnetOut: public Outputter {
  // should be a PatchOut subscriber somehow (probably put in Outputter itself...)
  // then to enforce FPS simply have like a one-time scheduled event Task
  // which gets a callback in this class
  // and gets scheduled to fire in (ms til next keyframe)
  // IMPORTANT might get more frames while waiting, so first clear any scheduled Task.
  // this way no relying on silly polling for frame-limited shit!

  // nor risk of flushing buffer while it's mid-update (could (should?) also be done with mutex)
  // since still very much run that risk I suppose if Renderer spins all spare cycles...
  //
  // actually amt of Tasks = num buffers/ports...
  std::shared_ptr<ArtnetDriver> driver;

  public:
  ArtnetOut(uint16_t startUni, uint8_t numPorts, std::shared_ptr<ArtnetDriver> driver):
    Outputter("artnet out", 1, 512, numPorts), driver(driver) {
      setType("output");

      // oh yeah so have to re-engineer internal Port getup a bit...
      // if same group is to be able to have one port both send/receive.
      // bring back ext buffer option, then sendDMX optionally becomes just a signal
      // to take what's curr in buf and send.
      // - buffer optional (might need 1 in, 0 in 0 out, 1 out...)
      uint8_t groupID = 0; // net, subnet
      // for(int p=0; p < numPorts; p++) {
      //   auto port = (*driver)->getPort(groupID, p);
      //   if(port) port->portType = artnet::PortArtnetHub; // XXX temp test
      // }
       
      // uint8_t groupID = (*driver)->addGroup(0, 1)->index; // net, subnet
      
      // uint8_t groupID = (*driver)->addGroup(0, 0); // fuckit lets try dupl
      // uint8_t groupID = 0; // XXX bad depends on in creating it first...
      // causes buffer overflow in "IP" because called from there? lol
      // but we're not even are we??

      for(int portIdx=0; portIdx < numPorts; portIdx++) {
        auto port = (*driver)->getPort(0, portIdx);
        if(!port) {
          int pID = (*driver)->addPort(groupID, portIdx, startUni + portIdx, artnet::PortArtnetIn); // XXX have swapped "In" and "Out"
        } else {
          port->portType = artnet::PortArtnetHub;
        }
        
      }
  }

  bool _run() override {
    bool outcome = false;
    for(uint8_t port=0; port < buffers().size(); port++) { // XXX bad wrong because now eg port 3 is idx 0
      // uint8_t group = 1; //send-group, made-up for now...
      uint8_t group = 0; //send-group, made-up for now...
      auto& buf = buffer(port);
      // it's weird tho how buf1/controls somehow throttles even w/o dirty check? not actually impl like...
      // turns out setting maxHz makes it start pushing data even when no changes.
      // WEIRD! bc should only affect microsTilReady() right?

      if(buf.dirty()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
      // if(buf.dirty() || port.hasnt_sent_in_4s_yada()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
        // (*driver)->sendDMX(group, port, buf.get(), buf.lengthBytes()); // so it's required it properly set up. wont overflow at least
        (*driver)->sendDMX(group, port, buf.get(), buf.lengthBytes()); // XXX TEMP
        buf.setDirty(false); // our view anyways. Many Buf same addr both good and tricky
        outcome = true;
      }
    }
    return outcome;
  }

  void onEvent(const PatchOut& event) override {
    uint8_t group = 0; //send-group, made-up for now...
    auto& buf = buffer(event.i);
    // it's weird tho how buf1/controls somehow throttles even w/o dirty check? not actually impl like...
    // turns out setting maxHz makes it start pushing data even when no changes.
    // WEIRD! bc should only affect microsTilReady() right?

    if(buf.dirty() /* || port.hasnt_sent_in_4s_yada() */) { // should always be dirty if event arrived tho
      (*driver)->sendDMX(group, event.i, buf.get(), buf.lengthBytes());
      buf.setDirty(false); // our view anyways. Many Buf same addr both good and tricky
    }
    
  }

  void cbRDM() {} // get on so can start using
};

}
