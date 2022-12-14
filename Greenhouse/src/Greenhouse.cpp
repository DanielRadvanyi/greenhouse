
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
#include "NoEdit.h"

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
QueueHandle_t mailboxSetCo2;
QueueHandle_t mailboxTemp;
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

/*
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
*/

/* Read sensors */
void vReadSensor(void *params)
{
	(void) params;

	retarget_init();		// important for safe printf, also work with DEBUGOUT

	ModbusMaster node3(241); // Create modbus object that connects to slave id 241 (HMP60)
	node3.begin(9600); // all nodes must operate at the same speed!
	node3.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister RH(&node3, 256, true);


	ModbusMaster node4(241);
	node4.begin(9600); // all nodes must operate at the same speed!
	node4.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister TEMP(&node4, 257, true);


	ModbusMaster node5(240); // Create modbus object that connects to slave id 240
	node5.begin(9600); // all nodes must operate at the same speed!
	node5.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister CO2(&node5, 257, true);

	// CO2 relay valve
	DigitalIoPin relay(0, 27, DigitalIoPin::output);
	relay.write(0);

	while(true)
	{
		float rh, co2, temp;
		//char buffer1[32], buffer3[32];

		rh = RH.read()/10.0;

		// Write rh to the rh MAILBOX.
		// Third parameter 0 means don't wait for the queue to have space.
		if( !xQueueSend(mailboxRh, &rh, 0)) {
			printf("Failed to send RH data, queue full.\r\n");
		}

		co2 = CO2.read()*10.0;

		// Write co2 to the co2 MAILBOX.
		// Third parameter 0 means don't wait for the queue to have space.
		if( !xQueueSend(mailboxCo2, &co2, 0)) {
			printf("Failed to send CO2 data, queue full.\r\n");
		}

		temp = TEMP.read()/10.0;
		if( !xQueueSend(mailboxTemp, &temp, 0)) {
			printf("Failed to send TEMP data, queue full.\r\n");
		}
		vTaskDelay(10);
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

	bool b1Pressed = false;
	bool b2Pressed = false;
	bool b3Pressed = false;

    SimpleMenu menu;

    //NoEdit *readRh = new NoEdit(lcd, std::string("RH"));
    DecimalEdit *readRh = new DecimalEdit(lcd, std::string("RH"), 0, 100, 1);
    DecimalEdit *readCo2 = new DecimalEdit(lcd, std::string("C02"), 0, 10000, 1);
    DecimalEdit *editCo2 = new DecimalEdit(lcd, std::string("SET C02"), 200, 10000, 50);
    DecimalEdit *readTemp = new DecimalEdit(lcd, std::string("TEMP"), 0, 60, 1);

    PropertyEdit* menuItem = nullptr;

    int sleepCounter = 0;

    menu.addItem(new MenuItem(readRh));
    menu.addItem(new MenuItem(readCo2));
    menu.addItem(new MenuItem(editCo2));
    menu.addItem(new MenuItem(readTemp));

	lcd->begin(16,2);
	lcd->setCursor(0,0);
	menu.event(MenuItem::show);

	while(1) {
		// Read queue values into these variables
		// (In the example they use uint_32_t, but we want float values)
		float rhData = 0.0;
		float co2Data = 0.0;
		float setCo2Data = 0.0;
		float tempData = 0.0;

		// Read mailbox. 0 means don't wait for queue, return immediately if queue empty
		// True when a value is read and false when queue is empty
		bool rhUpdated = xQueueReceive(mailboxRh, &rhData, 0 );
		bool co2Updated = xQueueReceive(mailboxCo2, &co2Data, 0 );
		bool setCo2Updated = xQueueReceive(mailboxSetCo2, &setCo2Data, 0 );
		bool tempUpdated = xQueueReceive(mailboxTemp, &tempData, 0 );

		/* Receive Rotary signal*/
		int rotary=0;
		bool input = xQueueReceive(menuQueue, &rotary, 0 );

		// press a button -> send an event to menu handler
		if (menuItem != nullptr) {
			// We have a menuItem selected
			// We are in edit property mode
			if (rotary==1) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				menu.event(MenuItem::up);
			}

			if (rotary==2) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				menu.event(MenuItem::down);
			}

			if (rotary==3) {
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
			if (rotary==1) {
				b1Pressed = true;
			} else if (b1Pressed == true) {
				b1Pressed = false;
				menu.event(MenuItem::down);
			}

			if (rotary==2) {
				b2Pressed = true;
			} else if (b2Pressed == true) {
				b2Pressed = false;
				menu.event(MenuItem::up);
			}

			if (rotary==3) {
				b3Pressed = true;
			} else if (b3Pressed == true) {
				b3Pressed = false;
				int menuItemId = menu.getPosition();
				if (menuItemId == 2) {
					menu.event(MenuItem::ok);
					menuItem = editCo2;
				} else {
					menuItem = nullptr;
				}
			}

			// Update current values
			// -> when we are not in the edit menu (only simulator values)
			// -> when the value has been updated
			if (rhUpdated) {
				readRh->setValue(rhData);
			}
			if (co2Updated) {
				readCo2->setValue(co2Data); // not the value edited with the rotary
			}
			if (setCo2Updated) {
				editCo2->setValue(setCo2Data);
			}
			if (tempUpdated) {
				readTemp->setValue(tempData);
			}
		}

		// reset counter
		if (rotary==1 || rotary==2 ||rotary==3) {
			sleepCounter = 0;
		}

		// no button is pressed for an amount of time -> call back event
		if (sleepCounter >= 2000) {
			b1Pressed = false;
			b2Pressed = false;
			b3Pressed = false;

			menu.event(MenuItem::back);

			// cancel setting value of the item
			menuItem = nullptr;
		}

		vTaskDelay(10);
		sleepCounter +=10;
	}
}


int main(void){
	prvSetupHardware();
	heap_monitor_setup();
	vConfigureInterrupts();

	rotaryQueue = xQueueCreate(50, sizeof(bool));
	mailboxRh = xQueueCreate(3, sizeof(int32_t));
	mailboxCo2 = xQueueCreate(3, sizeof(int32_t));
	mailboxSetCo2 = xQueueCreate(3, sizeof(int32_t));
	mailboxTemp = xQueueCreate(3, sizeof(int32_t));
	menuQueue = xQueueCreate(10, sizeof(int));

	/* UART port config */
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none, none };

	LpcUart *dbgu = new LpcUart(cfg);


	/*xTaskCreate(vSensor, "vSensor",
			((configMINIMAL_STACK_SIZE) + 128), NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);*/

	xTaskCreate(vReadSensor, "vReadSensor",
				((configMINIMAL_STACK_SIZE) * 5), NULL, (tskIDLE_PRIORITY + 1UL),
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
