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

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "retarget_uart.h"

#include "ModbusRegister.h"
#include "DigitalIoPin.h"
#include "LiquidCrystal.h"

#include "SimpleMenu.h"
#include "DecimalEdit.h"

// Not used.
//#include "NoEdit.h"

// Added ITM print to print menu
#include "ITM_print.h"

// Mailbox (a queue with a single item)
QueueHandle_t mailboxHumidity;
QueueHandle_t mailboxCo2;


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

}

/* end runtime statictics collection */
static void idle_delay()
{
	vTaskDelay(1);
}

/* Read 2 sensors */
void taskReadSensor(void *params)
{
	(void) params;

	retarget_init();		// important for safe printf, also work with DEBUGOUT

	ModbusMaster node3(241); // Create modbus object that connects to slave id 241 (HMP60)
	node3.begin(9600); // all nodes must operate at the same speed!
	node3.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister RH(&node3, 256, true);

	/*
	ModbusMaster node4(241);
	node4.begin(9600); // all nodes must operate at the same speed!
	node4.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister TEMP(&node4, 256, true);
	*/

	idle_delay();

	ModbusMaster node5(240); // Create modbus object that connects to slave id 240
	node5.begin(9600); // all nodes must operate at the same speed!
	node5.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister CO2(&node5, 257, true);

	/*
	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);

	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);
	*/

	DigitalIoPin relay(0, 27, DigitalIoPin::output); // CO2 relay
	relay.write(0);

	while(true)
	{
		float rh, co2;
		//char buffer1[32], buffer3[32];

		rh = RH.read()/10.0;

		// Write rh to the humidity MAILBOX.
		// Third parameter 0 means don't wait for the queue to have space.
		if( !xQueueSend(mailboxHumidity, &rh, 0)) {
			printf("Failed to send Humidity data, queue full.\r\n");
		}

		idle_delay();

		co2 = CO2.read();

		// Write co2 to the co2 MAILBOX.
		// Third parameter 0 means don't wait for the queue to have space.
		if( !xQueueSend(mailboxCo2, &co2, 0)) {
			printf("Failed to send Humidity data, queue full.\r\n");
		}

		idle_delay();
	}

	idle_delay();
}

// New Task menu (2)
void taskMenu2(void *params) {
	(void) params;

	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);

	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);

	// 3 buttons: up, down, ok
	// TODO: check that how to initialise these
	DigitalIoPin b1(0, 17, DigitalIoPin::pullup);
	DigitalIoPin b2(1, 11, DigitalIoPin::pulldown);
	DigitalIoPin b3(1, 9, DigitalIoPin::input); // <- how to express OK(select/press)?

	bool b1Pressed = false;
	bool b2Pressed = false;
	bool b3Pressed = false;

    SimpleMenu menu;

    DecimalEdit *editHumidity = new DecimalEdit(lcd, std::string("Humidity"), 0, 1, 0.1);
    DecimalEdit *editCo2 = new DecimalEdit(lcd, std::string("C02"), 0, 1, 0.1);

    PropertyEdit* menuItem = nullptr;

    ITM_print p;

    int sleepCounter = 0;

    /* Main Menu has 2 menu items:
	 * 1. HUMIDITY
	 * * HUMIDITY Value will be updated from the mailbox.
	 * * LCD display will be updated every x seconds
	 * * If selected -> open edit mode
	 * 2. Co2
	 * * HUMIDITY Value will be updated from the mailbox.
	 * * LCD display will be updated every x seconds
	 * * If selected -> open edit mode
	 */

    menu.addItem(new MenuItem(editHumidity));
    menu.addItem(new MenuItem(editCo2));

    // Set up Menu with initial values 0.0
    p.print("INITIAL MENU");
    editHumidity->setValue(0.0);
	p.print("\nHumidity: 0.0");
	editCo2->setValue(0.0);
	p.print("\nC02: 0.0");

	lcd->begin(16,2);
	lcd->setCursor(0,0);
	menu.event(MenuItem::show);

	while(1) {
		// Read queue values into these variables
		// (In the example they use uint_32_t, but we want float values)
		float humidityData = 0.0;
		float co2Data = 0.0;
		// Read mailbox. 0 means don't wait for queue, return immediately if queue empty
		// True when a value is read and false when queue is empty
		bool humidityUpdated = xQueueReceive(mailboxHumidity, &humidityData, 0 );
		bool co2Updated = xQueueReceive(mailboxCo2, &co2Data, 0 );

		// press a button -> send an event to menu handler
		if (menuItem != nullptr) {
			// We have a menuItem selected
			// We are in edit property mode
			if (b1.read()) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				menu.event(MenuItem::up);
				p.print("UP VAL\n");
			}

			if (b2.read()) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				menu.event(MenuItem::down);
				p.print("DOWN VAL\n");
			}

			if (b3.read()) {
				b3Pressed = true;
			} else if (b3Pressed == true) {
				b3Pressed = false;
				menu.event(MenuItem::ok);
				p.print("\nOK VALUE\n");

				// Print menu
				p.print("MENU");
				p.print("\nHumidity:");
				p.print(editHumidity->getValue());
				p.print("\nCo2:");
				p.print(editCo2->getValue());
				p.print("\n");

				menu.event(MenuItem::back);
				menuItem = nullptr;

			}
		} else {
			// No menu item selected
			// We are in the top level menu
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
				int menuItemId = menu.getPosition();
				p.print("\nOK ITEM\n");
				if (menuItemId == 0) {
					menuItem = editHumidity;
				} else if (menuItemId == 1) {
					menuItem = editCo2;
				} else {
					p.print("\nMenu ITEM Selection failed\n");
					menuItem = nullptr;
				}
			}

			// Update current Humidity and Co2 values
			// -> when we are not in the edit menu
			// -> when the value has been updated
			if (humidityUpdated) {
				editHumidity->setValue(humidityData);
			}
			if (co2Updated) {
				editCo2->setValue(co2Data);
			}

			// Update the menu with the latest Humidity and Co2 values
			// Shown on the LCD every x amount of idle time
			if (sleepCounter >= 500) {
				// Print menu
				p.print("MENU");
				p.print("\nHumidity:");
				p.print(editHumidity->getValue());
				p.print("\nCo2:");
				p.print(editCo2->getValue());
				p.print("\n");
			}
		}

		// reset counter
		if (b1.read() || b2.read() || b3.read()) {
			sleepCounter = 0;
		}

		// no button is pressed for an amount of time -> call back event
		if (sleepCounter >= 10000) {
			b1Pressed = false;
			b2Pressed = false;
			b3Pressed = false;

			menu.event(MenuItem::back);

			p.print("\nBACK");

			// cancel setting value of the item
			menuItem = nullptr;
		}

		// Instead of Sleep, using vTaskDelay like in the readSensor task
		vTaskDelay(1);
		sleepCounter +=1;
		//Sleep(10);
		//sleepCounter +=10;
	}
}

/* TODO Menu */
//  Check that taskMenu2 works, then remove
void taskMenu(void *params) {
	(void) params;

	DigitalIoPin sw_a2(1, 8, DigitalIoPin::pullup, true);
	DigitalIoPin sw_a3(0, 5, DigitalIoPin::pullup, true);
	DigitalIoPin sw_a4(0, 6, DigitalIoPin::pullup, true);
	DigitalIoPin sw_a5(0, 7, DigitalIoPin::pullup, true);

	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);

	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);

	/* Main Menu has 2 menu items:
	 * 1. Monitor: rh, CO2, (maybe exit to go back to main menu OR timeout)
	 * * In Monitor menu -> display sensors values rh & CO2 (read sensor task)
	 * 2. Setting: rh, CO2, (maybe exit to go back to main menu OR timeout)
	 * * In Setting menu -> set values for 2 items: rh & CO2
	 */

	SimpleMenu mainMenu;

	SimpleMenu monitorMenu;
	SimpleMenu settingMenu;

	// TODO add monitor and setting to main menu

	//mainMenu.addItem(new MenuItem(monitorMenu));
	//mainMenu.addItem(new MenuItem(settingMenu));

	DecimalEdit *rhValue = new DecimalEdit(lcd, std::string("RH"), 0, 100, 10);
	DecimalEdit *co2Value = new DecimalEdit(lcd, std::string("CO2"), 10000, 200, 0.1);

	monitorMenu.addItem(new MenuItem(rhValue));
	monitorMenu.addItem(new MenuItem(co2Value));

	//PropertyEdit *menuItem = nullptr;

	idle_delay();

	lcd->begin(16,2);
	lcd->setCursor(0,0);
	mainMenu.event(MenuItem::show);

	idle_delay();
}


/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void)
{
	prvSetupHardware();

	heap_monitor_setup();

	// Initialise Mailbox for humidity and co2
	// Queue set to 3
	mailboxHumidity = xQueueCreate(3, sizeof(float));
	mailboxCo2 = xQueueCreate(3, sizeof(float));

	// The menu will read the mailbox
	xTaskCreate(taskMenu2, "Menu",
				configMINIMAL_STACK_SIZE * 4, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	// Sensor readers will write to the mailbox
	xTaskCreate(taskReadSensor, "Read sensors",
				configMINIMAL_STACK_SIZE * 4, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

