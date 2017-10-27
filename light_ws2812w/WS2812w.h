/*
* light weight WS2812 lib V2.1 - Arduino support
*
* Controls WS2811/WS2812/WS2812B RGB-LEDs
* Author: Matthias Riegler
*
* Mar 07 2014: Added Arduino and C++ Library
*
* September 6, 2014      Added option to switch between most popular color orders
*                          (RGB, GRB, and BRG) --  Windell H. Oskay
* 
* January 24, 2015       Added option to make color orders static again
*                        Moved cRGB to a header so it is easier to replace / expand 
*                              (added SetHSV based on code from kasperkamperman.com)
*                                              -- Freezy
*
* License: GNU GPL v2 (see License.txt)
*/

#ifndef WS2812W_H_
#define WS2812W_H_

// #include <avr/interrupt.h>
// #include <avr/io.h>
#ifndef F_CPU
#define  F_CPU 8000000UL
#endif
// #include <util/delay.h>
#include <stdint.h>

#ifdef ARDUINO
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif
#endif

//Easier to change cRGB into any other rgb struct
#include "cRGBW.h"

// If you want to use the setColorOrder functions, enable this line
#define RGB_ORDER_ON_RUNTIME

#ifdef RGB_ORDER_ON_RUNTIME	
	#define OFFSET_R(r) r+offsetRed
	#define OFFSET_G(g) g+offsetGreen
	#define OFFSET_B(b) b+offsetBlue
	#define OFFSET_W(w) w+offsetWhite
#else
// CHANGE YOUR STATIC RGB ORDER HERE
	#define OFFSET_R(r) r
	#define OFFSET_G(g) g+1	
	#define OFFSET_B(b) b+2
	#define OFFSET_W(w) w+3	
#endif

class WS2812W {
public: 
	WS2812W(uint16_t num_led);
	~WS2812W();
	
	#ifndef ARDUINO
	void setOutput(const volatile uint8_t* port, volatile uint8_t* reg, uint8_t pin);
	#else
	void setOutput(uint8_t pin);
	#endif
	
	cRGB get_crgb_at(uint16_t index);
	uint8_t set_crgb_at(uint16_t index, cRGB px_value);
	uint8_t set_subpixel_at(uint16_t index, uint8_t offset, uint8_t px_value);

	void sync();
	
#ifdef RGB_ORDER_ON_RUNTIME	
	void setColorOrderRGB();
	void setColorOrderGRB();
	void setColorOrderBRG();
	void setColorOrderGRBW();
	void setColorOrderRGBW();
#endif

private:
	uint16_t count_led;
	uint8_t *pixels;

#ifdef RGB_ORDER_ON_RUNTIME	
	uint8_t offsetRed;
	uint8_t offsetGreen;
	uint8_t offsetBlue;
	uint8_t offsetWhite;
#endif	

	void ws2812w_sendarray_mask(uint8_t *array,uint16_t length, uint8_t pinmask,uint8_t *port, uint8_t *portreg);

	const volatile uint8_t *ws2812w_port;
	volatile uint8_t *ws2812w_port_reg;
	uint8_t pinMask; 
};



#endif /* WS2812W_H_ */
