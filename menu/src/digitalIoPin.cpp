/*
 * digitalIoPin.cpp
 *
 *  Created on: 7 Sep 2022
 *      Author: aivua
 */

#include "digitalIoPin.h"
#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

DigitalIoPin::DigitalIoPin(int port, int pin, bool input, bool pullup, bool invert) {
	// TODO Auto-generated constructor stub
	this->port = port;
	this->pin = pin;
	this->input = input;
	this->pullup = pullup;
	this->invert = invert;

	// Configure buttons as pull-up inputs and LEDs as outputs
	if (this->input && this->pullup && this->invert) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, this->port, this->pin, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN | IOCON_INV_EN));
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, this->port, this->pin);
	} else if (!this->input) {
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, this->port, this->pin);
	}
}

DigitalIoPin::~DigitalIoPin() {
	// TODO Auto-generated destructor stub
}

// Reads a pin's state
bool DigitalIoPin::read() {
	return Chip_GPIO_GetPinState(LPC_GPIO, this->port, this->pin);
}

// Sets a pin's state (no effect on input pin)
void DigitalIoPin::write(bool value) {
	// Turn on LEDs
	if (!this->input) {
		Chip_GPIO_SetPinState(LPC_GPIO, this->port, this->pin, value);
	}
}


