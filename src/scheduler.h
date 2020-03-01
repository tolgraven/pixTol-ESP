#pragma once

#include <vector>
#include <memory>

#include "renderer.h"
#include "buffer.h"
#include "io/strip-multi.h"
#include "io/sacn.h"
#include "io/artnet.h"
#include "io/opc.h"
#include "functions.h"
#include "watchdog.h"
#include "base.h"
#include "task.h"


// make some of these actual tasks I guess, tho mainly seems workflow w eventbus and
// callbacks most reasonable.
class Scheduler: public Runnable, public core::Task  {
  TaskGroup<Inputter>  ins    = {"input", 8, 0};
  TaskGroup<Renderer>  render = {"render"};
  TaskGroup<Outputter> outs   = {"output", 25, 1};
  TaskGroup<Runnable> general = {"general", 1};
  std::vector<std::unique_ptr<TaskedRunnable>> scheduled; // no run() on these, OS handles. just keep so can kill...
  std::vector<std::unique_ptr<InterruptTask>> crap;

  String controlsSource = "artnet"; // "" // should just pick up whichever is actively sending (XXX set up "dont ignore 0 if prev pos input from src in session") */

  uint32_t now;
  uint32_t timeInputs = 0;
  float progressMultiplier = 0;

  public:
  Scheduler(const String& id):
    Runnable(id, "scheduler"),
    core::Task(id.c_str(), 9000, core::APPLICATION_BASE_PRIO, std::chrono::milliseconds{3}) {
    DEBUG("Enter");
    // Three "phases": Collect settings, use to create objects for and patches between IN / OUT / RENDER (tho in the end a sys with a Pi recompiling and reflashing units on fly yet more adaptable)
    }

  void init() override {
    const int startUni = 2;
    const int numPorts = 1;
    auto artnet = new ArtnetDriver(id() + " ArtNet");
    scheduled.emplace_back(new TaskedRunnable(artnet)); // ugly thing here is driver doesnt auto-delete on clearing stuff that uses it...


    auto artnetIn = new ArtnetIn(id(), startUni, numPorts, artnet);
    ins.add(artnetIn); // if nothings coming its because the multiple buffers overwrite first one.
    ins.add(new sAcnInput(id() + " sACN", 1, 1));
    ins.add(new OPC(id() + " OPC", 1, 1, 120));
    lg.dbg("Done add inputs"); //lg << "Done add inputs!" << endl;


    const int startPin = 15; // had at 25 but apparently some bad pins above that...
    auto strip = new Strip("Strip 1", 4, 120, startPin, numPorts);
    uint8_t order[] = {1, 0, 2, 3}; // should be set for then/retrieved from Strip tho...
    strip->setSubFieldOrder(order);
    outs.add(strip);

    auto artnetOut = new ArtnetOut(id(), startUni, numPorts, artnet);
    artnetOut->setTargetFreq(40);
    outs.add(artnetOut);
    lg.dbg("Done add outputs");

    render.add(new Renderer("Renderer", MILLION / 40, *strip));
    artnetOut->setBuffer(render.get().getDestination(), 0); //fwd calculated packets...
    // artnet->Outputter::setBuffer(*render.get(0).controls, 1); //fwd ctrls
    lg.dbg("Done create renderer");

    crap.emplace_back(new InterruptTask(20000, [artnetIn, artnetOut] {
          artnetIn->printTo(lg); artnetOut->printTo(lg); return true; }));
    crap.emplace_back(new InterruptTask(20000, [this] {
        lg.f("Runs", ::Log::INFO, "in / render / out %u %u %u \n",
              this->ins.getCounts().run, this->render.getCounts().run, this->outs.getCounts().run);
        return true; }));

    crap.emplace_back(new InterruptTask(10000, [this] {
          float tot = std::max((uint32_t)1, this->getCounts().totalTime); //haha well v dumb to still run all math every time lol
          lg.f("Timings", ::Log::INFO, "in / merge / render / out %.4f %.4f %.4f %.4f \n",
              this->ins.getCounts().totalTime / tot, this->timeInputs / tot, this->render.getCounts().totalTime / tot, this->outs.getCounts().totalTime / tot);
          return true; }));

    ins.start();
    render.start();
    outs.start();
    // general.start();

    DEBUG("DONE");
  }

  void handleInput() {
    lwd.stamp(PIXTOL_SCHEDULER_INPUT_DMX);
    bool blank = true;
    now = micros();

    for(auto&& src: ins.tasks()) {

      auto source = static_cast<Inputter*>(src.get());
      if(!source->dirty()) continue; // source's responsibility not set newData until all frames in (esp if sync used)

      for(size_t i = 0; i < source->buffers().size(); i++) {
        // heh this turned into a copy again watta! deleted cpy constructor now...
        // auto&& b = source->buffer(i); // be careful. was creating a copy when just auto no &...
        auto* b = &source->buffer(i); // be careful. was creating a copy when just auto no &...
        if(!b->dirty()) continue;
        // lg.f("merge", Log::DEBUG, "buffer %u, %x at %x, dirty: %u\n", i, b, b->get(), b->dirty());

        bool blendAll = !controlsSource.length(); //still bit confusing
        if((controlsSource == source->type() || blendAll)
            && (i == 0)) { //pref matches, only first chan used.
          // _device->debug->logDMXStatus(b->get(), "SOURCE");
          render.get(0).updateControls(*b, blendAll);
        }

        auto& strip = outs.get(i);
        auto offsetBuffer = Buffer(*b, strip.fieldCount(), render.get(0).f->numChannels, false);
        offsetBuffer.setFieldSize(strip.fieldSize()); // ^ clone target-buffer and then just adjust its offset instead. fugly.

        if(blank) { // so, need more stuff to splice which seems reasonable?  or rather, need Patch.  Tho given mult outs in this case dunno whether eg 2 renderers x 240px or all in one makes more sense?
          render.get(0).updateTarget(offsetBuffer, i);
          blank = false; // doesn't hold up with multiple buffers.
        } else {
          int blendMode = 4; //used to come from iot but decoupled
          render.get(0).updateTarget(offsetBuffer, i, blendMode, true);
        }

        b->setDirty(false); // consumed
      }
    }

    if(false) { // } else if(all sources timed out) {
      lg.dbg("No input for 5 seconds, running fallback...");
      ins.add(new Generator("Fallback animation", outs.get(0))); // what will be
      progressMultiplier = 0.001f;
    }
    timeInputs += micros() - now;
  }

  void tick() override { _run(); }

  bool _run() override {
    lwd.stamp(PIXTOL_SCHEDULER_ENTRY);
    handleInput();
    return true;

    // lwd.stamp(PIXTOL_RENDERER_FRAME);
    // ins.run(); render.run(); outs.run(); general.run();
    // all good. just bit pointless unless much more infra.
    // std::vector<std::function<void(Scheduler&)>> vec =
    //  { [] (Scheduler& s) { s.handleInput(); }, [] (Scheduler& s) { s.handleFallback(); }, [] (Scheduler& s) { s.render.get(0).frame(s.progressMultiplier); }, [] (Scheduler& s) { s.handleOutput(); }};

    // remember to be careful with auto and initializer lists :P
    // std::vector<std::pair<std::function<void(Scheduler*)>, uint32_t&>> vec =
    // {{[this] { this->handleInput(); }, timeInputs}, {[this] { this->handleFallback(); }}, // should not be here but trigger putting a Generator in Inputter vec, by lapsing timer maybe dunno {[this] { this->render.get(0).frame(this->progressMultiplier); }, timeRenders }, {[this] { this->handleOutput(); }}, timeOutputs }

    // for(auto&& f: vec) { // well and w renderer oopsien
    //   wrapper()
    //   // now = micros(); //, log before/after for each cat, much easier than now.
    //   // f(*this);
    //   // timeThing += micros() - now; // group their things so use right var for each.
    //   // lwd.stamp(EnumsForThese...);
    //   // update time, stamp di stamp, yada. common inbetween crap.
    // }
  }
};
