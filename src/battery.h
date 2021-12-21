#pragma once

/* #include <Homie.hpp> */
/* #include <LoggerNode.h> */
#include "log.h"

/* class BatteryNode: public HomieNode { */
class BatteryNode {
public:
  /* BatteryNode(): HomieNode("Battery", "charge") {} */
  BatteryNode() {}
  BatteryNode(uint16_t readInterval, int cutoffLevel):
    /* HomieNode("Battery", "charge"), */
    interval(readInterval),
    cutoff(cutoffLevel)
  {}

  void setup() {
    timeCounter = millis();

    lastCharge = analogRead(A0);
    rollingAverageCharge = lastCharge;
    status = lastCharge > cutoff? Safe: Danger;

    /* advertise("level"); */
    /* advertise("cutoff").settable(); */
    /* advertise("safe"); */
    lg.logf("Battery", tol::Log::INFO, "BatteryNode initialized.");
  }

  void loop() {
    if(millis() - timeCounter < interval) return;
    timeCounter = millis();
    update();
  }

  uint16_t getDrainRate();
  uint16_t getTimeRemaining();

  enum BatteryStatus { Invalid = -1, Danger = 0, Safe = 1 };
  BatteryStatus status = Invalid;

private:
  int lastCharge = -1;
  int rollingAverageCharge = -1;
  uint16_t interval = 10000;
  uint16_t cutoff = 500;

  unsigned long timeCounter = 0;
  BatteryStatus lastStatus = Invalid;

  void update() {
    lastCharge = analogRead(A0);
    rollingAverageCharge = (rollingAverageCharge*9 + lastCharge) / 10;

    if(lastCharge < cutoff-100 || rollingAverageCharge < cutoff) {
      if(lastStatus == Safe)
        lg.logf("Battery", tol::Log::ERROR, "Charge CRITICAL, %d, prepare to shutdown...", rollingAverageCharge);
      status = Danger; // then if remains there for a few minutes, shut off stuff using battery.
    } else {           // But guess that'd be done elsewhere after polling this object? Rather than passing callback or some shite.
      if(lastStatus == Danger)
        lg.logf("Battery", tol::Log::INFO, "Charge back above cutoff! %d", rollingAverageCharge);
      status = Safe;
    }
    /* setProperty("level").send(String(rollingAverageCharge)); */
    /* if(status != lastStatus) setProperty("safe").send(String(status)); */

    lastStatus = status;
  }
};
