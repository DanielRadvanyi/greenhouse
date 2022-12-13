/*
 * IntegerEdit.h
 *
 *  Created on: 2.2.2016
 *      Author: krl
 */

#ifndef INTEGEREDIT_H_
#define INTEGEREDIT_H_

#include "PropertyEdit.h"
#include "LiquidCrystal.h"
#include <string>

class IntegerEdit: public PropertyEdit {
public:
	IntegerEdit(LiquidCrystal *lcd_, std::string editTitle, int min, int max, int step);
	virtual ~IntegerEdit();
	void increment() override;
	void decrement() override;
	void accept() override;
	void cancel() override;
	void setFocus(bool focus) override;
	bool getFocus() override;
	void display() override;
	std::string getValue() override;
	void setValue(int value);
	std::string getTitle() override;
private:
	void save();
	void displayEditValue();
	LiquidCrystal *lcd;
	std::string title;
	int value;
	int edit;
	bool focus;
	int min;
	int max;
	int step;
};

#endif /* INTEGEREDIT_H_ */
