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
#include "IntegerEdit.h"
#include "DecimalEdit.h"

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

	vTaskDelay(10);

	/*
	ModbusMaster node4(241);
	node4.begin(9600); // all nodes must operate at the same speed!
	node4.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister TEMP(&node4, 256, true);
	*/

	ModbusMaster node5(240); // Create modbus object that connects to slave id 240
	node5.begin(9600); // all nodes must operate at the same speed!
	node5.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister CO2(&node5, 257, true);

	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);

	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);

	DigitalIoPin relay(0, 27, DigitalIoPin::output); // CO2 relay
	relay.write(0);

	while(true)
	{
		float rh, co2;
		char buffer1[32], buffer3[32];

		rh = RH.read()/10.0;
		snprintf(buffer1, 32, "RH=%5.1f%%", rh);
		printf("%s\n",buffer1);

		lcd->begin(16, 2);
		lcd->setCursor(0, 0);
		lcd->print(buffer1);

		/*
		temp = TEMP.read();
		snprintf(buffer2, 32, "TEMP=%5.1f%", temp);
		printf("%s\n",buffer2);

		lcd->setCursor(0, 0);
		// Print a message to the LCD.
		lcd->print(buffer2);
		*/

		vTaskDelay(10);

		co2 = CO2.read();
		snprintf(buffer3, 32, "CO2=%5.1f", co2);
		printf("%s\n",buffer3);

		DEBUGOUT("%s\n",buffer3);

		lcd->setCursor(0, 1);
		lcd->print(buffer3);
	}

	vTaskDelay(10);
}

/* TODO Menu */
void taskMenuTest(void *params) {

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

	SimpleMenu menu; // this could also be allocated from the heap

	DecimalEdit *co2level = new DecimalEdit(lcd, std::string("CO2 Level"), 0, 200, 20);

	//PropertyEdit* menuItem = nullptr;

	menu.addItem(new MenuItem(co2level));

	vTaskDelay(500);

	lcd->begin(16,2);
	lcd->setCursor(0,0);
	menu.event(MenuItem::show);

	// TODO set up CO2 level value
	vTaskDelay(100);
}

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void)
{
	prvSetupHardware();

	heap_monitor_setup();

	/* TODO create queue to recieve signals from rotary coder
	 * if ...
	 * Menu 1 -> display sensors values
	 * Menu 2 -> setting sensors values
	 */
	//xQueue = xQueueCreate( 5, sizeof( int32_t ) );

	xTaskCreate(taskReadSensor, "read sensors",
				configMINIMAL_STACK_SIZE * 4, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/*xTaskCreate(taskMenuTest, "test menu",
				configMINIMAL_STACK_SIZE * 4, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);*/

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

