/*! @mainpage Parcial Main
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | sensorHUMEDAD 	| 	2			|
 * |  bombaAGUA	 	| 	3			|
 * | bombaPHA	 	| 	4			|
 * | bombaPHB	 	| 	5			|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/06/2024 | Document creation		                         |
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
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/

/** @def TIMER2_CONFIG_US
 * @brief Indica el tiempo en microsegundos del timer2 para trasmitir
*/
#define TIMER2_CONFIG_US 5000000  //5 segundos

/** @def TIMER1_CONFIG_US
 * @brief Indica el tiempo en microsegundos del timer1 para trasmitir
*/
#define TIMER1_CONFIG_US 3000000 // 3 segundos

/** @var iniciar
 *  @brief bandera que permite iniciar o parar el programa
*/
bool iniciar = 0;

/** @var bombaA
 *  @brief bandera para ver si esta encenddida la bombaA
*/
bool bombaA = 0;

/** @var bombaB
 *  @brief bandera par aver si esta encencdida la bombaB
*/
bool bombaB = 0;

/** @var humedad
 *  @brief bandera para ver si necesita agua o no
*/
bool humedad = 0;

/** @var valorAD
 *  @brief variable que indica valor AD de ph
*/
uint16_t valorAD = 0;

/** @var gpio_t
 *  @brief son los diferentes gpios a utilizar
*/
static gpio_t gpioHumedad, gpioBombaAGUA, gpioBombapHA, gpioBombapHB;

/*==================[internal data definition]===============================*/

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t uart_send_handle = NULL;

/*==================[internal functions declaration]=========================*/

/** @fn void FuncTimerA (void *vParameter)
* @brief Funcion que utiliza el timer para llamar a las tareas.
* @param[in] vParameter puntero tipo void 
*/
void FuncTimer1 (void* param){
	vTaskNotifyGiveFromISR (medir_task_handle, pdFALSE);
	
}

/** @fn void FuncTimerA (void *vParameter)
* @brief Funcion que utiliza el timer para llamar a las tareas.
* @param[in] vParameter puntero tipo void 
*/
void FuncTimer2 (void* param){
	vTaskNotifyGiveFromISR (uart_send_handle, pdFALSE);
}

/** @fn bool SensorHumInit (gpio_t pHumedad, gpio_t pBombaAG)
* @brief inicializador del sensor de humedad
* @param[in] pHumedad es el gpio implicado en el sensor
* @param[in] pBombaAG es el gpio implicado en la bomba de agua
*/
bool SensorHumInit (gpio_t pHumedad, gpio_t pBombaAG){
	gpioHumedad = pHumedad;
	gpioBombaAGUA = pBombaAG;

	GPIOInit (pHumedad, GPIO_INPUT);
	GPIOInit (pBombaAG, GPIO_OUTPUT);

	return true;
}

/** @fn void SensorHumFunc
* @brief sensor de humedad en funcionamiento
*/
void SensorHumFunc (){

	bool necesitaHUM = true;
	bool noNecesitaHUM = true;
	// gpioHumedad es true si necesita humedad
	// gpioHumedad es false si esta bien
	if (necesitaHUM){
		GPIOOn (gpioHumedad);
		humedad = true;
	}
	if (noNecesitaHUM){
		GPIOOff (gpioHumedad);
		humedad = false;
	}
}

/** @fn bool BombapHAInit (gpio_t pbombaHA)
* @brief inicializador de la bomba de phA
* @param[in] pBombapHA es el gpio de la bombaPHA
*/
bool BombapHAInit (gpio_t pBombapHA){
	gpioBombapHA = pBombapHA;
	GPIOInit (pBombapHA, GPIO_INPUT);

	return true;
}

/** @fn bool BombapHBInit (gpio_t pBombapHB)
* @brief inicializador de la bomba de phB
* @param[in] pBombapHA es el gpio de la bombaPHB
*/
bool BombapHBInit (gpio_t pBombapHB){
	gpioBombapHB = pBombapHB;
	GPIOInit (pBombapHB, GPIO_INPUT);

	return true;
}

/** @fn void enciendoBombapHA
* @brief enciendo la bomba phA
*/
void enciendoBombapHA (){
	bombaA = true;
	GPIOOn (gpioBombapHA);
}

/** @fn void enciendoBombapHB
* @brief enciendo la bomba phB
*/
void enciendoBombapHB (){
	bombaB = true;
	GPIOOn (gpioBombapHB);
}

/** @fn void bombasPHApagadas
* @brief apagos las bombas de ph
*/
void bombasPHApagadas (){
	GPIOOff (gpioBombapHA);
	GPIOOff (gpioBombapHB);
}

/** @fn void SensorPHfunc 
* @brief El sensor de ph en funcionamiento
*/
void SensorPHfunc (){
	AnalogInputReadSingle (CH1, &valorAD); //valor AD lee de 0 a 3000
	if (valorAD < 1285){
		enciendoBombapHB();
		
	}
	if (valorAD > 1435){
		enciendoBombapHA();
	}
	if (valorAD < 1435 && valorAD > 1285){
		bombasPHApagadas();
	}
}

/** @fn void bombaAGUAAapagada
* @brief bomba de agua apagada
*/
void bombaAGUAAapagada (){
	GPIOOff (gpioBombaAGUA);
}

/** @fn void encenderBombaAGUA
* @brief enciendo bomba de agua
*/
void encenderBombaAGUA (){
	
	if (GPIORead(gpioHumedad)){
		GPIOOn (gpioBombaAGUA);
	}
	else bombaAGUAAapagada();

}

/** @fn void  iniciarTrue 
* @brief comienza a funcionar 
*/
static void iniciarTrue (){
	iniciar = true;
}

/** @fn void iniciarFalse
* @brief deja de funcionar 
*/
static void iniciarFalse (){
	iniciar = false;
}

/** @fn void medirTask
* @brief tarea que realiza la medicion
* @param[in] vParameter puntero tipo void.
*/
static void medirTask (void *vParameter){

	while (true)
	{
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY);
		if (iniciar){

			SensorPHfunc();
			SensorHumFunc();

		}
	}
	
}

/** @fn static void uartTaskSend (void *vParameter)
* @brief Tarea que permite mandar datos a traves de puerto serie.
* @param[in] vParameter puntero tipo void.
*/
static void uartTaskSend (void *vParameter){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (iniciar){
			UartSendString (UART_PC, "pH ");
			uint8_t *ptrDatos = UartItoa (valorAD, 10);
			UartSendString(UART_PC, (char *) ptrDatos);
			if (humedad == false) {
				UartSendString (UART_PC, ", humedad correcta.\n");
			}
			 //falto seguir con la logica de mensajes
		}
		
	}
}


/*==================[external functions definition]==========================*/
void app_main(void){


	timer_config_t timer1 = {
		.timer = TIMER_A,
		.period = TIMER1_CONFIG_US,
		.func_p = FuncTimer1,
		.param_p = NULL
	};
	TimerInit (&timer1);

	timer_config_t timer2 = {
		.timer = TIMER_B,
		.period = TIMER2_CONFIG_US,
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
	};
	AnalogInputInit (&adc_config);

	SensorHumInit (2, 3);
	BombapHAInit (4);
	BombapHBInit (5);
	SwitchesInit ();

	SwitchActivInt (SWITCH_1, &iniciarTrue, NULL );
	SwitchActivInt (SWITCH_2, &iniciarFalse, NULL );

	xTaskCreate (&medirTask, "ADC", 512, NULL, 5, &medir_task_handle);
	xTaskCreate (&uartTaskSend, "UART", 512, NULL, 5, &uart_send_handle);

	TimerStart (timer1.timer);
	TimerStart (timer2.timer);

}
/*==================[end of file]============================================*/