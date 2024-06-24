/*! @mainpage Proyecto Integrador
 *
 * @section genDesc General Description
 *
 * 	
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Facundo Main (facundo.main@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max3010x.h"
#include "spo2_algorithm.h"
#include "switch.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
#include "ble_mcu.h"
#include "led.h"

/*==================[macros and definitions]=================================*/

#define MAX_PEAKS 40
//#define BUFFER_SIZE 256
#define DURACION 5
#define SAMPLE_FREQ 25  // 25 muestras por segundo
#define BUFFER_SIZE (SAMPLE_FREQ * DURACION) //2400 corresponde a 24 segundos


#define CONFIG_SENSOR_DELAY 24000  //ver con ejemplos anteriores cuanto poner aca para equivalga a 24 segs
#define CONFIG_TIMER1_US 1000  //timer cada un segundo usado para temperatura
#define CONFIG_PROCES_DELAY 500 // es en mili segundos, esto equivale a 0.5s
#define CONFIG_BLINK_PERIOD 500
#define CONFIG_MAX_DELAY 4000  
#define LED_BT LED_1

float temperaturaProm = 0.0;
int diferenciaTemp = 0;
float HRV = 0.0;
int8_t temperaturaVect[24];
uint16_t temp_inst = 0;

bool estresHRV = false;
bool fiebre = false;
bool iniciar = false;

int32_t peak_intervals[MAX_PEAKS];  //almacena intervalos entre picos de señal ppg
int32_t peak_count = 0;  //contador de picos se señal ppg

uint32_t irBuffer[BUFFER_SIZE]; //infrared LED sensor data
int32_t bufferLength=BUFFER_SIZE; //data length
int32_t heartRate; //heart rate value
int8_t validHeartRate ; //indicator to show if the heart rate calculation is valid

uint8_t entradaBle ;
bool terminoSensor = false;
bool terminoLM = false;
int temp_count = 0;
bool estresALTO = false;
bool estresMEDIO = false;
char msg[48];

/*==================[internal data definition]===============================*/

/*TaskHandle_t utilizarSensor_task_handle = NULL;
TaskHandle_t medirTemperatura_task_handle = NULL;
TaskHandle_t procesamientoDatos_task_handle = NULL;
TaskHandle_t transmitirDatos_task_handle = NULL;
 */


/*==================[internal functions declaration]=========================*/

static void cambia_iniciar (){
	iniciar =! iniciar;
}

/*void FuncTimer1 (void *param){
	vTaskNotifyGiveFromISR (medirTemperatura_task_handle, pdFALSE);
}*/

void read_ble ( uint8_t *data, uint8_t length){  //funcion igual que en ejemplos
	if (data[0] == 'A' ){
		cambia_iniciar();
	}
}
/*void utilizarSensorTask (void *vParameter){
	while (true){
		if (iniciar) {
		uint32_t sample_count = 0;
			while (sample_count < BUFFER_SIZE){

				while (MAX3010X_available() == false)
					MAX3010X_check();

				irBuffer[BUFFER_SIZE] = MAX3010X_getIR();
				MAX3010X_nextSample();

				sample_count++;
			}
			vTaskDelay (CONFIG_MAX_DELAY / portTICK_PERIOD_MS);
		}
	}
}
*/
void calculointervalos (uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, int32_t *pn_heart_rate, int8_t *pch_hr_valid){

	printf ("entra en calculointervalos\n");
	uint32_t un_ir_mean;  
	int32_t k;
	//int32_t i, n_exact_ir_valley_locs_count;
	int32_t n_th1, n_npks;
	int32_t an_ir_valley_locs[40] ;
	int32_t n_peak_interval_sum;
	
	// calculates DC mean and subtract DC from ir
	un_ir_mean = 0;
	for (k = 0; k < n_ir_buffer_length; k++) {
		un_ir_mean += pun_ir_buffer[k];
	}
	un_ir_mean = un_ir_mean / n_ir_buffer_length;

	// remove DC and invert signal so that we can use peak detector as valley detector
	for (k = 0; k < n_ir_buffer_length; k++){
		an_x[k] = -1 * (pun_ir_buffer[k] - un_ir_mean);
	} 

	// 4 pt Moving Average
	for (k = 0; k < BUFFER_SIZE - MA4_SIZE; k++) {
    	an_x[k] = (an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]) / 4;
  	}

	// calculate threshold
	n_th1 = 0;
	for (k = 0; k < BUFFER_SIZE; k++) {
    	n_th1 += an_x[k];
	}
  	n_th1 = n_th1 / BUFFER_SIZE;
  	if (n_th1 < 30) n_th1 = 30; // min allowed
  	if (n_th1 > 60) n_th1 = 60; // max allowed

	for (k = 0; k < MAX_PEAKS; k++){
		an_ir_valley_locs[k] = 0;
	} 

  	// since we flipped signal, we use peak detector as valley detector
  	maxim_find_peaks(an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, MAX_PEAKS); // peak_height, peak_distance, max_num_peaks

  	n_peak_interval_sum = 0;
  	peak_count = 0;
  	if (n_npks >= 2) {
    	for (k = 1; k < n_npks; k++) {
      	int32_t interval = abs(an_ir_valley_locs[k] - an_ir_valley_locs[k - 1]); 
     	peak_intervals[peak_count++] = interval;
      	n_peak_interval_sum += interval;
   		}
    n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1);
    *pn_heart_rate = (int32_t)((FreqS * 60) / n_peak_interval_sum);
    *pch_hr_valid = 1;
  	} else {
    *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
    *pch_hr_valid = 0;
  }
}


void calculoVariabilidad (void){
	// se usa la desviacion estandar de los intervalos RR (SDNN)
	
	//calculointervalos(irBuffer, bufferLength, &heartRate, &validHeartRate);
		//if (peak_count < 2) // No suficiente datos
		//return -1;
		//; 
	printf ("entra en calculoHRV \n");
  	float mean = 0.0;
  	for (int i = 0; i < peak_count; i++) {
    	mean += peak_intervals[i];
  	}
  	mean /= peak_count;
  	for (int i = 0; i < peak_count; i++) {
   		HRV += pow(peak_intervals[i] - mean, 2);
	}

	HRV /= (peak_count - 1);
	HRV = sqrt(HRV);
	printf ("\tHRV: %f\n", HRV);
	sprintf (msg, "*mHRV: %f\n", HRV);
	BleSendString(msg);
	
}

/*void medirTemperaturaTask (void *vParameter){
	while (true){
		if (iniciar){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		int temp_count = 0;

		AnalogInputReadSingle (CH1, &temp_inst);  //lectura temperatura instantanea.
		temperaturaVect[temp_count] = temp_inst;

		if (temp_count == 24 || temp_count > 24){
			temp_count = 0;
		}
		else ++temp_count;

		}
		
	}
	
}
*/
void calculoTemperaturaProm (int8_t *vecTemp){
	int elementosVector = 5;
	int suma = 0;
	int diferenciaaux = 0;

	for (int i = 1; i < elementosVector ; i++){
		diferenciaaux = vecTemp[i] - vecTemp[i-1];
		if (diferenciaaux > diferenciaTemp){
			diferenciaTemp = diferenciaaux;
		}
	}

	for (int i = 0; i < elementosVector ; i++){
		suma += vecTemp[i];
	}
	//temperaturaProm = temperaturaProm 
	temperaturaProm = suma/elementosVector;
	temperaturaProm = temperaturaProm / 10;

	printf ("temperaturaProm = %f \n", temperaturaProm);
	sprintf (msg, "*mTemperatura Promedio = %f\n", temperaturaProm);
	BleSendString(msg);
}

/*void procesamientoDatosTask (void *vParameter){
	while(true){
		if (iniciar){
			calculointervalos(irBuffer, bufferLength, &heartRate, &validHeartRate);
			calculoVariabilidad();
			calculoTemperaturaProm (temperaturaVect);

			if (HRV < 25){
				estresHRV = true;
			}
			else estresHRV = false;
			if (diferenciaTemp > 1500){  //1500 equivale a 1,5°c
				fiebre = true;
			}
			else fiebre = false;
		
		}
		vTaskDelay (CONFIG_PROCES_DELAY / portTICK_PERIOD_MS);
	}

}

void transmitirDatosTask (void *vParameter){
	while (true) {
		if (iniciar){
			
			char msg[48];
			sprintf (msg, "*T%.1f", temperaturaProm);  //envio un valor despues de la coma
			if (fiebre){
				sprintf (msg, "*fTemperatura promedio elevada");
				BleSendString(msg);
				sprintf (msg, "*gPuede ser que hayas tenido fiebre");
				BleSendString(msg);
			}
			else {
				sprintf (msg, "*fTemperatura promedio normal");
				BleSendString(msg);
			}
			if (estresHRV){
				sprintf (msg, "*pPareces estar estresado, realiza esto:");  //23 carac
				BleSendString(msg);
				sprintf (msg, "*sInhalar 4 segundos");
				BleSendString(msg);
				sprintf (msg, "*tMantener 4 segundos");
				BleSendString(msg);
				sprintf (msg, "*cExhalar. Esperar 4 segundos y repetir");
				BleSendString(msg);
				sprintf (msg, "*qRealizarlo por 5 minutos");
				BleSendString(msg);
			}
			else {
				sprintf (msg, "*pNo estas bajo estres agudo");
				BleSendString(msg);
				sprintf (msg, "*sPodes seguir en lo tuyo..");
				BleSendString(msg);
			}
			vTaskDelay (CONFIG_PROCES_DELAY / portTICK_PERIOD_MS);
		}

	}

}
*/
/*==================[external functions definition]==========================*/
void app_main(void){
	
	ble_config_t ble_configuration = {
		"ESP_EDU_FACUM",
		read_ble
	};
	MAX3010X_begin();
	MAX3010X_setup(30, 1, 2, SAMPLE_FREQ, 69, 4096);
	SwitchesInit();
	LedsInit();
	BleInit(&ble_configuration);


	analog_input_config_t adc_config = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
	};
	AnalogInputInit (&adc_config);
	/*timer_config_t timer1 = {
		.timer = TIMER_A,
		.period = CONFIG_TIMER1_US,
		.func_p = FuncTimer1,
		.param_p = NULL
	};
	TimerInit (&timer1);*/

	

	/*xTaskCreate (&utilizarSensorTask, "SENSORMAX", 512, NULL, 5, &utilizarSensor_task_handle);
	xTaskCreate (&medirTemperaturaTask, "TEMP", 512, NULL, 5, &medirTemperatura_task_handle);
	xTaskCreate (&procesamientoDatosTask, "DATOS", 512, NULL, 5, &procesamientoDatos_task_handle);
	xTaskCreate (&transmitirDatosTask, "ENVIO", 512, NULL, 5, &transmitirDatos_task_handle);
	*/

	//TimerStart (timer1.timer);

	while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
                LedOff(LED_BT);
            break;
            case BLE_DISCONNECTED:
                LedToggle(LED_BT);
            break;
            case BLE_CONNECTED:
                LedOn(LED_BT);
            break;
        }
		if (iniciar) {
		uint32_t sample_count = 0;
			printf ("tomando muestras\n");
			sprintf (msg, "*mtomando muestras\n");
			BleSendString(msg);
			while (sample_count < BUFFER_SIZE){

				while (MAX3010X_available() == false)
					MAX3010X_check();

				irBuffer[sample_count] = MAX3010X_getIR();
				MAX3010X_nextSample(); 

				sample_count++;
			}
			terminoSensor = true;
			//printf ("termina muestras\n");
			//vTaskDelay (CONFIG_MAX_DELAY / portTICK_PERIOD_MS);
		}

	if (iniciar){
		
		
		
		
			AnalogInputReadSingle (CH1, &temp_inst);  //lectura temperatura instantanea.
		temperaturaVect[temp_count] = temp_inst;
		temp_inst = temp_inst / 10;
		printf ("temperatura: %u", temp_inst);
		temp_count++;
		if (temp_count == 5 || temp_count > 23){
			terminoLM = true;	
			temp_count = 0;
		}
		
		
		
		//vTaskDelay (1000 / portTICK_PERIOD_MS);
		
	
	
	}
	

		//vTaskDelay (CONFIG_PROCES_DELAY / portTICK_PERIOD_MS);

	if (iniciar){
		
					//sprintf (msg, "*mmidiendo..");
			if (terminoSensor && terminoLM){
				calculointervalos(irBuffer, bufferLength, &heartRate, &validHeartRate);
				calculoVariabilidad();
				calculoTemperaturaProm (temperaturaVect);
				if (HRV < 25){
				estresALTO = true;
				printf ("estresalto\n");
			}
			else if (HRV > 25 && HRV < 50) {
				estresMEDIO = true;
			} 
			else {
				estresALTO = false;
				estresMEDIO = false;
			}

			if (diferenciaTemp > 15){  //1500 equivale a 1,5°c
				fiebre = true;
			}
			else fiebre = false;

			//char msg[48];
			//sprintf (msg, "*lmidiendo..");
			printf ("*T%.1f", temperaturaProm);  //envio un valor despues de la coma
			if (fiebre){
				printf ("Temperatura promedio elevada\n");
				
				printf ( "Puede ser que hayas tenido fiebre\n\n\n\n\n");
				
			}
			else {
				printf ( "*Temperatura promedio normal\n");
				
			}
			if (estresALTO){
				printf ("ALTO NIVEL DE ESTRES realiza esto: \n");  //23 carac
				
				printf ( "Inhalar 4 segundos\n");
				
				printf ( "Mantener 4 segundos\n");
				
				printf ( "Exhalar. Esperar 4 segundos y repetir\n");
				
				printf ( "Realizarlo por 5 minutos\n\n\n\n\n");
				
			}
			else if (estresMEDIO){
				printf ("MEDIO NIVEL DE ESTRES igual realiza esto:\n");  //23 carac
				
				printf ("Inhalar 4 segundos\n");
				
				printf ( "Mantener 4 segundos\n");
				
				printf ( "Exhalar. Esperar 4 segundos y repetir\n");
				
				printf ( "Realizarlo por 5 minutos\n\n\n\n\n");
				
			} 
			else {
				printf ("No estas bajo estres agudo\n");
				
				printf ( "Podes seguir en lo tuyo..\n\n\n\n\n");
				
			}
			}
			
				
	

		
	}
			
    }

	SwitchActivInt (SWITCH_1, &cambia_iniciar, NULL); //esto en realidad hacerlo con el bluetooth
}
/*==================[end of file]============================================*/