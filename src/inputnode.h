#ifndef PIXTOL_INPUTNODE
#define PIXTOL_INPUTNODE

#include <Homie.h>
#include <LoggerNode.h>
// #include <ArtnetnodeWifi.h>
// #include "config.h"

// extern ArtnetnodeWifi artnet;


class InputNode: public HomieNode {
 public:
    // InputNode();
    void initArtnet(const String& name);

 protected:
    virtual bool handleInput(const String& property, const HomieRange& range, const String& value);
    virtual void setup() override;
    // virtual void loop(); // relevant?
  
//  private:
};

#endif

