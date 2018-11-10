#pragma once

#include <Homie.h>
// special crap because (mqtt lib in?) Homie don't deal well with this.
// not that delay is cool or good to use...
void homieYield();
void homieDelay(unsigned long ms);

