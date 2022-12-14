/*
 * NoEdit.h
 */

#include "PropertyEdit.h"
#include "LiquidCrystal.h"
#include <string>

class NoEdit: public PropertyEdit{
public:
	NoEdit(LiquidCrystal *lcd_, std::string editTitle);
	virtual ~NoEdit();
	void increment() override;
	void decrement() override;
	void accept() override;
	void cancel() override;
	void setFocus(bool focus) override;
	bool getFocus() override;
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
	bool focus;
};
