/*! @mainpage Proyecto Integrador
 *
 * @section genDesc General Description
 *		Medidor de estrés y temperatura.
 *	Mediante sensor MAX30102 se medira el pulso de la sangre, con esto se calculara
 *  la variablidad de la frecuencia cardíaca (HRV) entre latidos. Este parametro nos da informacion
 * sobre el estado del sistema nervioso autonómico, que midiendo en reposo, puede ser un indicador 
 * del estado de estres de la persona. Además con el LM35 se medirá la temperatura corporal, en esta 
 * aplicacion se hara cada un segundo, sin embargo a modo de idea, se mediría cada una hora, y 
 * se realizarán calculos sobre temperatura promedio y diferencias de temperatura entre cada elemento.
 * Luego, con estos parametros mediante comunicacion Bluetooth, se informará la temperatura promedio 
 * y una posible fiebre; también se informará si la persona esta bajo estrés o no, indicando
 * ejercicios de respiración para disminuirlo. 
 *  NO funcionó conexion bluetooth.
 * 	
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	LM35 Vin	| 		3V3		|
 * | 	LM35 GND	| 		GND		|
 * | 	LM35 OUT	| 		CH1		|	
 * | MAX30102 Vin	| 		3V3		|
 * | MAX30102 Vin	| 		GND		|
 * | MAX30102 SCL	| 		SCL		|
 * | MAX30102 SDA	| 		SDA		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 16/05/2024 | Document creation		                         |
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

/** @def MAX_PEAKS 
 * @brief indica el tamaño del vector de picos
*/
#define MAX_PEAKS 40

/** @def DURACION
 * @brief tiempo en segundos que dura la medicion del sensor MAX30102
*/
#define DURACION 15

/** @def SAMPLE_FREQ
 * @brief frecuencia de muestreo de sensor MAX30102
*/
#define SAMPLE_FREQ 25  // 25 muestras por segundos

/** @def BUFFER_SIZE
 * @brief indica la cantidad de muestras que voy a tener
*/
#define BUFFER_SIZE (SAMPLE_FREQ * DURACION) 

/** @def CONFIG_PROCES_DELAY
 * @brief tiempo en milisegundos de delay luego del procesamiento de datos
*/
#define CONFIG_PROCES_DELAY 10000 

/** @def CONFIG_BLINK_PERIOD
 * @brief Periodo de parpadeo del led
*/
#define CONFIG_BLINK_PERIOD 500

/** @def CONFIG_MAX_DELAY
 * @brief Delay luego del uso del sensor MAX30102
*/
#define CONFIG_MAX_DELAY 5000  

/** @def CONFIG_LM_DELAY	
 * @brief Delay en la tarea de medir temperatura 
 */
#define CONFIG_LM_DELAY 1000 

/** @def CONFIG_ENV_DELAY
 * @brief Delay luego de transmitir datos
 */
#define CONFIG_ENV_DELAY 10000

/** @def LED_BT
 * @brief Led que va a parpadear cuando se conecte el bluetooth.
*/
#define LED_BT LED_1

/** @var HRV
 * @brief Variable que indica en milisegundos la variabilidad de la frecuencia cardíaca.
*/
float HRV = 0.0;

/** @var temperaturaPROM
 * @brief variable que indica la temperatura promedio
*/
float temperaturaProm = 0.0;

/** @var diferenciaTemp
 * @brief indica la diferencia entre dos temperaturas sucesivas
*/
int diferenciaTemp = 0;

/** @var temperaturaVECT[]
 * @brief vector donde se almacenan los valores de temperatura
*/
int8_t temperaturaVect[24];

/** @var temp_inst
 * @brief valor de temperatura instantanea que mido con el sensor
*/
uint16_t temp_inst = 0;

/** @var temp_count
 * @brief contador utilizado para calculos en vector temperatura.
*/
int temp_count = 0; 

/** @var estresALTO
 * @brief booliano que indica si esta muy estresado
*/
bool estresALTO = false;

/** @var estresMEDIO
 *  @brief booliano que indica si esta un poco estresado
 */
bool estresMEDIO = false;

/** @var fiebre
 * @brief booliano que indica si la persona pudo haber tenido fiebre
*/
bool fiebre = false;

/** @var iniciar
 * @brief bandera que indica si se inicia la medicion
*/
bool iniciar = false;

/** @var peak_intervals[]
 * @brief vector que almacena los intervalos entre los picos de la señal ppg
*/
int32_t peak_intervals[MAX_PEAKS];  //almacena intervalos entre picos de señal ppg

/** @var peak_count
 * @brief contador de picos de la señal ppg
*/
int32_t peak_count = 0;  //contador de picos se señal ppg

/** @var irBuffer
 * @brief vector donde se almacenan los datos del sensorMAX30102
*/
uint32_t irBuffer[BUFFER_SIZE]; //infrared LED sensor data

/** @var bufferLength
 * @brief cantidad de muestras
*/
int32_t bufferLength=BUFFER_SIZE; //data length

/** @var heartRate
 * @brief valor de frecuencia cardiaca
*/
int32_t heartRate; //heart rate value

/** @var validHeartRate
 * @brief indicador que muestra si el calculo de la frecuencia cardiaca es valido.
*/
int8_t validHeartRate ; //indicator to show if the heart rate calculation is valid

/** @var entradaBle
 * @brief Entrada de datos bluetooth
*/
uint8_t entradaBle ;

/** @var terminoSensor 
 * 	@brief Bandera que indica si termino de medir el sensor MAX30102
 */
bool terminoSensor = false;

/** @var terminoSensorLM
 * 	@brief Bandera que indica si termino de medir el sensor LM35
 */
bool terminoSensorLM = false;

/*==================[internal data definition]===============================*/

TaskHandle_t utilizarSensor_task_handle = NULL;
TaskHandle_t medirTemperatura_task_handle = NULL;
TaskHandle_t procesamientoDatos_task_handle = NULL;
TaskHandle_t transmitirDatos_task_handle = NULL;

/*==================[internal functions declaration]=========================*/

/** @fn static void cambia_Iniciar ()
* @brief Funcion que cambia la bandera iniciar.
*/
static void cambia_iniciar (){
	iniciar =! iniciar;
}


/** @fn void read_ble ( uint8_t *data, uint8_t length)
* @brief Funcion que permite lectura de datos transmitidos desde otro dispositivo via bluetooth.
* @param[in] data Puntero a array de datos recibidos
* @param[in] length Longitud del array de datos recibidos
*/
void read_ble ( uint8_t *data, uint8_t length){ 
	printf ("dato que recibo: %u", data[0]);
	if (data[0] == 'A'){
		
		cambia_iniciar();
	}
}

/** @fn utilizarSensorTask (void *vParameter)
 *  @brief Tarea que realiza uso de sensorMAX30102
 *  @param[in] vParameter puntero tipo void
 */
void utilizarSensorTask (void *vParameter){
	while (true){
		if (iniciar) {
		uint32_t sample_count = 0;
			printf ("tomando muestras\n");
			while (sample_count < BUFFER_SIZE){

				while (MAX3010X_available() == false)
					MAX3010X_check();

				irBuffer[sample_count] = MAX3010X_getIR();
				MAX3010X_nextSample(); 

				sample_count++;
			}
			terminoSensor = true;
			printf ("termina muestras\n");
			vTaskDelay (CONFIG_MAX_DELAY / portTICK_PERIOD_MS);
		}
	}
}

/** @fn void calculoIntervalos (uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, int32_t *pn_heart_rate, int8_t *pch_hr_valid)
 *  @brief Funcion que realiza el calculo de los intervalos de tiempo 
 * @param[in] pun_ir_buffer Puntero a array de datos
 * @param[in] n_ir_buffer_length Longitud de array de datos
 * @param[in] pn_heart_rate Puntero a variable de frecuencia cardiaca
 * @param[in] pch_hr_valid Puntero a valor de frecuencia cardiaca valido
 */
void calculointervalos (uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, int32_t *pn_heart_rate, int8_t *pch_hr_valid){

	uint32_t un_ir_mean;  
	int32_t k;
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

/** @fn calculoVariabilidad ()
 * @brief Funcion que calcula la variabilidad de la frecuencia cardíaca
 */
void calculoVariabilidad (void){
	// se usa la desviacion estandar de los intervalos RR (SDNN)
	
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
	printf ("HRV= %f \n", HRV);
}

/** @fn medirTemperaturaTask (void *vParameter)
 *  @brief Tarea que realiza la medicion de la temperatura con el sensor LM35
 * @param[in] vParameter Puntero tipo void
 */

void medirTemperaturaTask (void *vParameter){
	while (true){
		if (iniciar){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		AnalogInputReadSingle (CH1, &temp_inst);  //lectura temperatura instantanea.
		temperaturaVect[temp_count] = temp_inst;
		printf ("Temperatura inst= %u\n", temp_inst);
		
		temp_count++;
		if (temp_count == 5 || temp_count > 24){
			terminoSensorLM = true;
			temp_count = 0;
		}
		

		vTaskDelay (CONFIG_LM_DELAY / portTICK_PERIOD_MS);
		}
		
	}
	
}

/** @fn calculoTemperaturaProm
 *  @brief Funcion que calcula la temperatura promedio y la diferencia entre
 * 	temperaturas sucesivas
 * @param[in] vecTemp Puntero a array de datos de temperatura
 */
void calculoTemperaturaProm (int8_t *vecTemp){
	int elementosVector = 24;
	int suma = 0;
	int diferenciaaux = 0;
	printf ("entra en calculo temp");
	
	for (int i = 0; i < elementosVector ; i++){
		suma += vecTemp[i];
	}

	for (int i = 1; i < elementosVector ; i++){
		diferenciaaux = vecTemp[i] - vecTemp[i-1];
		if (diferenciaaux > diferenciaTemp){
			diferenciaTemp = diferenciaaux;
		}
	}

	
	//temperaturaProm = temperaturaProm 
	//temperaturaProm = temperaturaProm / 100; 
	temperaturaProm = suma/elementosVector;
	temperaturaProm = temperaturaProm / 10;

	printf ("temperaturaProm = %f \n", temperaturaProm);
}

/** @fn procesamientoDatosTask(void *vParameter)
 *  @brief Tarea que realiza el procesamiento de los datos de temperatura 
 *  y de la señal ppg
 *  @param[in] vParameter puntero tipo void
 */
void procesamientoDatosTask (void *vParameter){
	while(true){
		if (iniciar){
			//printf ("procesamiento datos \n");
			if (terminoSensor && terminoSensorLM){
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

			/* 				PRUEBAS REALIZADAS  
			//char msg[48];
			//sprintf (msg, "*lmidiendo..");
			printf ("*T%.1f", temperaturaProm);  //envio un valor despues de la coma
			if (fiebre){
				printf ("*fTemperatura promedio elevada");
				
				printf ( "*gPuede ser que hayas tenido fiebre");
				
			}
			else {
				printf ( "*fTemperatura promedio normal");
				
			}
			if (estresALTO){
				printf ("ALTO NIVEL DE ESTRES realiza esto: \n");  //23 carac
				
				printf ( "Inhalar 4 segundos\n");
				
				printf ( "Mantener 4 segundos\n");
				
				printf ( "Exhalar. Esperar 4 segundos y repetir\n");
				
				printf ( "Realizarlo por 5 minutos\n");
				
			}
			else if (estresMEDIO){
				printf ("MEDIO NIVEL DE ESTRES igual realiza esto:\n");  //23 carac
				
				printf ("Inhalar 4 segundos\n");
				
				printf ( "Mantener 4 segundos\n");
				
				printf ( "Exhalar. Esperar 4 segundos y repetir\n");
				
				printf ( "Realizarlo por 5 minutos\n");
				
			} 
			else {
				printf ("No estas bajo estres agudo\n");
				
				printf ( "Podes seguir en lo tuyo..\n");
				
			}
			
			*/
			}
				
		}
		vTaskDelay (CONFIG_PROCES_DELAY / portTICK_PERIOD_MS);
	}

}

/** @fn transmitirDatosTask (void *vParameter)
 * 	@brief Tarea que transmite los datos via bluetooth
 * @param[in] vParameter puntero tipo void 
 */

void transmitirDatosTask (void *vParameter){
	while (true) {
		if (iniciar){
			char msg[48];
			sprintf (msg, "*lmidiendo..");
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
			if (estresALTO){
				sprintf (msg, "*pALTO NIVEL DE ESTRES realiza esto:");  //23 carac
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
			else if (estresMEDIO){
				sprintf (msg, "*pMEDIO NIVEL DE ESTRES igual realiza esto:");  //23 carac
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
			vTaskDelay (CONFIG_ENV_DELAY / portTICK_PERIOD_MS);
		}

	}

}

/*==================[external functions definition]==========================*/
void app_main(void){
	
	ble_config_t ble_configuration = {
		"ESP_EDU_FACUM",
		read_ble
	};
	MAX3010X_begin();
	MAX3010X_setup(30, 1, 2, SAMPLE_FREQ, 69, 4096);
	LedsInit();
	BleInit(&ble_configuration);
	SwitchesInit();
	
	analog_input_config_t adc_config = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
	};
	AnalogInputInit (&adc_config);
	SwitchActivInt(SWITCH_1, &cambia_iniciar, NULL);

	xTaskCreate (&utilizarSensorTask, "SENSORMAX", 4096, NULL, 5, &utilizarSensor_task_handle);
	xTaskCreate (&medirTemperaturaTask, "TEMP", 4096, NULL, 5, &medirTemperatura_task_handle);
	xTaskCreate (&procesamientoDatosTask, "DATOS", 4096, NULL, 5, &procesamientoDatos_task_handle);
	xTaskCreate (&transmitirDatosTask, "ENVIO", 512, NULL, 5, &transmitirDatos_task_handle);

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
    }
	
}
/*==================[end of file]============================================*/