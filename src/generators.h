#pragma once

#include "renderstage.h"
#include "color.h"

// Basic color and gradient generator...
// might be better as more an "action" on an arbitrary Buffer than something fixed with own Buffer of spec size n shit, dunno
// Then again with resizing+patching, two pixels all you need for fill+gradient heh...
class ColorGen: public Generator {
  public:
  ColorGen(const std::string& id, uint8_t fieldSize, uint16_t fieldCount):
    // Generator(id, fieldSize, fieldCount), pixels(static_cast<PixelBuffer*>(&get())),
    Generator(id, fieldSize, fieldCount)
    // pixels(new PixelBuffer(get())),
    /* hsl(new HslColor(0, 50, 50)) { */
     {
  }
  // std::map<uint8_t*, std::function... //thinking like mapping offsets to functions hmm
  // but here'd be much like fn channels no?
  // make a layout, say:
  // 0 gain
  // 1 h
  // 2 s
  // 3 l
  // 4 a
  // 5 attack
  // 6 release
  enum InputMap { Gain = 0, H, S, L, A, Attack, Release };
  bool run() { //act on data in controls
    hsl->h = input->get(H);
    hsl->s = input->get(S);
    hsl->l = input->get(L);

  }

  // PixelBuffer* pixels; //just mirrors existing buffer... but wait creating as a Buffer we'd need dynamic cast for PixelBuffer no?
  /* HslColor* hsl; */
  Color color;
  private:

};


// use for on-chip generated stuff. StatusLed/Ring breathing, strip standby and simple animations...
// and random set/fill colors prob pass through here?
// again, maybe Generator should be thing, transforming Inputter ctrl data to cool shit passed back to it
//                             Output
//                               ^
// or oh yeah maybe. ctrl -> Inputter -> Generator
//                                ^   <- puts anim
// should this maybe be a HomieNode btw? so dont gotta pass...
// start off by basically equally shitty Blinky, but with Buffer target instead of Strip...
class LocalAnimation: public Generator {
  public:
  LocalAnimation(const std::string& id, uint8_t bitDepth, uint16_t pixels):
    Generator(id, bitDepth, pixels) { }

  bool run() {
    // if animation registered, advance it appropriately. if "static" animation + pinned source,
    // simply refresh (if sharing buffer and has been overwritten? but seems unlikely)
    // or simply... nothing
    // start by using NeoPixelAnimator prob...
  }
  std::map<std::string, RgbwColor> colors;
  void generatePalette() {
    colors["black"]  = RgbwColor(0, 0, 0, 0);
    colors["white"]  = RgbwColor(150, 150, 150, 255);
    colors["red"]    = RgbwColor(255, 15, 5, 8);
    colors["orange"] = RgbwColor(255, 40, 20, 35);
    colors["yellow"] = RgbwColor(255, 112, 12, 30);
    colors["green"]  = RgbwColor(20, 255, 22, 35);
    colors["blue"]   = RgbwColor(37, 85, 255, 32);
  }
  bool color(const std::string& name = "black") {
    if(colors.find(name) != colors.end()) {
      lg.dbg("Set color: " + name);
      uint8_t c[4];
      c[0] = colors[name].R;
      c[1] = colors[name].G;
      c[2] = colors[name].B;
      c[3] = colors[name].W;
      for(auto t=0; t<sizeInBytes(); t+=fieldSize()) {
        get().set(c, 4, 0, t);
      }
      // get().fill(colors[name]); //soon!
      // s->setColor(colors[name]);
      return true;
    }
    return false;
  }

  void gradient(const std::string& from = "white", const std::string& to = "black") {
    lg.dbg(from + "<>" + to + " gradient requested");
    RgbwColor* one = colors.find(from) != colors.end()? &colors[from]: colors["white"]; //;
    RgbwColor* two = colors.find(to)   != colors.end()? &colors[to]:   colors["black"]; //;
    s->setGradient(one, two);
  }

  void test() {
    lg.logf("Blinky test", Log::DEBUG, "Run gradient test");
    color("black"); //homieDelay(50); homiedelay (and also reg delay? causing crash :/)
    gradient("black", "blue"); //homieDelay(300);
    color("blue"); //homieDelay(100);
    gradient("blue", "green"); //homieDelay(300);
    gradient("green", "red"); //homieDelay(300);
    gradient("red", "orange"); //homieDelay(300);
    gradient("orange", "black");
  }

  void blink(const std::string& colorName, uint8_t blinks = 1) {
    for(int8_t b = 0; b < blinks; b++) {
      color(colorName);
      // homieDelay(100);
      color("black");
      // homieDelay(50);
    }
  }

  void fadeRealNiceLike() { //proper, worthy, impl of above...
    // scheduler->registerAnimation(lala); //nahmean
  }
};
