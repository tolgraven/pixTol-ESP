#pragma once

#include <vector>
#include <memory>

#include "WiFi.h"

#include "smooth/core/network/Wifi.h"
#include "smooth/core/network/NetworkStatus.h"
// should group some of these used for all...

#include "task.h"
#include "renderer.h"
#include "buffer.h"
#include "io/strip-multi.h"
// #include "io/strip.h"
// #include "io/sacn.h"
#include "io/artnet.h"
// #include "io/opc.h"
#include "functions.h"
#include "watchdog.h"
#include "base.h"

// #include <AsyncUDP.h>

namespace tol {

using namespace smooth;
using namespace smooth::core;
using namespace smooth::core::ipc;
using namespace std::chrono;

namespace event {
// for I/O pipeline purposes basically (and adjusting for flexibility in inserting further
// stages and back-n-forth crap so to a certain point keep a bit generic
// - We deal in keyframes, being::
//  IN.
//    - A full (standalone/assembled/deferred by wait for sync event) input frame (pushed to us)
//    - Data from periodic polling (pulled by us)
//      * variant: animations created on device
//
//  PATCH.
//    - Might result in a mix of or different view of what constitutes key.
//    - Likely wants to shove available data forward afap anyways,
//      meaning "syncing up" gets done later
//
//   RENDER:
//    - The interesting part. Interpolation speed must be balanced vs "exceptional" events.
//    - Example case 1: strobe animation.
//        Generally very simple and hence its very different keyframe tempoo still easily handled.
//        Will add hue-shift halogen emu bs etc but basically, little processing even at full speed.
//      Case 2:
//      Heavier processing. Load caused by effect will have to determine where it can run.
//      Eg only each incoming keyframe.
//      Other instances must run both on incoming, and at interpolation (smoothest spinnaz)
//
//      Of course if limit to 8 parallel (duh) it's all waiting for strips anyways...
//
// Conclusion:
//    I am fucking retarded.  Back to event modeling.
//
//    KEYFRAME IN -> "patcher" grabs from driver to central storage/processing
//    ("what is data for") and correct Buffers (not ex an issue if using OSC or relevant artnet facilities
//    for control and not raw DMX but same diff for assembling and breaking down spannng packets)
//
//
//  0    RAW DATA EVENT                     FRAME              has expected interval, will jitter
//       DATA CLASSIFICATION
//  1    AWAIT FURTHER OR POST DATA EVENT   KEY                real driver exit / IPC entry
//
//       (POSSIBLE PROCESSING)
//
//  2    RENDERING:                         KEY
//         -MERGE STREAMS (not shit like in driver)
//         -BLEND INCOMING (envelope/response etc)
//         -APPLY EFFECTS
//         rolling data, some stagnant / dupl
//
//  3    INTERPOLATION                      FRAME
//         -
//         -FINAL SAY EFFECTS
//
//  4    OUTPUT
//
//  Paths:
//  Basic passthrough - 1 -> 4
//  Basic strip - 1 -> 2 -> 3 -> 4
//
//  Ive done this same sketch way too many times goddamnit just think of what actual things to post
//  and basically how to chuck out events to whoever needs.
//  For example - Interpolation / non-data-hz animation is output dependent
//  (but certainly must for all lights - fhsjdfhjsdfsh)
}

// TODO: whenever an out or in etc added or reconfigured, it gets reflected in DeviceState
// which can also be used to recreate from a config file, which will get written to when stuff done
class DeviceState: public Named {

  struct StageConfig {
    // enabled, name, type (in, out...), subtype (artnet, generator...), startport (aka uni, pin...), numports, fieldSize/count if not default for type, SubFieldOrder, targetFreq...
    // ideally a binding to a factory fn which runs make_shared? dont see how possible haha
    // but apart from switching on type/subtype and hardcoding that,
    // most stuff is generic so can be done from pile of Outputters etc = not that much duplicate boilerplate

    // actually have a constructor that (apart from anything specific) just takes this object, nom
    //
    // something vector of std::variant?
  };

  struct RendererConfig {
    // enabled, name, target, targetfreq/fulltilt, white/blacklist sources? since get all PatchIn events. If really need multiple renderers hah

  };

  // how implement object-custom settings if needed?
  struct ArtnetConfig {

  };
  
  struct FunctionChannelsConfig {

  };

};

// used to be somewhat a scheduler. now just a thing that holds all the io and stuff.
class Scheduler: public Runnable, public core::Task  {
  std::shared_ptr<ArtnetDriver> artnet;
  std::shared_ptr<Renderer> renderer;
  std::vector<std::shared_ptr<Inputter>> ins;
  std::vector<std::shared_ptr<Outputter>> outs;

  EventFnTask<IPAddress> ip{"IP", // really should be put in ArtnetDriver. Which might subclass a Driver which listens to a bunch of events. Generic "Connected"/"Disconnected" event (whether means wifi, bt, physical...) etc
    [this](const IPAddress& ip) {
        uint8_t mac[6]; WiFi.macAddress(mac);
        this->artnet->init(ip, IPAddress(255,255,255,0), mac);
        this->artnet->start(); //start task
    }};

  uint32_t timeInputs = 0;
  float progressMultiplier = 0;

  public:
  Scheduler(const std::string& id):
    Runnable(id, "scheduler"),
    core::Task("not-scheduler", 4096, core::APPLICATION_BASE_PRIO - 3, milliseconds{2500}), // try pin 1
    artnet(std::make_shared<ArtnetDriver>(id)) { // try in main task tho..

      DEBUG("Enter");
    // Three "phases": Collect settings, use to create objects for and patches between IN / OUT / RENDER (tho in the end a sys with a Pi recompiling and reflashing units on fly yet more adaptable)
  }


  void init() override {
    const int startUni = 2;
    const int numPorts = 2;

    auto artnetIn = std::make_shared<ArtnetIn>(startUni, numPorts, artnet);
    ins.push_back(artnetIn); // so can free it up later if needed
    // ins->add(new sAcnInput(id() + " sACN", 1, 1));
    // ins->add(new OPC(id() + " OPC", 1, 1, 120));
    lg.dbg("Done add inputs"); //lg << "Done add inputs!" << endl;

    const int startPin = 15; // had at 25 but apparently some bad pins above that...
    auto strip = std::make_shared<Strip>("Strip 1", 4, 120, startPin, numPorts);
    uint8_t order[] = {1, 0, 2, 3}; // should be set for then/retrieved from Strip tho...
    strip->setSubFieldOrder(order);
    outs.push_back(strip);

    auto artnetOut = std::make_shared<ArtnetOut>(startUni + 10, numPorts, artnet);
    artnetOut->setTargetFreq(40);
    outs.push_back(artnetOut);
    lg.dbg("Done add outputs");

    renderer = std::make_shared<Renderer>("Renderer", 40, 1000, *strip);
    renderer->addEffectManager(new Functions("Strip", *renderer)); // welll eh myeh...

    // connect artnetOut to renderer result...
    // should have a cleaner way to multi-setup second output for something
    // even if renderer always has its original target as "master"
    artnetOut->setBuffer(renderer->getDestination(0), 0); //fwd calculated packets...
    artnetOut->setBuffer(artnetIn->buffer(0), 1); //fwd incoming
    lg.dbg("Done create renderer");

    renderer->start();
    DEBUG("DONE");
  }

  // only thing need from this style going ahead is the mult-srcs support
  // (obviously essential for local generators)
  // so flesh out Patch obj more better and yea.
  // void handleInput() {
  //   lwd.stamp(PIXTOL_SCHEDULER_INPUT_DMX);
  //   bool blank = true;
  //   auto begin = micros();
  //   auto fChans = renderer->f->numChannels;

  //   for(auto& src: ins->tasks()) {

  //     auto* source = static_cast<Inputter*>(src.get());
  //     if(!source->dirty()) continue; // source's responsibility not set newData until all frames in (esp if sync used)

  //     for(size_t i = 0; i < source->buffers().size(); i++) {
  //       auto& b = source->buffer(i); // be careful. was creating a copy when just auto no &... well no shit?
  //       if(!b.dirty()) continue;

  //       bool blendAll = !controlsSource.length(); //still bit confusing
  //       if((controlsSource == source->id() || blendAll) && (i == 0)) { //pref matches, only first chan used.
  //         renderer->updateControls(b, blendAll);
  //       }
  //       // auto& strip = static_cast<Strip&>((*outs)[i]);
  //       // auto offsetBuffer = Buffer(*b, strip.fieldCount(), fChans, false);
  //       // offsetBuffer.setFieldSize(strip.fieldSize()); // ^ clone target-buffer and then just adjust its offset instead. fugly.

  //       if(blank) { // so, need more stuff to splice which seems reasonable?  or rather, need Patch.  Tho given mult outs in this case dunno whether eg 2 renderers x 240px or all in one makes more sense?
  //         // renderer->updateTarget(offsetBuffer, i);
  //         blank = false; // doesn't hold up with multiple buffers.
  //       } else {
  //         int blendMode = 4; //used to come from iot but decoupled
  //         // renderer->updateTarget(offsetBuffer, i, blendMode, true);
  //       }

  //       b.setDirty(false); // consumed
  //     }
  //   }

  //   if(false) { // } else if(all sources timed out) {
  //     lg.dbg("No input for 5 seconds, running fallback...");
  //     // auto model = (Outputter&)(*outs)[0];
  //     // ins->add(new Generator("Fallback animation", model)); // what will be
  //     progressMultiplier = 0.001f;
  //   }
  //   timeInputs += micros() - begin;
  // }

  // void tick() override { _run(); }

  // bool _run() override {
  //   return true;
  // }
};

}
