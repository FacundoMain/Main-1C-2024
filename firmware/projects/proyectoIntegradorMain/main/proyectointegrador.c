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
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <max3010x.h>
#include <spo2_algorithm.h>
#include <math.h>
#include "switch.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/

#define MAX_PEAKS 40
#define BUFFER_SIZE_DATA 2000
#define SAMPLE_FREQ 100

uint16_t temperatura;
float HRV = 0.0;
bool estresHRV = false;
bool estresTemp = false;
bool estresTotal = false;
bool iniciar = false;

int32_t peak_intervals[MAX_PEAKS];  //almacena intervalos entre picos de señal ppg
int32_t peak_count = 0;  //contador de picos se señal ppg

uint32_t irBuffer[BUFFER_SIZE_DATA]; //infrared LED sensor data
int32_t bufferLength=BUFFER_SIZE_DATA; //data length
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

/*==================[internal data definition]===============================*/

TaskHandle_t utilizarSensor_task_handle = NULL;
TaskHandle_t medirTemperatura_task_handle = NULL;
TaskHandle_t detectarNivelEstres_task_handle = NULL;
TaskHandle_t transmitirDato_task_handle = NULL;


/*==================[internal functions declaration]=========================*/

static void cambia_iniciar (){
	iniciar =! iniciar;
}

void utilizarSensorTask (void *vParameter){
	while (true){
		if (iniciar) {
			uint8_t i;
			for (i = 25; i < BUFFER_SIZE_DATA; i++)
			{
				irBuffer[i - 25] = irBuffer[i];
			}

			//take 25 sets of samples before calculating the heart rate.
			for ( i = BUFFER_SIZE_DATA - 25 ; i < BUFFER_SIZE_DATA ; i++)
			{
				while (MAX3010X_available() == false) //do we have new data?
					MAX3010X_check(); //Check the sensor for new data
				
				irBuffer[i] = MAX3010X_getIR();
				MAX3010X_nextSample(); //We're finished with this sample so move to next sample        

			}
		}
	}
}

void calculointervalos (uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, int32_t *pn_heart_rate, int8_t *pch_hr_valid){

	uint32_t un_ir_mean;  
	int32_t k;
	int32_t i, n_exact_ir_valley_locs_count;
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


void medirVariabilidad (void *vParameter){
	// se usa la desviacion estandar de los intervalos RR (SDNN)
	if (iniciar){
		calculointervalos(irBuffer, bufferLength, &heartRate, &validHeartRate);
		//if (peak_count < 2) // No suficiente datos
		//return -1;
		//; 

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
	}
	
}

void medirTemperaturaTask (void *vParameter){
	while (true){
		AnalogInputReadSingle (CH1, &temperatura);
	}
	
}

void detectarNivelEstresTask (void *vParameter){

	while (true) {
		
		if (iniciar){

			if (HRV < 25){
				estresHRV = true;
			}
			else estresHRV = false;
			if (temperatura < 3700){
				estresTemp = true;
			}
			else estresTemp = false;

			if (estresHRV && estresTemp){
				estresTotal = true;
			}
		}
	}
			
}

void transmitirDatoTask (void *vParameter){


}

/*==================[external functions definition]==========================*/
void app_main(void){
	
	MAX3010X_begin();
	MAX3010X_setup(30, 1, 2, SAMPLE_FREQ, 69, 4096);
	SwitchesInit();

	SwitchActivInt (SWITCH_1, &cambia_iniciar, NULL);
}
/*==================[end of file]============================================*/