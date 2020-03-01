#pragma once

#include <functional>
#include <ArtNetE131Lib.h>
#include "log.h"
#include "renderstage.h"

// I mean indirection for Driver is just dumb since all calls to it is gonna be type-dependent haha.
// class ArtnetDriver: public Driver<espArtnetRDM> {
//   ArtnetDriver():
//     Driver("Artnet") {};
// };
class ArtnetDriver: public Runnable { // well prob want to inherit Task directly? but leave design open...
// class ArtnetDriver { // or just wrap this after?
  std::unique_ptr<espArtNetRDM> artnet;
  bool started = false;
  bool newInput = false;

  public:
  ArtnetDriver(const String& id):
    Runnable(id, "driver"),
    artnet(new espArtNetRDM()) {
    artnet->init(id.c_str());
  }
  // template<class Fun, class... Args>
  // typename std::result_of<Fun&&(Args&&...)>::type
  // operator()(Fun&& f, Args&&... args) {
  //   artnet->std::forward<Fun>(f)(std::forward<Args>(args)...);
  // }

  // espArtnetRDM& operator() { return *(*_driver); }
  // espArtnetRDM* operator* { return *_driver; }
  espArtNetRDM* operator*() { return artnet.get(); }
  espArtNetRDM* operator->() { return artnet.get(); }
  espArtNetRDM* get() { return artnet.get(); }
  // espArtNetRDM* get() { return *artnet; }
  template<typename ...Args, typename T>
  // typename std::result_of<Fun&&(Args&&...)>::type
  // T forward(T (espArtNetRDM::* function)(Args...), Args &&...args) {
  void operator()(T (espArtNetRDM::* function)(Args...), Args&& ...args) {
    return (artnet->*function)(std::forward<Args>(args)...);
  }

  void init() {
    if(!started) {
      artnet->begin();
      started = true;
    }
  }

  bool _run() {
    auto res = artnet->handler(); // would want to translate (ext-of-driver relevant) artnet types to some generic ones
    if(res == ARTNET_ARTDMX) {    // so DMX = "dataInput", RDM = "configYada",
      newInput = true;
      return true;
    }
    return false;
  }

  void cbRDM() {} // get on so can start using

  bool checkAndClearInput() {
    if(newInput) {
      newInput = false;
      return true;
    }
    return false;
  }
};

class ArtnetIn: public NetworkIn {
  std::shared_ptr<ArtnetDriver> driver;

  public:
  ArtnetIn(const String& id, uint16_t startUni, uint8_t numPorts, ArtnetDriver* driver):
    NetworkIn(id, startUni, numPorts), driver(driver) {
      setType("artnet");

      uint8_t groupID = driver->get()->addGroup(0, 0); // net, subnet
      // uint8_t groupID = driver()->addGroup(0, 0); // net, subnet
      // uint8_t groupID = driver(addGroup, 0, 0); // net, subnet
      for(int port=0; port < numPorts; port++) {
        driver->get()->addPort(groupID, port, startUni + port, (uint8_t)port_type::RECEIVE_DMX);
      }

      driver->get()->setArtDMXCallback(
          [this](uint8_t group, uint8_t port, uint16_t length, bool sync) {
            uint8_t* data = this->driver->get()->getDMX(group, port);
            if(data) this->onData(port, length, data, !sync);
      });
      driver->get()->setArtSyncCallback([this] {
            for(auto&& buf: this->buffers())
            buf->setDirty(true);
      });

      driver->init(); // cause lib aint ready for begin() before got groups and stuff right? so gotta dupl for now. fix
  }

  // now with cb silliness refactoring exposes it's dumb design in any case to
  // have an I + O underlying driver _IN_ status depend on its general run-fn.
  // but this is because "in" means multiple things:
  // - actual input (dmx)
  // - other networking shit
  //
  // if Driver then becomes standalone Runnable or straight OS task,
  // how keep this neat generalized packet counting?
  //
  // could rely on checking self for dirty(), but if consumer doesnt keep up w input,
  // would double count and shit.
  // But then if Driver coupled to OS event, us coupled to effect of that,
  // need to run() disappears anyways and eg cb could just do a dummy one for stats.
  // different for Outputter side since supposed to stick to a specific clock and such...
  //
  // also afa Artnet since supports multiple "input types" really, the more we can decouple
  // each one the better? then artpollreply status becomes an output ostream, yada lala
  bool _run() override {
    // OLA now complaining about "got unknown packet", "mismatched version" 3584 and stuff. investigate!!
    return driver->checkAndClearInput(); //safe to poll away at I guess, will change soon anyways
  }
};

class ArtnetOut: public Outputter {
  std::shared_ptr<ArtnetDriver> driver;

  public:
  ArtnetOut(const String& id, uint16_t startUni, uint8_t numPorts, ArtnetDriver* driver):
    Outputter(id, 1, 512, numPorts), driver(driver) {
      // setType("artnet");

      // uint8_t groupID = driver->get()->addGroup(0, 1); // net, subnet
      // uint8_t groupID = *driver->addGroup(0, 1); // net, subnet
      uint8_t groupID = (*driver)->addGroup(0, 1); // net, subnet
      for(int port=0; port < numPorts; port++) {
        driver->get()->addPort(groupID, port, startUni + port, (uint8_t)port_type::SEND_DMX);
      }

      driver->init();
  }

  bool _run() {
    bool outcome = false;
    for(uint8_t port=0; port < buffers().size(); port++) {
      uint8_t group = 1; //send-group, made-up for now...
      // auto& buf = buffer(port);
      auto&& buf = buffer(port);
      // it's weird tho how buf1/controls somehow throttles even w/o dirty check? not actually impl like...
      // turns out setting maxHz makes it start pushing data even when no changes.
      // WEIRD! bc should only affect microsTilReady() right?

      if(buf.dirty()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
      // if(buf.dirty() || port.hasnt_sent_in_4s_yada()) { // well not gonna happen because got mult buffer objs using same resource, not us getting set...
        driver->get()->sendDMX(group, port, buf.get(), buf.lengthBytes()); // so it's required it properly set up. wont overflow at least
        buf.setDirty(false); // our view anyways. Many Buf same addr both good and tricky
        outcome = true;
      }
    }
    return outcome;
  }

  void cbRDM() {} // get on so can start using
};

