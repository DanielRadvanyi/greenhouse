/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include <stdio.h>
#include <atomic>
using namespace std;

#include "SimpleMenu.h"
#include "IntegerEdit.h"
#include "DecimalEdit.h"
#include "ITM_print.h"

static volatile atomic_int counter;
#ifdef __cplusplus
extern "C" {
#endif

void SysTick_Handler(void) {
	if(counter > 0) counter--;
}
#ifdef __cplusplus
}
#endif

void Sleep(int ms);

int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, false);
#endif
#endif
	/* Enable and setup SysTick Timer at a periodic rate */
	//SysTick_Config(SystemCoreClock / 1000);
	SysTick_Config(Chip_Clock_GetSysTickClockRate() / 1000);

    Chip_RIT_Init(LPC_RITIMER);
    //NVIC_EnableIRQ(RITIMER_IRQn);

    DigitalIoPin *rs = new DigitalIoPin(0, 8, false, false, false);
	DigitalIoPin *en = new DigitalIoPin(1, 6, false, false, false);
	DigitalIoPin *d4 = new DigitalIoPin(1, 8, false, false, false);
	DigitalIoPin *d5 = new DigitalIoPin(0, 5, false, false, false);
	DigitalIoPin *d6 = new DigitalIoPin(0, 6, false, false, false);
	DigitalIoPin *d7 = new DigitalIoPin(0, 7, false, false, false);

	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);

	// 3 buttons: up, down, ok
	DigitalIoPin b1(0, 17, true, true, true);
	DigitalIoPin b2(1, 11, true, true, true);
	DigitalIoPin b3(1, 9, true, true, true);

	bool b1Pressed = false;
	bool b2Pressed = false;
	bool b3Pressed = false;

	DigitalIoPin red(0, 25, false, false, false);
	DigitalIoPin green(0, 3, false, false, false);
	DigitalIoPin blue(1, 1, false, false, false);

    SimpleMenu menu; // this could also be allocated from the heap

    DecimalEdit *time = new DecimalEdit(lcd, std::string("Time"), 0, 200, 20);
    DecimalEdit *blank = new DecimalEdit(lcd, std::string("Blank"), 0, 1, 0.1);
    IntegerEdit *light = new IntegerEdit(lcd, std::string("Light"), 0, 3, 1);

    PropertyEdit* menuItem = nullptr;

    ITM_print p;

    int sleepCounter = 0;
    int ledOnTimer = 0;
    int ledOffTimer = 0;

    bool ledOn = false;

    menu.addItem(new MenuItem(time));
    menu.addItem(new MenuItem(blank));
    menu.addItem(new MenuItem(light));

    // Set up stage
    p.print("INITIAL MENU");
    time->setValue(0);
	p.print("\nTime: 0");
	blank->setValue(0);
	p.print("\nBlank: 0");
	light->setValue(0);
	p.print("\nLight: 0");


	lcd->begin(16,2);
	lcd->setCursor(0,0);
	menu.event(MenuItem::show);

	while(1) {

		// press a button -> send an event to menu handler
		if (menuItem != nullptr) {
			if (b1.read()) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				//menuItem->increment();
				//menuItem->display();
				menu.event(MenuItem::up);
				p.print("UP VAL\n");
			}

			if (b2.read()) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				//menuItem->decrement();
				//menuItem->display();
				menu.event(MenuItem::down);
				p.print("DOWN VAL\n");
			}

			if (b3.read()) {
				b3Pressed = true;
			} else if (b3Pressed == true) {
				b3Pressed = false;
				menu.event(MenuItem::ok);
				//menuItem->accept();
				//menuItem->display();
				p.print("\nOK VALUE\n");

				// print menu with values
				p.print("\nMENU\n");
				p.print("\nTime: ");
				p.print(time->getValue());
				p.print("\nBlank: ");
				p.print(blank->getValue());
				p.print("\nLight: ");
				p.print(light->getValue());
				p.print("\n");

				menu.event(MenuItem::back);
				//menu.event(MenuItem::show);
				menuItem = nullptr;

			}
		} else {
			if (b1.read()) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				menu.event(MenuItem::down);
			}

			if (b2.read()) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				menu.event(MenuItem::up);
			}

			if (b3.read()) {
				b3Pressed = true;
			} else if (b3Pressed == true) {
				b3Pressed = false;
				menu.event(MenuItem::ok);
				//menu.event(MenuItem::back);
				menuItem != nullptr;
				int menuItemId = menu.getPosition();
				p.print("\nOK ITEM\n");
				if (menuItemId == 0) {
					menuItem = time;
				} else if (menuItemId == 1) {
					menuItem = blank;
				} else if (menuItemId == 2) {
					menuItem = light;
				}
			}
		}

		// reset counter
		if (b1.read() || b2.read() || b3.read()) {
			sleepCounter = 0;
		}

		// no button is pressed for 10 seconds -> call back event
		if (sleepCounter >= 10000) {
			b1Pressed = false;
			b2Pressed = false;
			b3Pressed = false;

			menu.event(MenuItem::back);

			p.print("\nBACK");

			// cancel setting value of the item
			//menuItem->cancel();
			menuItem = nullptr;
		}

		/*
		 * Blinking
		 * Time = 200, Blank = 0.3, led = 3 → blue led blinks (200 ms ON, 300 ms OFF, 200 ms ON, …)
		 * Check which light should be on and how
		 */

		double timeVal = time->getDoubleValue();
		double blankVal = blank->getDoubleValue();
		string lightVal = light->getValue();

		// when led on enough time -> set off
		if (ledOn && ledOnTimer >= timeVal && blankVal > 0) {
			ledOn = false;
			ledOffTimer = 0;
		} else if (!ledOn && ledOffTimer >= (blankVal * 100) && timeVal > 0) {
			ledOn = true;
			ledOnTimer = 0;
		}
		// red, rest off
		if (lightVal == "1") {
			red.write(!ledOn);
			green.write(true);
			blue.write(true);

		// green
		} else if (lightVal == "2") {
			green.write(!ledOn);
			red.write(true);
			blue.write(true);

		// blue
		} else if (lightVal == "3") {
			blue.write(!ledOn);
			green.write(true);
			red.write(true);

		// all leds off
		} else {
			red.write(true);
			green.write(true);
			blue.write(true);
		}

		Sleep(10);
		sleepCounter +=10;
		ledOnTimer +=10;
		ledOffTimer +=10;
	}

	Board_LED_Set(3, false);

	return 0;
}

void Sleep(int ms) {
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}
