#ifndef PIXTOL_BATTERY_H_
#define PIXTOL_BATTERY_H_

#include <Homie.hpp>
#include <LoggerNode.h>
// #include <Ticker.h>

class BatteryNode: public HomieNode {
public:
  BatteryNode(uint16_t readInterval = 10000, int cutoffLevel = 500);
  void setup();
  void loop();

  uint16_t getDrainRate();
  uint16_t getTimeRemaining();

  bool safe;

private:
  uint16_t firstCharge, lastCharge, rollingAverageCharge;
  uint16_t cutoff;

  unsigned long timeCounter;
  uint16_t interval;

  void update();

};

BatteryNode::BatteryNode(uint16_t readInterval, int cutoffLevel): 
  HomieNode("Battery", "charge"),
  interval(readInterval),
  cutoff(cutoffLevel)
{
  timeCounter = millis();
  firstCharge = analogRead(A0);
  lastCharge = firstCharge;
  rollingAverageCharge = lastCharge;

  safe = firstCharge > cutoff;
}

void BatteryNode::setup() {
  advertise("level");
  advertise("cutoff").settable();
  advertise("safe");
  // batteryReadTimer.attach_ms(10000, BatteryNode.update())
}

void BatteryNode::loop() {
  if(millis() - timeCounter < interval) return;
  timeCounter = millis();
  update();

}

void BatteryNode::update() {
  lastCharge = analogRead(A0);
  rollingAverageCharge = (rollingAverageCharge*10 + lastCharge) / 11;
  // LN.logf("Battery", LoggerNode::DEBUG, "Charge: last %d / average %d", lastCharge, rollingAverageCharge);
  setProperty("level").send(String(rollingAverageCharge));

  if(lastCharge < cutoff-100 || rollingAverageCharge < cutoff) {
    LN.logf("Battery", LoggerNode::ERROR, "Charge too low! %d", rollingAverageCharge);
    safe = false;
    // then if remains there for a few minutes, shut off stuff using battery.
    // But guess that'd be done elsewhere after polling this object? Rather than passing
    // callback or some shite.
  } else {
    LN.logf("Battery", LoggerNode::INFO, "Charge above cutoff! %d", rollingAverageCharge);
    safe = true;
  }
  setProperty("safe").send(String(safe));
}


#endif
