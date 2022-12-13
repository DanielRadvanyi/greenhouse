/*
 * DecimalEdit.cpp
 *
 *  Created on: 12 Oct 2022
 *      Author: aivua
 */

#include "DecimalEdit.h"
#include <cstdio>
#include <cmath>

DecimalEdit::DecimalEdit(LiquidCrystal *lcd_, std::string editTitle, int min, int max, double step): lcd(lcd_), title(editTitle) {
	value = 0;
	edit = 0;
	focus = false;
	this->min = min;
	this->max = max;
	this->step = step;
}

void DecimalEdit::increment() {

	if (edit <= (max - step)) {
		edit += step;
	}

}

void DecimalEdit::decrement() {

	if (edit >= (min + step)) {
		edit -= step;
	}

}

void DecimalEdit::accept() {
	save();
}

void DecimalEdit::cancel() {
	edit = value;
}


void DecimalEdit::setFocus(bool focus) {
	this->focus = focus;
}

bool DecimalEdit::getFocus() {
	return this->focus;
}

void DecimalEdit::display() {
	lcd->clear();
	lcd->setCursor(0,0);
	lcd->print(title);
	lcd->setCursor(0,1);
	char s[17];
	if(focus) {
		snprintf(s, 17, "     [%4f]     ", std::ceil(edit * 10.0) / 10.0);
	}
	else {
		snprintf(s, 17, "      %4f      ", std::ceil(edit * 10.0) / 10.0);
	}
	lcd->print(s);
}


void DecimalEdit::save() {
	// set current value to be same as edit value
	value = std::ceil(edit * 10.0) / 10.0;
	// todo: save current value for example to EEPROM for permanent storage
}


std::string DecimalEdit::getValue() {
	string s = to_string(value);
	return s.substr(0, s.find(".") + 2);
}
void DecimalEdit::setValue(int value) {
	edit = value;
	save();
}

double DecimalEdit::getDoubleValue() {
	return (std::ceil(value * 10.0) / 10.0);
}

std::string DecimalEdit::getTitle() {
	return title;
}

DecimalEdit::~DecimalEdit() {
	// TODO Auto-generated destructor stub
}

