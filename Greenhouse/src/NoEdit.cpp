/*
 * NoEdit.cpp
 */

#include "NoEdit.h"
#include <cstdio>
#include <cmath>

NoEdit::NoEdit(LiquidCrystal *lcd_, std::string editTitle): lcd(lcd_), title(editTitle) {
	value = 0;
	focus = false;
}

void NoEdit::increment() {
}

void NoEdit::decrement() {
}

void NoEdit::accept() {
}

void NoEdit::cancel() {
}


void NoEdit::setFocus(bool focus) {
	this->focus = focus;
}

bool NoEdit::getFocus() {
	return this->focus;
}

void NoEdit::display() {
	lcd->clear();
	lcd->setCursor(0,0);
	lcd->print(title);
	lcd->setCursor(0,1);
	char s[32];
	/*
	if(focus) {
		snprintf(s, 32, "     [%4f]     ", value);
	}
	else {
		snprintf(s, 32, "      %4f      ", value);
	} */
	snprintf(s, 32, "     %4f     ", value);
	lcd->print(s);
}


void NoEdit::save() {
}


std::string NoEdit::getValue() {
	string s = to_string(value);
	return s.substr(0, s.find(".") + 2);
}
void NoEdit::setValue(float value) {
	value = value;
}

double NoEdit::getDoubleValue() {
	return (std::ceil(value * 10.0) / 10.0);
}

std::string NoEdit::getTitle() {
	return title;
}

NoEdit::~NoEdit() {
	// TODO Auto-generated destructor stub
}

