/*
 * digitalIoPin.h
 *
 *  Created on: 7 Sep 2022
 *      Author: aivua
 */

#ifndef DIGITALIOPIN_H_
#define DIGITALIOPIN_H_

class DigitalIoPin {
public:
	/*
	 * Boolean:
	 * True: input, enable pull-up, invert (only for input)
	 * False: output, enable pull-down, normal (only for input)
	 */
	DigitalIoPin(int port, int pin, bool input = true, bool pullup = true, bool invert = false);
	// Makes object non-copyable
	DigitalIoPin(const DigitalIoPin &) = delete;
	virtual ~DigitalIoPin();
	// Reads a pin's state
	bool read();
	// Sets a pin's state (no effect on input pin)
	void write(bool value);
private:
	 // add these as needed
	int port;
	int pin;
	bool input;
	bool pullup;
	bool invert;
};

#endif /* DIGITALIOPIN_H_ */
