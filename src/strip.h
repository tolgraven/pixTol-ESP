#ifndef PIXTOL_STRIP
#define PIXTOL_STRIP

#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <Homie.h>

typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>         DmaGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>         DmaGRBW;
typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266AsyncUart800KbpsMethod>   UartGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod>   UartGRBW;
typedef NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266BitBang800KbpsMethod>     BitBangGRB;
typedef NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>     BitBangGRBW;


// class Strip: public Outputter {
class Strip {
  public:
    enum PixelBytes { Invalid = 0, Single = 1, RGB = 3, RGBW = 4, RGBWAU = 6 };
    Strip(PixelBytes type = RGBW, uint16_t ledCount = 120); // + something for method, pin etc...


    // later: templates for neopixelbus and rgb/rgbw, then further abstractions so can also switch backing lib etc...
    // template <class T>

    // need own implementations right? for index to make sense. get pixelbuffer and use getPixelIndex for pixel +/- whatever
    void rotatePixels(int amount); // per-pixel or percentage-based? neg numbers for backwards
    void rotateHue(float degrees); // -360 to 360
      

    // this gets called by onDmxFrame or equivalent... maybe skip bools and just pass nil if not updating either one?
    void keyFrame(uint8_t* pixelData, uint8_t* functionData, bool updatePixels = true, bool updateFunctions = true);

  private:
    // DmaGRB *bus;      // NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>       *bus;	// this way we can (re!)init off config values, post boot
    DmaGRBW *bus;      // NeoPixelBrightnessBus<NeoGrbFeature,  NeoEsp8266Dma800KbpsMethod>       *bus;	// this way we can (re!)init off config values, post boot
    // DmaGRBW *busW;    // NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod>       *busW;
    // UartGRBW *busW2;  // NeoPixelBrightnessBus<NeoGrbwFeature, NeoEsp8266AsyncUart800KbpsMethod> *busW2;

    int getPixelIndex(int pixel);
    int getPixelFromIndex(int idx);

    void renderInterFrame();
    void interCallback();
    void updatePixels(uint8_t* data);
    void updateFunctions(uint8_t* functions, bool isKeyframe);

    uint8_t bytes;
    uint8_t ledsInData;
    uint8_t ledsInStrip;

    HomieSetting<bool> setGammaCorrect;
    NeoGamma<NeoGammaTableMethod> *colorGamma;
    HomieSetting<bool> setMirrored;             
    HomieSetting<bool> setFolded; 
    HomieSetting<bool> setFlipped;
    bool flipped;

    uint8_t keyFrameRate; // off eg dmx hz or manual anim update rate
    uint8_t targetFrameRate; // calculate from ledCount etc

    Ticker interFrameTimer;
    uint8_t interCounter;
    uint8_t interFrameCount = 3; // ~40 us/led gives 5 ms for 1 universe of 125 leds. so around 4-5 frames per frame should be alright.
    float blendBaseline = 1.00f - 1.00f / (1 + interFrameCount);

    uint8_t brightness, lastBrightness;

    uint8_t* lastData;
    uint8_t* lastFunctions;

    uint8_t iteration = 0;
    // int8_t subPixelNoise[125][4] = {0};
    int8_t subPixelNoise[125][4];

  protected:
    // virtual void setup() override;
    // virtual void loop() override;
};


class Color {

};

#endif
