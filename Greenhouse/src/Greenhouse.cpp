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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static QueueHandle_t q;
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

	char buff[40];
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
	/**/
	/* UART port config */
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none, none };

	LpcUart *dbgu = new LpcUart(cfg);


	while(1){/**/
		bool received=false;
		DigitalIoPin button(1, 8, DigitalIoPin::pullup, true);
		if(button.read()){
			dbgu->write((const char*)"button\r\n");

			vTaskDelay(350);
		}
		if(xQueueReceive(q, &clockwise, (TickType_t) 10)){
			received = true;
		}
		if(received){
			xQueueReceive(q, &clockwise, (TickType_t) 150);
			timestamp = xTaskGetTickCount();
			if (timestamp - prev_timestamp > 70){
				if(clockwise){
					dbgu->write((const char*)"clockwise\r\n");
				}else{
					dbgu->write((const char*)"counterclockwise\r\n");
				}
				prev_timestamp = timestamp;
			}
			vTaskDelay(5);
		}
	}
}

/**/static void vDisplay(void *pvParams){
	//LCD configuration
	DigitalIoPin *rs = new DigitalIoPin(0, 29, DigitalIoPin::output);
	DigitalIoPin *en = new DigitalIoPin(0, 9, DigitalIoPin::output);
	DigitalIoPin *d4 = new DigitalIoPin(0, 10, DigitalIoPin::output);
	DigitalIoPin *d5 = new DigitalIoPin(0, 16, DigitalIoPin::output);
	DigitalIoPin *d6 = new DigitalIoPin(1, 3, DigitalIoPin::output);
	DigitalIoPin *d7 = new DigitalIoPin(0, 0, DigitalIoPin::output);
	LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

	while(true){
		vTaskDelay(500);
	}

}
int main(void){
	prvSetupHardware();
	heap_monitor_setup();
	vConfigureInterrupts();


	q = xQueueCreate(50, sizeof(bool));
	xTaskCreate(vSensor, "vSensor",
			((configMINIMAL_STACK_SIZE) + 128), NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vRotary, "vRotary",
			((configMINIMAL_STACK_SIZE) * 4), NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);/**/

	xTaskCreate(vDisplay, "vDisplay",
			(configMINIMAL_STACK_SIZE)+128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);/**/

	/*Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

