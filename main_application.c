/* Standard includes. */
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "extint.h"

/* Hardware simulator utility functions */
#include "HW_access.h"

//DEFINISAN PUN KAPACITET BATERIJE U kW 
#define PUN_KAPACITET 80

/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)
#define COM_CH_1 (1)

/* TASK PRIORITIES */
#define	TASK_SERIAL_SEND_PRI		((UBaseType_t)2 + tskIDLE_PRIORITY  )
#define TASK_SERIAL_REC_PRI			((UBaseType_t)3+ tskIDLE_PRIORITY )
#define	SERVICE_TASK_PRI		((UBaseType_t)1+ tskIDLE_PRIORITY )

/* TASKS: FORWARD DECLARATIONS */
void main_demo(void);
static void led_bar_tsk(void* pvParameters);
static void SerialSend_Task0(void* pvParameters);
static void SerialReceive_Task0(void* pvParameters);
static void prosek_nivoa_baterije(void* pvParameters);
static void nivo_baterije_u_procentima(void* pvParameters);
static void SerialSend_Task1(void* pvParameters);
static void SerialReceive_Task1(void* pvParameters);

/* TRASNMISSION DATA - CONSTANT IN THIS APPLICATION */
static uint16_t napon;
static uint16_t MINBAT;
static uint16_t MAXBAT;
static uint16_t POTROSNJA_BAT;
static uint16_t nivo_baterije_procenti;
static uint16_t autonomija;


/* RECEPTION DATA BUFFER */
#define R_BUF_SIZE (32)
static char r_buffer[R_BUF_SIZE];
static char r_buffer1[R_BUF_SIZE];
static uint8_t volatile r_point;
static uint8_t volatile r_point1;
static char brojevi[R_BUF_SIZE];
static char min_napon[R_BUF_SIZE];
static char max_napon[R_BUF_SIZE];


/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS */
static const uint8_t hexnum[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
								0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };

/* GLOBAL OS-HANDLES */
static SemaphoreHandle_t RXC_BinarySemaphore0;
static SemaphoreHandle_t RXC_BinarySemaphore1;
static QueueHandle_t napon_q;

// Task koji simulira vrednosti napona koji stizu svakih 1s, tako sto svakih 1s saljemo karakter 'U'  
static void SerialSend_Task0(void* pvParameters) {
	uint8_t c = (uint8_t)'U';

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));

		if (send_serial_character(COM_CH_0, c) != 0) {
			printf("Greska prilikom slanja");
		}
	}
}

// Task koji vrsi prijem podataka sa kanala 0 
static void SerialReceive_Task0(void* pvParameters) { 
	uint8_t cc;

	for (;;) {
		if (xSemaphoreTake(RXC_BinarySemaphore0, portMAX_DELAY) != pdTRUE) {
			printf("Neuspesno");
		}

		if (get_serial_character(COM_CH_0, &cc) != 0) {
			printf("Neuspesno");
		}

		if (cc == (uint8_t)'V') {
			r_point = 0;
		}
		else if (cc == (uint8_t)'m') {
			char* vrednost;

			napon = (uint16_t)strtol(r_buffer, &vrednost, 10);
			printf(" Napon: %s mV\n", r_buffer);

			if (xQueueSend(napon_q, &napon, 0) != pdTRUE) {
				printf("Neuspesno slanje u red");
			}

			r_buffer[0] = '\0';
			r_buffer[1] = '\0';
			r_buffer[2] = '\0';
			r_buffer[3] = '\0';
		}
		else {
			r_buffer[r_point++] = (char)cc;
		}

	}
}


//Task koji racuna prosek zadnje tri pristigle vrednosti napona 
static void prosek_nivoa_baterije(void* pvParameters){

	int v_buf = 0;
	int brojac = 0;
	int suma = 0;
	int prosek;

	for (;;) {
		if (xQueueReceive(napon_q, &v_buf, portMAX_DELAY) != pdTRUE) {
			printf("Neuspesan prijem");
		}
		suma = suma + v_buf;
		brojac++;
		if (brojac == 3) {
			prosek = suma / 3;
			printf("Prosek: %d mV\n", prosek);
			brojac = 0;
			suma = 0;
			v_buf = 0;
		}

	}
}

//Task koji vrsi prijem podataka sa kanala 1
 //      \00MINBAT10\0d\00MAXBAT4900\0d\00PP15\0d   
static void SerialReceive_Task1(void* pvParameters) {

	uint8_t cc;
	uint8_t j = 0, k = 0, l = 0;

	for (;;) {
		if (xSemaphoreTake(RXC_BinarySemaphore1, portMAX_DELAY) != pdTRUE) {
			printf("Neuspesno");
		}

		if (get_serial_character(COM_CH_1, &cc) != 0) {
			printf("Neuspesno");
		}

		if (cc == (uint8_t)0x00) {
			j = 0;
			k = 0;
			l = 0;
			r_point1 = 0;
		}
		else if (cc == (uint8_t)13) { // 13 decimalno - CR(carriage return)

			if (r_buffer1[0] == 'M' && r_buffer1[1] == 'I' && r_buffer1[2] == 'N') {
				size_t i;
				for (i = (size_t)0; i < strlen(r_buffer1); i++) {

					if (r_buffer1[i] == '0' || r_buffer1[i] == '1' || r_buffer1[i] == '2' || r_buffer1[i] == '3' || r_buffer1[i] == '4' || r_buffer1[i] == '5' || r_buffer1[i] == '6' || r_buffer1[i] == '7' || r_buffer1[i] == '8' || r_buffer1[i] == '9') {
						min_napon[k] = r_buffer1[i];
						k++;
					}
				}
			}
			else if (r_buffer1[0] == 'M' && r_buffer1[1] == 'A' && r_buffer1[2] == 'X') {
				size_t i;
				for (i = 0; i < strlen(r_buffer1); i++) {

					if (r_buffer1[i] == '0' || r_buffer1[i] == '1' || r_buffer1[i] == '2' || r_buffer1[i] == '3' || r_buffer1[i] == '4' || r_buffer1[i] == '5' || r_buffer1[i] == '6' || r_buffer1[i] == '7' || r_buffer1[i] == '8' || r_buffer1[i] == '9') {
						max_napon[l] = r_buffer1[i];
						l++;
					}
				}
			}
			else if (r_buffer1[0] == 'P' && r_buffer1[1] == 'P') {
				size_t i;
				for (i = (size_t)0; i < strlen(r_buffer1); i++) {

					if (r_buffer1[i] == '0' || r_buffer1[i] == '1' || r_buffer1[i] == '2' || r_buffer1[i] == '3' || r_buffer1[i] == '4' || r_buffer1[i] == '5' || r_buffer1[i] == '6' || r_buffer1[i] == '7' || r_buffer1[i] == '8' || r_buffer1[i] == '9') {
						brojevi[j] = r_buffer1[i];
						j++;
					}
				}
			}
			else {
				printf("Nepoznata naredba");
			}

			char* ostatak;

			MINBAT = (uint16_t)strtol(min_napon, &ostatak, 10);
			MAXBAT = (uint16_t)strtol(max_napon, &ostatak, 10);
			printf("MINBAT %d\n", MINBAT);
			printf("MAXBAT %d\n", MAXBAT);
			POTROSNJA_BAT = (uint16_t)strtol(brojevi, &ostatak, 10);
			printf("potrosnja %d\n", POTROSNJA_BAT);

			r_buffer1[0] = '\0';
			r_buffer1[1] = '\0';
			r_buffer1[2] = '\0';
			r_buffer1[3] = '\0';
			r_buffer1[4] = '\0';
			r_buffer1[5] = '\0';
			r_buffer1[6] = '\0';
			r_buffer1[7] = '\0';
			r_buffer1[8] = '\0';
			r_buffer1[9] = '\0';
			r_buffer1[10] = '\0';
			r_buffer1[11] = '\0';
		}
		else {
			r_buffer1[r_point1++] = (char)cc;
		}
	}
}

// Task koji preko kanala 1 salje svakih 1s vrednost nivoa baterije u procentima
static void SerialSend_Task1(void* pvParameters) {

	uint8_t str[5];
	static uint8_t brojac = 0;

	for (;;) {

		vTaskDelay(pdMS_TO_TICKS(1000));

		uint16_t jedinica = nivo_baterije_procenti % (uint16_t)10;
		uint16_t desetica = (nivo_baterije_procenti / (uint16_t)10) % (uint16_t)10;
		uint16_t stotina = nivo_baterije_procenti / (uint16_t)100;

		str[0] = (uint8_t)stotina + (uint8_t)'0';
		str[1] = (uint8_t)desetica + (uint8_t)'0';
		str[2] = (uint8_t)jedinica + (uint8_t)'0';
		str[3] = (uint8_t)'%';
		str[4] = (uint8_t)'\n';

		brojac++;

		if (MINBAT != (uint16_t)0 && MAXBAT != (uint16_t)0) {
			if (brojac == (uint8_t)1) {
				if (send_serial_character(COM_CH_1, str[0]) != 0) {
					printf("Greska prilikom slanja");
				}
			}
			else if (brojac == (uint8_t)2) {
				if (send_serial_character(COM_CH_1, str[1]) != 0) {
					printf("Greska prilikom slanja");
				}
			}
			else if (brojac == (uint8_t)3) {
				if (send_serial_character(COM_CH_1, str[2]) != 0) {
					printf("Greska prilikom slanja");
				}
			}
			else if (brojac == (uint8_t)4) {
				if (send_serial_character(COM_CH_1, str[3]) != 0) {
					printf("Greska prilikom slanja");
				}
			}
			else if (brojac == (uint8_t)5) {
				if (send_serial_character(COM_CH_1, str[4]) != 0) {
					printf("Greska prilikom slanja");
				}
			}
			else {
				brojac = (uint8_t)0;
			}
		}
	}
}

// Task koji racuna trenutni nivo baterije u procentima i na osnovu toga koliko jos kilomenatra automobil moze da se krece 
static void nivo_baterije_u_procentima(void* pvParameters) {

	uint16_t v_buf;
	uint8_t str[3];

	for (;;) {

		// preuzima iz reda vrednost napona
		if (xQueueReceive(napon_q, &v_buf, portMAX_DELAY) != pdTRUE) {
			printf("Neuspesan prijem");
		}

		if (MINBAT != (uint16_t)0 && MAXBAT != (uint16_t)0) {

			nivo_baterije_procenti = (uint16_t)100 * (v_buf - MINBAT) / (MAXBAT - MINBAT); //100 * (trenutni_napon - MINBAT) / (MAXBAT - MINBAT)
			printf("Nivo baterije u procentima: %d %% \n", nivo_baterije_procenti);
			if (nivo_baterije_procenti < (uint16_t)10) {

				if (set_LED_BAR(0, 0x08) != 0) { // ukljucena 4. ledovka u 1. stupcu
					printf("Greska");
				}
			}
			else {
				if (set_LED_BAR(0, 0x00) != 0) {
					printf("Greska");
				}
			}
		}
		if (POTROSNJA_BAT != (uint16_t)0 && MINBAT != (uint16_t)0 && MAXBAT != (uint16_t)0) {
			autonomija = nivo_baterije_procenti * (uint16_t)PUN_KAPACITET / POTROSNJA_BAT; 
			printf("Auto moze da vozi jos: %d km\n", autonomija);
		}
	}
}

// Task koji na osnovu pritisnutog tastera ispisuje na displeju informacije, brzina osvezavanja 100ms
static void led_bar_tsk(void* pvParameters) {

	uint8_t s3, s4;
	uint16_t stop_procenti = 0;
	uint16_t start_procenti = 0;
	uint16_t razlika = 0;

	for (;;) {

		vTaskDelay(pdMS_TO_TICKS(100));

		if (get_LED_BAR(2, &s3) != 0) { // stubac 3 
			printf("Greska pri ocitavanju");
		}
		if (get_LED_BAR(3, &s4) != 0) { // stubac 4
			printf("Greska pri ocitavanju");
		}

		if (s3 == (uint8_t)1) { // nivo baterije u procentima 

			uint16_t jedinica = nivo_baterije_procenti % (uint16_t)10;
			uint16_t desetica = (nivo_baterije_procenti / (uint16_t)10) % (uint16_t)10;
			uint16_t stotina = nivo_baterije_procenti / (uint16_t)100;
			if (select_7seg_digit(2) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[jedinica]) != 0) {
				printf("Greska");
			}
			if (select_7seg_digit(1) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[desetica]) != 0) {
				printf("Greska");
			}
			if (select_7seg_digit(0) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[stotina]) != 0) {
				printf("Greska");
			}


		}
		else if (s3 == (uint8_t)2) { // koliko jos kilometara moze da se krece auto

			uint16_t jedinica = autonomija % (uint16_t)10;
			uint16_t desetica = (autonomija / (uint16_t)10) % (uint16_t)10;
			uint16_t stotina = autonomija / (uint16_t)100;
			if (select_7seg_digit(2) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[jedinica]) != 0) {
				printf("Greska");
			}
			if (select_7seg_digit(1) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[desetica]) != 0) {
				printf("Greska");
			}
			if (select_7seg_digit(0) != 0) {
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[stotina]) != 0) {
				printf("Greska");
			}

		}

		else if (s3 == (uint8_t)4) { // razlika izmedju start - stop naredbe

			uint16_t jedinica = razlika % (uint16_t)10;
			uint16_t desetica = razlika / (uint16_t)10;
			if (select_7seg_digit(1) != 0)
			{
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[jedinica]) != 0)
			{
				printf("Greska");
			}
			if (select_7seg_digit(0) != 0)
			{
				printf("Greska");
			}
			if (set_7seg_digit(hexnum[desetica]) != 0)
			{
				printf("Greska");
			}
		}

		else {

			for (int i = 0; i < 8; i++) {
				if (select_7seg_digit((uint8_t)i) != 0) {
					printf("Greska");
				}
				if (set_7seg_digit(0x00) != 0) {
					printf("Greska");
				}
			}
		}

		//aktivan start taster
		if (s4 == (uint8_t)128) { // prvi taster od gore u 4. stupcu

			start_procenti = nivo_baterije_procenti;
			if (set_LED_BAR(1, 0x01) != 0) {  // ukljucena prva dioda od dole u 2. stupcu 
				printf("Greska");
			}
		}

		//aktivan stop taster
		else if (s4 == (uint8_t)64) { // drugi taster od gore u 4. stupcu

			if (set_LED_BAR(1, 0x00) != 0) {// sve ledovke iskljucene u 2. stupcu
				printf("Greska");
			}
			stop_procenti = nivo_baterije_procenti;
			razlika = start_procenti - stop_procenti;
		}
		else {
			printf("Nijedan taster nije pritisnut");
		}

	}
}

//Interrupt rutina za serijsku komunikaciju u kojoj se proverava koji je kanal poslao i na osnovu toga daje odgovarajuci semafor 
static uint32_t prvProcessRXCInterrupt(void) {
	BaseType_t higher_priority_task_woken = pdFALSE;

	if (get_RXC_status(0) != 0) {
		if (xSemaphoreGiveFromISR(RXC_BinarySemaphore0, &higher_priority_task_woken) != pdTRUE) {
			printf("Greska pri slanju podatka\n");
		}
	}
	if (get_RXC_status(1) != 0) {
		if (xSemaphoreGiveFromISR(RXC_BinarySemaphore1, &higher_priority_task_woken) != pdTRUE) {
			printf("Greska pri slanju podatka\n");
		}
	}
	portYIELD_FROM_ISR((uint32_t)higher_priority_task_woken);
}



// MAIN - SYSTEM STARTUP POINT 
void main_demo(void) {

	// Inicijalizacija periferija 
	if (init_LED_comm() != 0) {
		printf("Neuspesna inicijalizacija");
	}
	if (init_7seg_comm() != 0) {
		printf("Neuspesna inicijalizacija");
	}
	// inicijalizacija serijske TX na kanalu 0
	if (init_serial_uplink(COM_CH_0) != 0) {
		printf("Neuspesna inicijalizacija");
	}

	// inicijalizacija serijske TX na kanalu 1
	if (init_serial_uplink(COM_CH_1) != 0) {
		printf("Neuspesna inicijalizacija");
	}
	// inicijalizacija serijske RX na kanalu 0
	if (init_serial_downlink(COM_CH_0) != 0) {
		printf("Neuspesna inicijalizacija");
	}
	// inicijalizacija serijske RX na kanalu 1
	if (init_serial_downlink(COM_CH_1) != 0) {
		printf("Neuspesna inicijalizacija");
	}

	// SERIAL RECEPTION INTERRUPT HANDLER 
	vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt);//interrupt za serijsku prijem


	// Kreiranje semafora 
	RXC_BinarySemaphore0 = xSemaphoreCreateBinary();

	if (RXC_BinarySemaphore0 == NULL) {
		printf("Greska pri kreiranju\n");
	}

	RXC_BinarySemaphore1 = xSemaphoreCreateBinary();

	if (RXC_BinarySemaphore1 == NULL) {
		printf("Greska pri kreiranju\n");
	}

	//Kreiranje reda
	napon_q = xQueueCreate(10, sizeof(uint16_t));

	if (napon_q == NULL) {
		printf("Greska pri kreiranju\n");
	}

	//Kreiranje taskova
	BaseType_t status;

	status = xTaskCreate(
		prosek_nivoa_baterije,
		"baterija task",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	status = xTaskCreate(
		nivo_baterije_u_procentima,
		"procenti task",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	status = xTaskCreate(
		led_bar_tsk,
		"led task",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	// SERIAL RECEIVER AND SEND TASK 
	status = xTaskCreate(SerialReceive_Task0, "SRx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_REC_PRI, NULL);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	status = xTaskCreate(SerialReceive_Task1, "SRx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_REC_PRI, NULL);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	status = xTaskCreate(SerialSend_Task0, "STx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_SEND_PRI, NULL);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	status = xTaskCreate(SerialSend_Task1, "STx", configMINIMAL_STACK_SIZE, NULL, (UBaseType_t)TASK_SERIAL_SEND_PRI, NULL);
	if (status != pdPASS) {
		printf("Greska pri kreiranju\n");
	}

	r_point = 0;
	r_point1 = 0;

	vTaskStartScheduler();
}
