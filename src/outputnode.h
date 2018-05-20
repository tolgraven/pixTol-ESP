#ifndef PIXTOL_OUTPUTNODE
#define PIXTOL_OUTPUTNODE

#include <Homie.h>
#include <LoggerNode.h>


class OutputNode: public HomieNode {
 public:
    // OutputNode();

 protected:
    virtual bool handleInput(const String& property, const HomieRange& range, const String& value);
    virtual void setup() override;
    // virtual void loop(); // relevant?
  
//  private:
};

#endif
