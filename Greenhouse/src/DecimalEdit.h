/*
 * DecimalEdit.h
 *
 *  Created on: 12 Oct 2022
 *      Author: aivua
 */

#ifndef DECIMALEDIT_H_
#define DECIMALEDIT_H_

#include "PropertyEdit.h"
#include "LiquidCrystal.h"
#include <string>

class DecimalEdit: public PropertyEdit{
public:
	DecimalEdit(LiquidCrystal *lcd_, std::string editTitle, int min, int max, double step);
	virtual ~DecimalEdit();
	void increment() override;
	void decrement() override;
	void accept() override;
	void cancel() override;
	void setFocus(bool focus) override;
	bool getFocus() override;
	float getValueF();
	void display() override;
	std::string getValue() override;
	void setValue(float value);
	std::string getTitle() override;
	double getDoubleValue();
private:
	void save();
	void displayEditValue();
	LiquidCrystal *lcd;
	std::string title;
	double value;
	double edit;
	bool focus;
	int min;
	int max;
	double step;
};

#endif /* DECIMALEDIT_H_ */
