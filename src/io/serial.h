#pragma once

// serial commands of some sort, prob mirroring mqtt?
// useful stuff:
// To detect an unknown baudrate of data coming into Serial use
// Serial.detectBaudrate(time_t timeoutMillis).
// This method tries to detect the baudrate for a maximum of timeoutMillis ms.
// It returns zero if no baudrate was detected, or the detected baudrate otherwise.
// The detectBaudrate() function may be called before Serial.begin() is called,
// because it does not need the receive buffer nor the SerialConfig parameters.
// The detection itself does not change the baudrate, after detection it should
// be set as usual using Serial.begin(detectedBaudrate).
//
// Serial uses UART0, which is mapped to pins GPIO1 (TX) and GPIO3 (RX).
// Serial may be remapped to GPIO15 (TX) and GPIO13 (RX) by calling
// Serial.swap() after Serial.begin. Calling swap again maps UART0 back to GPIO1 and GPIO3.

// Serial1 uses UART1, TX pin is GPIO2.
// UART1 can not be used to receive data because normally its RX pin is
// occupied for flash chip connection. To use Serial1, call Serial1.begin(baudrate).

// If Serial1 is not used and Serial is not swapped - TX for UART0 can be
// mapped to GPIO2 instead by calling Serial.set_tx(2) after Serial.begin or
// directly with Serial.begin(baud, config, mode, 2).
//
// By default the diagnostic output from WiFi libraries is disabled when you
// call Serial.begin. To enable debug output again, call
// Serial.setDebugOutput(true). To redirect debug output to Serial1 instead,
// call Serial1.setDebugOutput(true).
