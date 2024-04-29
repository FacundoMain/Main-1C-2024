/*! @mainpage Guia2_ej4
 *
 * @section genDesc General Description
 *
 * FIrmware que permite convertir datos analogicos a digitales, y viceversa.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
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
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/

/** @def BUFFER_SIZE
 * @brief indica el tamaño del buffer del vector ecg
*/
#define BUFFER_SIZE 231

/** @def CONFIG_TIMER_US
 * @brief Indica el tiempo en microsegundos del timer1 para el ADC
*/
#define CONFIG_TIMER_US 2000 //1/500hz = 2000 microseg

/** @def CONFIG_TIMER2_US
 * @brief Indica el tiempo en microsegundos del timer2 para el DAC
*/
#define CONFIG_TIMER2_US 4329 // 1/231hz = 4329 microseg
/*==================[internal data definition]===============================*/
//TaskHandle_t main_task_handle = NULL;
TaskHandle_t uart_task_handle = NULL;
TaskHandle_t adc_task_handle = NULL;
TaskHandle_t dac_task_handle = NULL;

/** @var ecg[]
 *  @brief vector ecg que simula un ecg.
*/
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/** @var valor
 *  @brief Variable global valor que se usa para el ADC
*/
uint16_t valor;

/** @var distancia
 *  @brief Variable global que permite el recorrido del vector ecg
*/
int contador = 0;
/*==================[internal functions declaration]=========================*/

/** @fn void FuncTimer (void *vParameter)
* @brief Funcion que utiliza el timer para llamar a las tareas.
* @param[in] vParameter puntero tipo void 
*/
void FuncTimer (void *vParameter){
	vTaskNotifyGiveFromISR (adc_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (uart_task_handle, pdFALSE);
}	

/** @fn void FuncTimer2 (void *vParameter)
* @brief Funcion que utiliza el timer2 para llamar a la tarea DAC
* @param[in] vParameter puntero tipo void 
*/
void FuncTimer2 (void *vParameter){
	vTaskNotifyGiveFromISR (dac_task_handle, pdFALSE);
}

/** @fn void adcTask (void *vParameter)
* @brief Tarea que realiza la lectura de datos analogicos para convertirlos en digitales
* @param[in] vParameter puntero tipo void 
*/
void adcTask (void *vParameter){
	while (true){
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY);		
		AnalogInputReadSingle (CH1, &valor);
	}
}

/** @fn void uartTask (void *vParameter)
* @brief Tarea que envia el dato digital por puerto serie.
* @param[in] vParameter puntero tipo void 
*/
void uartTask (void *vParameter){
	while (true){
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY);
		uint8_t *dato_ascii;
		dato_ascii = UartItoa (valor, 10);  //10 base del valor es decimal
		
		UartSendString (UART_PC, (char *)dato_ascii);
		UartSendString (UART_PC, "\r");

	}

}

/** @fn void dacTask (void *vParameter)
* @brief Tarea que realiza la conversion de datos digitales a analogicos.
* @param[in] vParameter puntero tipo void 
*/
void dacTask (void *vParameter){
	while (true){
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY);
		
		AnalogOutputWrite (ecg[contador]);

		if (contador < 230){
			++contador;
		}
		else contador = 0;
	}
	
}

/*==================[external functions definition]==========================*/
void app_main(void){
	
	timer_config_t timer = {
		.timer = TIMER_A,
		.period = CONFIG_TIMER_US,
		.func_p = FuncTimer,
		.param_p = NULL
	};
	TimerInit (&timer);

	timer_config_t timer2 = {
		.timer = TIMER_B,
		.period = CONFIG_TIMER2_US,
		.func_p = FuncTimer2,
		.param_p = NULL
	};
	TimerInit (&timer2);

	serial_config_t puertoSerie = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};
	UartInit (&puertoSerie);

	analog_input_config_t adc_config = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		//.sample_frec = NULL
	};
	AnalogInputInit (&adc_config);
	AnalogOutputInit ();
	

	xTaskCreate (&adcTask, "ADC", 512, NULL, 5, &adc_task_handle);
	xTaskCreate (&uartTask, "UART", 512, NULL, 5, &uart_task_handle);
	xTaskCreate (&dacTask, "DAC", 512, NULL, 5, &dac_task_handle);

	TimerStart (timer.timer);
	TimerStart (timer2.timer);

}
/*==================[end of file]============================================*/