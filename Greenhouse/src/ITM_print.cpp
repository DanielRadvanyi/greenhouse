/*
 * ITM_print.cpp
 *
 *  Created on: 9 Sep 2022
 *      Author: aivua
 */

#include "ITM_print.h"
#include "ITM_write.h"
#include <stdio.h>
#include <string>

ITM_print::ITM_print() {
	// TODO Auto-generated constructor stub
	ITM_init();
}

ITM_print::~ITM_print() {
	// TODO Auto-generated destructor stub
}

void ITM_print::print(int num) {
	std::string s = std::to_string(num);
	const char *str = s.c_str();
	ITM_write(str);
	printf("%s", str);
}

void ITM_print::print(const char *str) {
	ITM_write(str);
	printf("%s", str);
}

void ITM_print::print(std::string string) {
	const char *str = string.c_str();
	ITM_write(str);
	printf("%s", str);
}

