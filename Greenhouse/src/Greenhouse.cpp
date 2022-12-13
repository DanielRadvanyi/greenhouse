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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static QueueHandle_t q;
static DigitalIoPin siga(1, 6, DigitalIoPin::input, true);
static DigitalIoPin sigb(0, 8, DigitalIoPin::input, true);

typedef struct TaskData {
	LpcUart *uart;
	Fmutex *guard;
} TaskData;

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
	xQueueSendToBackFromISR(q, (void *) &clockwise, &xHigherPriorityWoken);

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}
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
	/* Initialize PININT driver */
	Chip_PININT_Init(LPC_GPIO_PIN_INT);

	//Filter out sw2 bounce. lpcxpresso specific.
	for(int i = 0; i < 50000; i++);

	/* Enable PININT clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);

	/* Reset the PININT block */
	Chip_SYSCTL_PeriphReset(RESET_PININT);

	/* Configure interrupt channel for the GPIO pin in INMUX block */
	Chip_INMUX_PinIntSel(0, 0, 8);

	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(0));
	Chip_PININT_DisableIntHigh(LPC_GPIO_PIN_INT, PININTCH(0));

	/* Enable interrupt in the NVIC */
	NVIC_ClearPendingIRQ(PIN_INT0_IRQn);
	NVIC_SetPriority(PIN_INT0_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	NVIC_EnableIRQ(PIN_INT0_IRQn);
}

/* end runtime statictics collection */
static void idle_delay()
{
	vTaskDelay(1);
}

void taskReadSensor(void *pvParameters){
	TaskData *t = static_cast<TaskData *>(pvParameters);

	//portENTER_CRITICAL();
	retarget_init();

	ModbusMaster node3(241); // Create modbus object that connects to slave id 241 (HMP60)
	node3.begin(9600); // all nodes must operate at the same speed!
	node3.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister RH(&node3, 256, true);

	vTaskDelay(1000);

	ModbusMaster node4(240); // Create modbus object that connects to slave id 240
	node4.begin(9600); // all nodes must operate at the same speed!
	node4.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister CO2(&node4, 256, true);
	//portEXIT_CRITICAL();

	DigitalIoPin relay(0, 27, DigitalIoPin::output); // CO2 relay
	relay.write(0);

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

	while(true) {
		float rh, co2;
		char buffer2[32];
		char buffer1[32];

		vTaskDelay(2000);

		rh = RH.read()/10.0;
		snprintf(buffer1, 32, "RH=%5.1f%%\r\n", rh);
		t->guard->lock();
		t->uart->write(buffer1);
		t->guard->unlock();

		lcd->begin(16, 2);
		lcd->setCursor(0, 0);
		// Print a message to the LCD.
		lcd->print(buffer1);

		vTaskDelay(2000);

		co2 = CO2.read()/10.0;
		snprintf(buffer2, 32, "CO2=%5.1f%%\r\n", co2);
		t->guard->lock();
		t->uart->write(buffer2);
		t->guard->unlock();

		lcd->setCursor(0, 1);
		lcd->print(buffer2);
	}
}

void vRotary(void *pvParameters){
	TaskData *t = static_cast<TaskData *>(pvParameters);
	bool clockwise = false;
	int value = 0;
	int timestamp;
	int prev_timestamp = 0;

	t->uart->write("Rotary task running:\r\n");
	while(1){
		bool received=false;
		if(xQueueReceive(q, &clockwise, (TickType_t) 5000)){
			received = true;
		}
		xQueueReceive(q, &clockwise, (TickType_t) 150);

		if(received){

			timestamp = xTaskGetTickCount();
			if (timestamp - prev_timestamp > 70){
				if(clockwise){
					value--;
					char buf[255];
					snprintf(buf, 255, "[%d] clck %d\r\n", timestamp, value);
					t->guard->lock();
					t->uart->write(buf);
					t->guard->unlock();
				}else{
					value++;
					char buf[255];
					snprintf(buf, 255, "[%d] nclck %d\r\n", timestamp, value);
					t->guard->lock();
					t->uart->write(buf);
					t->guard->unlock();
				}
				prev_timestamp = timestamp;
			}
			vTaskDelay(5);
		}else{
			int timestamp = xTaskGetTickCount();
			char buf[255];
			snprintf(buf, 255, "[%d] timeout %d\r\n", timestamp, value);
			t->guard->lock();
			t->uart->write(buf);
			t->guard->unlock();
		}
	}
}

void vRotaryButton(void *pvParameters){
	TaskData *t = static_cast<TaskData *>(pvParameters);
	while(1){/**/
		DigitalIoPin button(1, 8, DigitalIoPin::pullup, true);
		if(button.read()){
			t->guard->lock();
			t->uart->write((const char*)"button pressed\r\n");
			t->guard->unlock();
			vTaskDelay(350);
		}
		vTaskDelay(5);
	}
}

int main(void)
{
	prvSetupHardware();
	heap_monitor_setup();
	vConfigureInterrupts();

	/*uart setup*/
	LpcPinMap none = { .port = -1, .pin = -1}; // unused pin has negative values in it
	LpcPinMap txpin = { .port = 0, .pin = 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { .port = 0, .pin = 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = {
		.pUART = LPC_USART0,
		.speed = 115200,
		.data = UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1,
		.rs485 = false,
		.tx = txpin,
		.rx = rxpin,
		.rts = none,
		.cts = none
	};
	LpcUart *uart = new LpcUart(cfg);
	Fmutex *guard = new Fmutex();
	static TaskData t;
	t.uart = uart;
	t.guard = guard;
	q = xQueueCreate(50, sizeof(bool));
	xTaskCreate(taskReadSensor, "read sensor",
			configMINIMAL_STACK_SIZE * 5, &t, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vRotary, "vRotary",
			configMINIMAL_STACK_SIZE + 256, &t, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vRotaryButton, "vRotaryButton",
			configMINIMAL_STACK_SIZE + 128, &t, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

