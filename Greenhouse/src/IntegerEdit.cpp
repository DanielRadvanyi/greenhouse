/*
 * IntegerEdit.cpp
 *
 *  Created on: 2.2.2016
 *      Author: krl
 */

#include "IntegerEdit.h"
#include <cstdio>

IntegerEdit::IntegerEdit(LiquidCrystal *lcd_, std::string editTitle, int min, int max, int step): lcd(lcd_), title(editTitle) {
	value = 0;
	edit = 0;
	focus = false;
	this->min = min;
	this->max = max;
	this->step = step;
}

IntegerEdit::~IntegerEdit() {
}

void IntegerEdit::increment() {

	if (edit <= (max - step)) {
		edit += step;
	}

}

void IntegerEdit::decrement() {

	if (edit >= (min + step)) {
		edit -= step;
	}

}

void IntegerEdit::accept() {
	save();
}

void IntegerEdit::cancel() {
	edit = value;
}


void IntegerEdit::setFocus(bool focus) {
	this->focus = focus;
}

bool IntegerEdit::getFocus() {
	return this->focus;
}

void IntegerEdit::display() {
	lcd->clear();
	lcd->setCursor(0,0);
	lcd->print(title);
	lcd->setCursor(0,1);
	char s[17];
	if(focus) {
		snprintf(s, 17, "     [%4d]     ", edit);
	}
	else {
		snprintf(s, 17, "      %4d      ", edit);
	}
	lcd->print(s);
}


void IntegerEdit::save() {
	// set current value to be same as edit value
	value = edit;
	// todo: save current value for example to EEPROM for permanent storage
}


std::string IntegerEdit::getValue() {
	return to_string(value);
}
void IntegerEdit::setValue(int value) {
	edit = value;
	save();
}

std::string IntegerEdit::getTitle() {
	return title;
}
