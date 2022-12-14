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
#include <cr_section_macros.h>
#include "task.h"
#include <stdint.h>
#include "heap_lock_monitor.h"
#include "retarget_uart.h"

#include "ModbusRegister.h"
#include "DigitalIoPin.h"
#include "LiquidCrystal.h"
#include "queue.h"
#include "semphr.h"
#include <mutex>
#include "Fmutex.h"
#include "modbus.h"
#include "SimpleMenu.h"
#include "ITM_print.h"
#include "DecimalEdit.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static DigitalIoPin siga(0, 5, DigitalIoPin::input, true);
static DigitalIoPin sigb(0, 6, DigitalIoPin::input, true);
modbusConfig modbus;
typedef struct TaskData {
	LpcUart *uart;
	Fmutex *guard;
} TaskData;
struct SensorData {
	int temp;
	int rh;
	int co2;
	uint64_t time_stamp;
};
QueueHandle_t mailboxRh;
QueueHandle_t mailboxCo2;
QueueHandle_t menuQueue;
QueueHandle_t rotaryQueue;

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
 * Interrupts
 ****************************************************************************/
extern "C" {
void PIN_INT0_IRQHandler(void) {
	static bool clockwise;
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;

	clockwise = siga.read();
	xQueueSendToBackFromISR(rotaryQueue, (void *) &clockwise, &xHigherPriorityWoken);

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}/**/
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
static void vConfigureInterrupts(void){
	Chip_PININT_Init(LPC_GPIO_PIN_INT);

	for(int i = 0; i < 50000; i++);

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);

	Chip_SYSCTL_PeriphReset(RESET_PININT);

	Chip_INMUX_PinIntSel(0, 0, 6);

	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_DisableIntHigh(LPC_GPIO_PIN_INT, PININTCH(0));

	NVIC_ClearPendingIRQ(PIN_INT0_IRQn);
	NVIC_SetPriority(PIN_INT0_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	NVIC_EnableIRQ(PIN_INT0_IRQn);/**/
}

/* end runtime statictics collection */
static void idle_delay()
{
	vTaskDelay(1);
}

static void vSensor(void *pvParameters){

	SensorData sensor_event;

	while(1)  {
		sensor_event.temp = modbus.get_temp();
		sensor_event.rh = modbus.get_rh();
		sensor_event.co2 = modbus.get_co2();
		sensor_event.time_stamp= xTaskGetTickCount();

		vTaskDelay(500);

	}
}

void vRotary(void *pvParameters){
	bool clockwise = false;
	int timestamp;
	int prev_timestamp = 0;


	LpcUart*dbgu = static_cast<LpcUart *>(pvParameters);
	while(1){/**/
		bool received=false;
		int signal;
		DigitalIoPin button(1, 8, DigitalIoPin::pullup, true);
		if(button.read()){
			signal = 3;
			xQueueSendToBack(menuQueue, &signal, 0 );
			dbgu->write((const char*)"button\r\n");
			vTaskDelay(350);
		}
		if(xQueueReceive(rotaryQueue, &clockwise, (TickType_t) 10)){
			received = true;
		}
		if(received){
			xQueueReceive(rotaryQueue, &clockwise, (TickType_t) 150);
			timestamp = xTaskGetTickCount();
			if (timestamp - prev_timestamp > 70){
				if(clockwise){
					signal = 1;
					xQueueSendToBack(menuQueue, &signal, 0 );
					dbgu->write((const char*)"clockwise\r\n");
				}else{
					signal = 2;
					xQueueSendToBack(menuQueue, &signal, 0 );
					dbgu->write((const char*)"counterclockwise\r\n");
				}
				prev_timestamp = timestamp;
			}
			vTaskDelay(5);
		}
	}
}


void vMenu(void *pvParameters) {
	LpcUart*dbgu = static_cast<LpcUart *>(pvParameters);

	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);
	LiquidCrystal *lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);

	DigitalIoPin b1(1, 8, DigitalIoPin::pullup, true); // sw_A2 go up
	DigitalIoPin b2(0, 5, DigitalIoPin::pullup, true); // sw_A3 go down
	DigitalIoPin b3(0, 6, DigitalIoPin::pullup, true); // sw_A4 pressed (clicked)

	bool b1Pressed = false;
	bool b2Pressed = false;
	bool b3Pressed = false;

    SimpleMenu menu;

    DecimalEdit *editRh = new DecimalEdit(lcd, std::string("RH"), 0, 100, 1);
    DecimalEdit *editCo2 = new DecimalEdit(lcd, std::string("C02"), 200, 10000, 50);

    PropertyEdit* menuItem = nullptr;

    int sleepCounter = 0;

    menu.addItem(new MenuItem(editRh));
    menu.addItem(new MenuItem(editCo2));

    editRh->setValue(0.0);
	editCo2->setValue(0.0);

	lcd->begin(16,2);
	lcd->setCursor(0,0);
	menu.event(MenuItem::show);

	while(1) {
		// Read queue values into these variables
		// (In the example they use uint_32_t, but we want float values)
		float rhData = 0.0;
		float co2Data = 0.0;
		// Read mailbox. 0 means don't wait for queue, return immediately if queue empty
		// True when a value is read and false when queue is empty
		bool rhUpdated = xQueueReceive(mailboxRh, &rhData, 0 );
		bool co2Updated = xQueueReceive(mailboxCo2, &co2Data, 0 );

		/* Receive Rotary signal*/
		int rotary;
		bool input = xQueueReceive(menuQueue, &rotary, 0 );

		// press a button -> send an event to menu handler
		if (menuItem != nullptr) {
			// We have a menuItem selected
			// We are in edit property mode
			if (b1.read()) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				menu.event(MenuItem::up);
			}

			if (b2.read()) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				menu.event(MenuItem::down);
			}

			if (b3.read()) {
				b3Pressed = true;
			} else if (b3Pressed == true) {
				b3Pressed = false;
				menu.event(MenuItem::ok);

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
				if (menuItemId == 0) {
					menuItem = editRh;
				} else if (menuItemId == 1) {
					menuItem = editCo2;
				} else {
					menuItem = nullptr;
				}
			}

			// Update current Humidity and Co2 values
			// -> when we are not in the edit menu
			// -> when the value has been updated
			if (rhUpdated) {
				editRh->setValue(rhData);
			}
			if (co2Updated) {
				editCo2->setValue(co2Data);
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

			// cancel setting value of the item
			menuItem = nullptr;
		}

		vTaskDelay(1);
		sleepCounter +=1;
	}
}


int main(void){
	prvSetupHardware();
	heap_monitor_setup();
	vConfigureInterrupts();

	rotaryQueue = xQueueCreate(50, sizeof(bool));
	mailboxRh = xQueueCreate(3, sizeof(float));
	mailboxCo2 = xQueueCreate(3, sizeof(float));
	menuQueue = xQueueCreate(10, sizeof(int));

	/* UART port config */
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none, none };

	LpcUart *dbgu = new LpcUart(cfg);


	xTaskCreate(vSensor, "vSensor",
			((configMINIMAL_STACK_SIZE) + 128), NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vRotary, "vRotary",
			(configMINIMAL_STACK_SIZE) + 128, dbgu, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);/**/

	xTaskCreate(vMenu, "vMenu",
			(configMINIMAL_STACK_SIZE) * 4, dbgu, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);/**/

	/*Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

