/*
 * ITM_print.h
 *
 *  Created on: 9 Sep 2022
 *      Author: aivua
 */
#include <string>

#ifndef ITM_PRINT_H_
#define ITM_PRINT_H_

class ITM_print {
public:
	ITM_print();
	virtual ~ITM_print();
	// Prints integer / c string / c++ string
	void print(int num);
	void print(const char *str);
	void print(std::string string);
};

#endif /* ITM_PRINT_H_ */
