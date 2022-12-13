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

// Includes
#include <cr_section_macros.h>
#include "FreeRTOS.h"
#include "task.h"
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


/*****************************************************************************
 * Private functions
 ****************************************************************************/
typedef struct TaskData {
	LpcUart *uart;
	Fmutex *guard;
} TaskData;

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
/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
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


/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

}
/* end runtime statictics collection */
static void idle_delay(){
	vTaskDelay(1);
}

/*****************************************************************************
 * FreeRTOS Tasks
 ****************************************************************************/
/*
Fmutex mutex;

bool latest;
void vRfixer(void *pvParameters){
	mutex.lock();
	while(1){
		mutex.lock();
		vTaskDelay(100);
	}

}*/

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
	while(1){
		DigitalIoPin button(1, 8, DigitalIoPin::pullup, true);
		if(button.read()){
			t->guard->lock();
			t->uart->write((const char*)"button pressed");
			t->guard->unlock();
			vTaskDelay(350);
		}
		vTaskDelay(5);
	}
}


/* DataRead
void taskReadRH(void *params){
	(void) params;

	retarget_init();

	ModbusMaster node3(241); // Create modbus object that connects to slave id 241 (HMP60)
	node3.begin(9600); // all nodes must operate at the same speed!
	node3.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister RH(&node3, 256, true);

	ModbusMaster node4(240); // Create modbus object that connects to slave id 241 (HMP60)
	node4.begin(9600); // all nodes must operate at the same speed!
	node4.idle(idle_delay); // idle function is called while waiting for reply from slave
	ModbusRegister RH(&node4, 256, true);

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
	// configure display geometry
	lcd->begin(16, 2);
	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd->setCursor(0, 0);
	// Print a message to the LCD.
	lcd->print("MQTT_FreeRTOS");

	while(true) {
		float rh;
		char buffer[32];

		vTaskDelay(2000);

		rh = RH.read()/10.0;
		snprintf(buffer, 32, "RH=%5.1f%%", rh);
		printf("%s\n",buffer);
		lcd->setCursor(0, 1);
		// Print a message to the LCD.
		lcd->print(buffer);

	}
}*/

int main(void)
{
	prvSetupHardware();
	heap_monitor_setup();
	vConfigureInterrupts();

	/*
	xTaskCreate(taskReadRH, "read RH",
			configMINIMAL_STACK_SIZE * 4, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);*/


	//uart setup
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
	q = xQueueCreate(50, sizeof(bool));
	static TaskData t;
	t.uart = uart;
	t.guard = guard;

	xTaskCreate(vRotary, "vRotary",
		configMINIMAL_STACK_SIZE + 256, &t, (tskIDLE_PRIORITY + 1UL),
		(TaskHandle_t *) NULL);

	xTaskCreate(vRotaryButton, "vRotaryButton",
		configMINIMAL_STACK_SIZE + 128, &t, (tskIDLE_PRIORITY + 1UL),
		(TaskHandle_t *) NULL);

	/*xTaskCreate(vRfixer, "vRfixer",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);
*/
	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

