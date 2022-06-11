/* Standard includes. */
#include <stdio.h>
#include <conio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "extint.h"

/* Hardware simulator utility functions */
#include "HW_access.h"
void vApplicationIdleHook(void);


/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)
#define COM_CH_1 (1)
#define COM_CH_2 (2)

/* TASK PRIORITIES */
#define	TASK_SERIAL_SEND_PRI	(2 + tskIDLE_PRIORITY )
#define TASK_SERIAL_REC_PRI		(3 + tskIDLE_PRIORITY )
#define	SERVICE_TASK_PRI		(1 + tskIDLE_PRIORITY )

#define velicina (32)


void main_demo(void);

static void SerialReceiveTask_0(const void* pvParameters); 
static void SerialReceiveTask_1(const void* pvParameters); 
static void SerialReceiveTask_2(const void* pvParameters); 
static void SerialSend_Task0(const void* pvParameters);    
static void SerialSend_Task2(const void* pvParameters);    

static void prosecna_temp_un(const void* pvParameters);
static void prosecna_temp_sp(const void* pvParameters);

static void kalibracija(const void* pvParameters);

static void mux_seg7_un(const void* pvParameters);   
static void mux_seg7_sp(const void* pvParameters);   

uint64_t idleHookCounter;
static uint16_t sp1;
static uint16_t un1;
static uint16_t prosek_un;
static uint16_t prosek_sp;
static uint16_t promenljiva;
static uint16_t promenljiva2;

/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS */
static const uint8_t hexnum[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
								0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };

/* GLOBAL OS-HANDLES */
static SemaphoreHandle_t RXC_BS_0, RXC_BS_1, RXC_BS_2;
static TimerHandle_t tH1;
static QueueHandle_t otpornost_u;
static QueueHandle_t otpornost_s;
static QueueHandle_t max_otporn;
static QueueHandle_t min_otporn;
static QueueHandle_t min_temperatura;
static QueueHandle_t max_temperatura;
static SemaphoreHandle_t seg7_un, seg7_sp;
static QueueHandle_t kalibrisana_unutrasnja;
static QueueHandle_t kalibrisana_spoljasnja;
static QueueHandle_t thigh_queue;
static QueueHandle_t tlow_queue;

/* RXC - RECEPTION COMPLETE - INTERRUPT HANDLER */
static uint32_t prvProcessRXCInterrupt(void)
{
	BaseType_t xHigherPTW = pdFALSE;

	if (get_RXC_status(0) != 0) {

		configASSERT(xSemaphoreGiveFromISR(RXC_BS_0, &xHigherPTW) == pdTRUE);
	}
	if (get_RXC_status(1) != 0) {
		configASSERT(xSemaphoreGiveFromISR(RXC_BS_1, &xHigherPTW) == pdTRUE);
	}
	if (get_RXC_status(2) != 0) {
		configASSERT(xSemaphoreGiveFromISR(RXC_BS_2, &xHigherPTW) == pdTRUE);
	}

	portYIELD_FROM_ISR((uint32_t)xHigherPTW);
}

static void SerialReceiveTask_0(const void* pvParameters)
{
	static uint16_t bafer[velicina]; 
	static uint16_t index;
	uint8_t cc = 0;
	uint8_t un = 0;
	uint8_t sp = 0;
	uint16_t temperatura_u;
	uint16_t temperatura_s;
	uint16_t desetica;
	uint16_t jedinica;

	for (;;)
	{
		if (xSemaphoreTake(RXC_BS_0, portMAX_DELAY) != pdTRUE) {
			printf("GRESKAA\n");
		}

		if (get_serial_character(COM_CH_0, &cc) != 0) {
			printf("GRESKAA kanal 0\n");
		}

		if (cc == (uint8_t)'U')
		{
			un = (uint8_t)1;
			un1 = (uint8_t)1;
			sp1 = (uint8_t)0;
			index = (uint8_t)0;
		}
		else if (cc == (uint8_t)'S')
		{
			sp = (uint8_t)1;
			un1 = (uint8_t)0;
			sp1 = (uint8_t)1;
			index = (uint8_t)0;
		}
		else if ((cc == (uint8_t)'K') && (un == (uint8_t)1)) {
			un = (uint8_t)0;
			desetica = (uint16_t)bafer[0] - (uint16_t)48; 
			jedinica = (uint16_t)bafer[1] - (uint16_t)48;
			temperatura_u = (desetica * (uint16_t)10) + jedinica;

			if (xQueueSend(otpornost_u, &temperatura_u, 0) != pdTRUE)
			{
				printf("GRESKAA otpornost_u\n");
			}
			else {
				printf("Unutrasnja otpornost: %d\n", temperatura_u);
			}

			temperatura_u = (uint16_t)0;

			static uint8_t i = 0;
			for (i = 0; i < index; i++) {
				bafer[i] = (uint8_t)'\0';
			}
		}
		else if ((cc == (uint8_t)'K') && (sp == (uint8_t)1)) {
			sp = (uint8_t)0;
			desetica = (uint16_t)bafer[0] - (uint16_t)48; 
			jedinica = (uint16_t)bafer[1] - (uint16_t)48;
			temperatura_s = (desetica * (uint16_t)10) + jedinica;

			if (xQueueSend(otpornost_s, &temperatura_s, 0) != pdTRUE)
			{
				printf("GRESKAA otpornost_s\n");
			}
			else {
				printf("Spoljasnja otpornost: %d\n", temperatura_s);
			}

			temperatura_s = (uint16_t)0;

			static uint8_t i = 0;
			for (i = (uint8_t)0; i < index; i++) {
				bafer[i] = (uint8_t)'\0';
			}
		}
		else {
			bafer[index] = (uint8_t)cc;
			index++;
		}
	}
}

static void SerialReceiveTask_1(const void* pvParameters)
{
	static uint16_t bafer1[velicina]; 
	static uint16_t index1;
	uint8_t cc;
	uint16_t max_r;
	uint16_t max_r_desetica;
	uint16_t max_r_jedinica;
	uint16_t min_r;
	uint16_t min_r_desetica;
	uint16_t min_r_jedinica;
	uint16_t min_t;
	uint16_t max_t;

	for (;;)
	{
		if (xSemaphoreTake(RXC_BS_1, portMAX_DELAY) != pdTRUE) {
			printf("GRESKAA\n");
		}

		if (get_serial_character(COM_CH_1, &cc) != 0) {
			printf("GRESKAA kanal 1\n");
		}

		if (cc == (uint8_t)0x00)
		{
			index1 = 0;
		}

		else if (cc == (uint8_t)'CR')
		{
			if ((bafer1[0] == (uint16_t)'M') && (bafer1[1] == (uint16_t)'I') && (bafer1[2] == (uint16_t)'N') && (bafer1[3] == (uint16_t)'T') && (bafer1[4] == (uint16_t)'E') && (bafer1[5] == (uint16_t)'M') && (bafer1[6] == (uint16_t)'P'))
			{
				max_r_desetica = (uint16_t)bafer1[7] - (uint16_t)48;
				max_r_jedinica = (uint16_t)bafer1[8] - (uint16_t)48;
				min_t = (max_r_desetica * (uint16_t)10) + max_r_jedinica;

				configASSERT(xQueueSend(min_temperatura, &min_t, 0) == pdTRUE);

				min_t = (uint16_t)0;

				max_r_desetica = (uint16_t)bafer1[9] - (uint16_t)48; 
				max_r_jedinica = (uint16_t)bafer1[10] - (uint16_t)48;
				max_r = (max_r_desetica * (uint16_t)10) + max_r_jedinica;

				configASSERT(xQueueSend(max_otporn, &max_r, 0) == pdTRUE);

				max_r = (uint16_t)0;
			}
			else if ((bafer1[0] == (uint16_t)'M') && (bafer1[1] == (uint16_t)'A') && (bafer1[2] == (uint16_t)'X') && (bafer1[3] == (uint16_t)'T') && (bafer1[4] == (uint16_t)'E') && (bafer1[5] == (uint16_t)'M') && (bafer1[6] == (uint16_t)'P')) {
				min_r_desetica = (uint16_t)bafer1[7] - (uint16_t)48;
				min_r_jedinica = (uint16_t)bafer1[8] - (uint16_t)48;
				max_t = (min_r_desetica * (uint16_t)10) + min_r_jedinica;

				configASSERT(xQueueSend(max_temperatura, &max_t, 0) == pdTRUE);

				max_t = (uint16_t)0;

				min_r_desetica = (uint16_t)bafer1[9] - (uint16_t)48;
				min_r_jedinica = (uint16_t)bafer1[10] - (uint16_t)48;
				min_r = (min_r_desetica * (uint16_t)10) + min_r_jedinica;

				configASSERT(xQueueSend(min_otporn, &min_r, 0) == pdTRUE);

				min_r = (uint16_t)0;
			}
			else {
				printf("Pogresan format pri upisu na kanal 1\n");
			}

			static uint8_t m = 0;
			for (m = (uint8_t)0; m < (uint8_t)10; m++) {
				bafer1[m] = (uint16_t)'\0';
			}
		}
		else
		{
			bafer1[index1] = (uint8_t)cc;
			index1++;
		}
	}
}


static void SerialReceiveTask_2(const void* pvParameters)
{
	static uint16_t bafer2[velicina]; 
	static uint16_t index2;
	uint8_t cc = 0;
	uint16_t thigh;
	uint16_t thigh_desetica;
	uint16_t thigh_jedinica;
	uint16_t tlow;
	uint16_t tlow_desetica;
	uint16_t tlow_jedinica;

	for (;;)
	{
		if (xSemaphoreTake(RXC_BS_2, portMAX_DELAY) != pdTRUE) {
			printf("GRESKAA\n");
		}

		if (get_serial_character(COM_CH_2, &cc) != 0) {
			printf("GRESKAA kanal 2\n");
		}

		if (cc == (uint8_t)'T') {
			index2 = (uint16_t)0;
		}
		else if ((cc == (uint8_t)'K')) {

			thigh_desetica = (uint16_t)bafer2[0] - (uint16_t)48; 
			thigh_jedinica = (uint16_t)bafer2[1] - (uint16_t)48;
			thigh = (thigh_desetica * (uint16_t)10) + thigh_jedinica;

			tlow_desetica = (uint16_t)bafer2[2] - (uint16_t)48; 
			tlow_jedinica = (uint16_t)bafer2[3] - (uint16_t)48;
			tlow = (tlow_desetica * (uint16_t)10) + tlow_jedinica;

			if (xQueueSend(thigh_queue, &thigh, 0) != pdTRUE)
			{
				//printf("Neuspesno slanje u red thigh_queue\n");
			}
			else {
				printf("THIGH: %d\n", thigh);
			}

			if (xQueueSend(tlow_queue, &tlow, 0) != pdTRUE)
			{
				//printf("Neuspesno slanje u red tlow_queue\n");
			}
			else {
				printf("TLOW: %d\n", tlow);
			}

			static uint8_t i = 0;
			for (i = 0; i < index2; i++) {
				bafer2[i] = (uint8_t)'\0';
			}
		}
		else {
			bafer2[index2] = (uint8_t)cc;
			index2++;
		}
	}
}

static void SerialSend_Task0(const void* pvParameters)
{
	uint8_t c = (uint8_t)'u';

	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		configASSERT(send_serial_character(COM_CH_0, c) == 0);
	}
}

static void SerialSend_Task2(const void* pvParameters)
{
	uint8_t c = (uint8_t)'t';

	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		configASSERT(send_serial_character(COM_CH_2, c) == 0);
	}
}

static void prosecna_temp_un(const void* pvParameters)
{
	uint16_t t_un;
	uint16_t counter = 0;
	uint16_t suma = 0;

	for (;;)
	{
		configASSERT(xQueueReceive(otpornost_u, &t_un, portMAX_DELAY) == pdTRUE);

		if ((t_un > (uint16_t)0) && (t_un < (uint16_t)33)) {
			suma = suma + t_un;
			counter++;
			if (counter == (uint16_t)5)
			{
				prosek_un = suma / (uint16_t)5;
				counter = (uint16_t)0;
				suma = (uint16_t)0;
			}
		}
		else {
			printf("Unutrasnja otpornost nije u zeljenom opsegu. \n");
		}
	}
}

static void prosecna_temp_sp(const void* pvParameters)
{
	uint16_t t_sp;
	uint16_t counter = 0;
	uint16_t suma = 0;

	for (;;)
	{
		configASSERT(xQueueReceive(otpornost_s, &t_sp, portMAX_DELAY) == pdTRUE);

		if ((t_sp > (uint16_t)0) && (t_sp < (uint16_t)33)) {
			suma = suma + t_sp;
			counter++;
			if (counter == (uint16_t)5)
			{
				prosek_sp = suma / (uint16_t)5;
				counter = 0;
				suma = 0;
			}
		}
		else {
			printf("Spoljasnja otpornost nije u zeljenom opsegu. \n");
		}
	}
}

static void timer_seg7(const TimerHandle_t* tm) {

	static uint8_t dd;

	configASSERT(!get_LED_BAR(2, &dd));

	if ((dd == (uint8_t)1) && (promenljiva == (uint8_t)0)) {
		promenljiva = 1; 
		dd = 0;

		configASSERT(xSemaphoreGive(seg7_un, portMAX_DELAY) == pdTRUE);
	}
	else if ((dd == (uint8_t)2) && (promenljiva2 == (uint8_t)0)) {
		promenljiva2 = 1;  
		dd = 0;

		configASSERT(xSemaphoreGive(seg7_sp, portMAX_DELAY) == pdTRUE);
	}
	else {
		//printf("nije stisnut nijedan taster\n");
	}
}

static void mux_seg7_un(const void* pvParameters) {

	uint16_t ku;
	uint16_t ou;
	uint16_t desetica_un_t;
	uint16_t jedinica_un_t;
	uint16_t desetica_un_o;
	uint16_t jedinica_un_o;

	for (;;) {

		configASSERT(xQueueReceive(kalibrisana_unutrasnja, &ku, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(otpornost_u, &ou, portMAX_DELAY) == pdTRUE);

		desetica_un_t = ku / (uint16_t)10;
		jedinica_un_t = ku % (uint16_t)10;
		desetica_un_o = ou / (uint16_t)10;
		jedinica_un_o = ou % (uint16_t)10;

		promenljiva = 0;

		configASSERT(xSemaphoreTake(seg7_un, portMAX_DELAY) == pdTRUE);

		else {

			configASSERT(!select_7seg_digit(0));
			configASSERT(!set_7seg_digit(0x3e));
			configASSERT(!select_7seg_digit(1));
			configASSERT(!set_7seg_digit(hexnum[desetica_un_t]));
			configASSERT(!select_7seg_digit(2));
			configASSERT(!set_7seg_digit(hexnum[jedinica_un_t]));
			configASSERT(!select_7seg_digit(3));
			configASSERT(!set_7seg_digit(hexnum[desetica_un_o]));
			configASSERT(!select_7seg_digit(4));
			configASSERT(!set_7seg_digit(hexnum[jedinica_un_o]));
		}
	}
}

static void mux_seg7_sp(const void* pvParameters) {
	uint16_t desetica_sp_t;
	uint16_t jedinica_sp_t;
	uint16_t desetica_sp_o;
	uint16_t jedinica_sp_o;
	uint16_t os;
	uint16_t ks;

	for (;;) {

		configASSERT(xQueueReceive(kalibrisana_spoljasnja, &ks, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(otpornost_s, &os, portMAX_DELAY) == pdTRUE);

		desetica_sp_t = ks / (uint16_t)10;
		jedinica_sp_t = ks % (uint16_t)10;
		desetica_sp_o = os / (uint16_t)10;
		jedinica_sp_o = os % (uint16_t)10;

		promenljiva2 = 0;

		if (xSemaphoreTake(seg7_sp, portMAX_DELAY) != pdTRUE) {
			printf("Neuspesno TAKE semafora seg7_sp\n");
		}
		else {
			configASSERT(!select_7seg_digit(0));
			configASSERT(!set_7seg_digit(0x6d));
			configASSERT(!select_7seg_digit(1));
			configASSERT(!set_7seg_digit(hexnum[desetica_sp_t]));
			configASSERT(!select_7seg_digit(2));
			configASSERT(!set_7seg_digit(hexnum[jedinica_sp_t]));
			configASSERT(!select_7seg_digit(3));
			configASSERT(!set_7seg_digit(hexnum[desetica_sp_o]));
			configASSERT(!select_7seg_digit(4));
			configASSERT(!set_7seg_digit(hexnum[jedinica_sp_o]));
		}
	}
}


static void kalibracija(const void* pvParameters) {

	static uint16_t max_otp;
	static uint16_t min_otp;
	static uint16_t max_tem;
	static uint16_t min_tem;
	static uint16_t THIGH;
	static uint16_t TLOW;
	static uint16_t trenutna_temp_un;
	static uint16_t trenutna_temp_sp;
	uint8_t  counter;

	for (;;) {

		configASSERT(xQueueReceive(thigh_queue, &THIGH, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(tlow_queue, &TLOW, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(max_otporn, &max_otp, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(min_otporn, &min_otp, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(max_temperatura, &max_tem, portMAX_DELAY) == pdTRUE);
		configASSERT(xQueueReceive(min_temperatura, &min_tem, portMAX_DELAY) == pdTRUE);

		printf("Min otpornost: %d\n", min_otp);
		printf("Max otpornost: %d\n", max_otp);
		printf("Min temperatura: %d\n", min_tem);
		printf("Max temperatura: %d\n", max_tem);

		if ((prosek_un != (uint16_t)0) && (un1 == (uint16_t)1)) {
			trenutna_temp_un = ((min_tem - max_tem) * (prosek_un - min_otp) / (max_otp - min_otp)) + max_tem;

			configASSERT(xQueueSend(kalibrisana_unutrasnja, &trenutna_temp_un, 0) == pdTRUE);

			prosek_un = 0;
			prosek_sp = 0;
			un1 = 0;
		}

		if ((prosek_sp != (uint16_t)0) && (sp1 == (uint16_t)1)) {

			trenutna_temp_sp = ((min_tem - max_tem) * (prosek_sp - min_otp) / (max_otp - min_otp)) + max_tem;

			configASSERT(xQueueSend(kalibrisana_spoljasnja, &trenutna_temp_sp, 0) == pdTRUE);

			prosek_sp = 0;
			prosek_un = 0;
			sp1 = 0;
		}

		printf("Kalibrisana unutrasnja temperatura: %d\n", trenutna_temp_un);
		printf("Kalibrisana spoljasnja temperatura: %d\n", trenutna_temp_sp);

		if (((trenutna_temp_un < (uint16_t)TLOW) || (trenutna_temp_un > (uint16_t)THIGH)) && ((trenutna_temp_sp < (uint16_t)TLOW) || (trenutna_temp_sp > (uint16_t)THIGH))) {
			for (counter = (uint8_t)0; counter < (uint8_t)3; counter++) {

				configASSERT(!set_LED_BAR(0, 0xff));
				configASSERT(!set_LED_BAR(1, 0xff));
				vTaskDelay(pdMS_TO_TICKS(500));

				configASSERT(!set_LED_BAR(0, 0x00));
				configASSERT(!set_LED_BAR(1, 0x00));
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
		else if ((trenutna_temp_un < (uint16_t)TLOW) || (trenutna_temp_un > (uint16_t)THIGH)) {

			for (counter = (uint8_t)0; counter < (uint8_t)3; counter++) {

				configASSERT(!set_LED_BAR(0, 0xff));
				vTaskDelay(pdMS_TO_TICKS(500));
				configASSERT(!set_LED_BAR(0, 0x00));
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
		else if ((trenutna_temp_sp < (uint16_t)TLOW) || (trenutna_temp_sp > (uint16_t)THIGH)) {

			for (counter = (uint8_t)0; counter < (uint8_t)3; counter++) {

				configASSERT(!set_LED_BAR(1, 0xff));
				vTaskDelay(pdMS_TO_TICKS(500));
				configASSERT(!set_LED_BAR(1, 0x00));
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
		else {
			printf("Obe temperature su u zeljenom opsegu\n");
		}
	}
}

void main_demo(void)
{
	BaseType_t status;

	configASSERT(init_LED_comm() == 0);
	configASSERT(init_serial_uplink(COM_CH_0) == 0);
	configASSERT(init_serial_downlink(COM_CH_0) == 0);
	configASSERT(init_serial_uplink(COM_CH_1) == 0);
	configASSERT(init_serial_downlink(COM_CH_1) == 0);
	configASSERT(init_serial_uplink(COM_CH_2) == 0);
	configASSERT(init_serial_downlink(COM_CH_2) == 0);
	configASSERT(init_7seg_comm() == 0);

	vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt);

	RXC_BS_0 = xSemaphoreCreateBinary();
	RXC_BS_1 = xSemaphoreCreateBinary();
	RXC_BS_2 = xSemaphoreCreateBinary();
	seg7_un = xSemaphoreCreateBinary();
	seg7_sp = xSemaphoreCreateBinary();

	configASSERT(xTaskCreate(SerialReceiveTask_0, "SR0", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)TASK_SERIAL_REC_PRI + 1, NULL));
	configASSERT(xTaskCreate(SerialReceiveTask_1, "SR1", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)TASK_SERIAL_REC_PRI + 1, NULL));
	configASSERT(xTaskCreate(SerialReceiveTask_2, "SR2", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)TASK_SERIAL_REC_PRI + 1, NULL));
	configASSERT(xTaskCreate(SerialSend_Task0, "ST0", configMINIMAL_STACK_SIZE, (void*)1, TASK_SERIAL_SEND_PRI, NULL));
	configASSERT(xTaskCreate(SerialSend_Task2, "ST2", configMINIMAL_STACK_SIZE, (void*)1, TASK_SERIAL_SEND_PRI, NULL));

	configASSERT(xTaskCreate(prosecna_temp_un, "prosecna_temp_un", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)SERVICE_TASK_PRI, NULL));
	configASSERT(xTaskCreate(prosecna_temp_sp, "prosecna_temp_sp", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)SERVICE_TASK_PRI, NULL));
	configASSERT(xTaskCreate(kalibracija, "kalibracija", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)SERVICE_TASK_PRI, NULL));
	configASSERT(xTaskCreate(mux_seg7_un, "mux_seg7_un", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)SERVICE_TASK_PRI, NULL));
	configASSERT(xTaskCreate(mux_seg7_sp, "mux_seg7_sp", configMINIMAL_STACK_SIZE, (void*)1, (UBaseType_t)SERVICE_TASK_PRI, NULL));


	otpornost_u = xQueueCreate(10, sizeof(uint16_t));
	if (otpornost_u == NULL)
	{
		printf("GRESKAAA otpornost_u\n");
	}
	otpornost_s = xQueueCreate(10, sizeof(uint16_t));
	if (otpornost_s == NULL)
	{
		printf("GRESKAAA otpornost_s\n");
	}
	max_otporn = xQueueCreate(10, sizeof(uint16_t));
	if (max_otporn == NULL)
	{
		printf("GRESKAAA max_otpornost\n");
	}
	min_otporn = xQueueCreate(10, sizeof(uint16_t));
	if (min_otporn == NULL)
	{
		printf("GRESKAAA min_otpornost\n");
	}
	max_temperatura = xQueueCreate(10, sizeof(uint16_t));
	if (max_temperatura == NULL)
	{
		printf("GRESKAAA max_temperatura\n");
	}
	min_temperatura = xQueueCreate(10, sizeof(uint16_t));
	if (min_temperatura == NULL)
	{
		printf("GRESKAAA min_temperatura\n");
	}
	kalibrisana_unutrasnja = xQueueCreate(10, sizeof(uint16_t));
	if (kalibrisana_unutrasnja == NULL)
	{
		printf("GRESKAAA kalibrisana_unutrasnja\n");
	}
	kalibrisana_spoljasnja = xQueueCreate(10, sizeof(uint16_t));
	if (kalibrisana_spoljasnja == NULL)
	{
		printf("GRESKAAA kalibrisana_spoljasnja\n");
	}
	thigh_queue = xQueueCreate(10, sizeof(uint16_t));
	if (thigh_queue == NULL)
	{
		printf("GRESKAAA thigh_queue\n");
	}
	tlow_queue = xQueueCreate(10, sizeof(uint16_t));
	if (tlow_queue == NULL)
	{
		printf("GRESKAAA tlow_queue\n");
	}

	tH1 = xTimerCreate(
		"test timer",
		pdMS_TO_TICKS(100),
		pdTRUE,
		NULL,
		timer_seg7
	);

	if (xTimerStart(tH1, 0) != pdPASS) {
		printf("GRESKAAA tajmer\n");
	}

	vTaskStartScheduler();

	for (;;) {}
}

void vApplicationIdleHook(void) {

	idleHookCounter++;
}