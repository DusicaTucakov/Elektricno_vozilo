/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "extint.h"

/* Hardware simulator utility functions */
#include "HW_access.h"

/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)

/* TASK PRIORITIES */
#define	TASK_SERIAL_SEND_PRI		((UBaseType_t)2 + tskIDLE_PRIORITY  )
#define TASK_SERIAL_REC_PRI			((UBaseType_t)3+ tskIDLE_PRIORITY )
#define	SERVICE_TASK_PRI		((UBaseType_t)1+ tskIDLE_PRIORITY )

/* TASKS: FORWARD DECLARATIONS */
void main_demo(void);
void led_bar_tsk(void* pvParameters);
static void SerialSend_Task0(void* pvParameters);
static void SerialReceive_Task0(void* pvParameters);

/* TRASNMISSION DATA - CONSTANT IN THIS APPLICATION */
static uint16_t napon;

/* RECEPTION DATA BUFFER */
#define R_BUF_SIZE (32)
static char r_buffer[R_BUF_SIZE]; 
static uint8_t volatile r_point;

/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS */
static const unsigned char hexnum[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
								0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };

/* GLOBAL OS-HANDLES */
static SemaphoreHandle_t RXC_BinarySemaphore0;
static SemaphoreHandle_t RXC_BinarySemaphore1;
static QueueHandle_t napon_q;

static void SerialSend_Task0(void* pvParameters) {
	uint8_t c = (uint8_t)'U';

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));

		if (send_serial_character(COM_CH_0, c) != 0) {
			printf("Greska prilikom slanja");
		}
	}
}

static void SerialReceive_Task0(void* pvParameters) {
	uint8_t cc;

	for (;;) {
		if(xSemaphoreTake(RXC_BinarySemaphore0, portMAX_DELAY)!=pdTRUE) {
			printf("Neuspesno");
		}
		
		if(get_serial_character(COM_CH_0, &cc)!=0) {
			printf("Neuspesno");
		}

		if (cc == (uint8_t) 'V') {
			r_point = 0;
		}
		else if (cc == (uint8_t)'m') {	

			char *vrednost; 

			napon = (uint16_t)strtol(r_buffer,&vrednost,10);
			printf(" Napon: %s mV\n", r_buffer);

			if(xQueueSend(napon_q, &napon, 0)!= pdTRUE) {
				printf("Neuspesno slanje u red");
			}

			r_buffer[0] = '\0';
			r_buffer[1] = '\0';
			r_buffer[2] = '\0';
			r_buffer[3] = '\0';
			r_buffer[4] = '\0';
		}
		else {
			r_buffer[r_point++] = (char) cc; 
		}
	
	}
}

static uint32_t prvProcessRXCInterrupt(void) {
	BaseType_t higher_priority_task_woken = pdFALSE;

	if (get_RXC_status(0) != 0) {
		if (xSemaphoreGiveFromISR(RXC_BinarySemaphore0, &higher_priority_task_woken) != pdTRUE) {
			printf("Greska pri slanju podatka\n");
		}
	}
}

// MAIN - SYSTEM STARTUP POINT 
void main_demo(void) {

	if(init_serial_uplink(COM_CH_0)!=0) {
		printf("Neuspesna inicijalizacija");
	}

	if(init_serial_downlink(COM_CH_0)!=0) {
		printf("Neuspesna inicijalizacija");
	}

	// SERIAL RECEPTION INTERRUPT HANDLER 
	vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt);//interrupt za serijsku prijem

 
	RXC_BinarySemaphore0 = xSemaphoreCreateBinary();

	if (RXC_BinarySemaphore0 == NULL) {
		printf("Greska pri kreiranju\n");
	}

	RXC_BinarySemaphore1 = xSemaphoreCreateBinary();

	if (RXC_BinarySemaphore1 == NULL) {
		printf("Greska pri kreiranju\n");
	}

	napon_q = xQueueCreate(10, sizeof(uint16_t));

	if (napon_q == NULL) {
		printf("Greska pri kreiranju\n");
	}

	BaseType_t status;

	// SERIAL RECEIVER AND SEND TASK 
	status=xTaskCreate(SerialReceive_Task0, "SRx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_REC_PRI, NULL);

	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}
	
	status=xTaskCreate(SerialSend_Task0, "STx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_SEND_PRI, NULL);

	if (status !=pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	r_point = 0;

    vTaskStartScheduler();
}
