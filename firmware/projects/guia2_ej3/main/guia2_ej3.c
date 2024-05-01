/*! @mainpage guia2_Ej2
 *
 * @section genDesc General Description
 *
 * Firmware que permite medir la distancia en centimetros con un sensor de ultrasonido e informar la misma a traves de una
 * pantalla LCD y el prendido y apagado de unos leds dependiendo de esta distancia. Se utiliza la conexion mediante un puerto
 * serial para transmitir los datos de distancia y recibir cuando se aprietan teclas para encencido y apago, y mantener
 * la distancia medida.
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
 * | 11/4/2023 | Document creation		                         |
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
#include "led.h"
#include "hc_sr04.h"
#include "switch.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/

/** @var iniciar
 * @brief Bandera que indica el estado del programa. 
*/
bool iniciar = false;

/** @var hold
 * @brief Bandera que indica si la pantalla debe congelarse.
*/
bool hold = false;

/** @var distancia
 *  @brief Variable global distancia.
*/
uint16_t distancia = 0;

/** @var pulgadas
 *  @brief Bandera que indica si la medicion se hará en pulgadas.
*/
bool pulgadas = false;

/** @def CONFIG_TIMER1_US
 * @brief Indica el tiempo en microsegundos del timer.
*/
#define CONFIG_TIMER1_US 200000

/** @def BASE
 * @brief Indica la base del sistema numerico para el cual se realiza conversion de un numero a string
*/
#define BASE 10
/*==================[internal data definition]===============================*/

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;
TaskHandle_t uart_tasksend_handle = NULL;

/*==================[internal functions declaration]=========================*/

/** @fn void FuncTimerA (void *vParameter)
* @brief Funcion que utiliza el timer para llamar a las tareas.
* @param[in] vParameter puntero tipo void 
*/
void FuncTimerA (void* param){
	vTaskNotifyGiveFromISR (medir_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (lcd_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (led_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (uart_tasksend_handle, pdFALSE);	
}

/** @fn static void cambia_Iniciar ()
* @brief Funcion que cambia la bandera iniciar.
* @param[in] vParameter puntero tipo void 
*/
static void cambia_Iniciar (){
	iniciar =! iniciar;
}

/** @fn static void cambia_Hold ()
* @brief Funcion que cambia la bandera hold.
* @param[in] vParameter puntero tipo void 
*/
static void cambiar_Hold (){
	hold =! hold;
}
/** @fn static void medirTask (void *vParameter)
* @brief Funcion que comienza a medir.
* @param[in] vParameter puntero tipo void 
*/
static void medirTask (void *vParameter){
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (iniciar && pulgadas == false){
			distancia = HcSr04ReadDistanceInCentimeters();
		}
		if (iniciar && pulgadas == true){
			distancia = HcSr04ReadDistanceInInches();
		}		
	}	
}

/** @fn static void muestraLCD (void *vParameter)
* @brief Tarea que muestra por la pantalla el valor de la distancia medido. 
* @param[in] vParameter puntero tipo void.
*/
static void muestraLCD (void *vParameter){
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		if (iniciar){		
			
			if (hold){
			} else {
				LcdItsE0803Write (distancia);
			}
		}
		else {
			LcdItsE0803Off();
		}
		
	}
	
}

/** @fn static void muestraLEDS (void *vParameter)
* @brief Tarea que prende y apaga los leds dependiendo la distancia. 
* @param[in] vParameter puntero tipo void.
*/
static void muestraLEDS (void *vParameter){

	while (true){

		if (iniciar){
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			uint8_t opcion = 0;
			if (distancia < 10){
				opcion = 1;
			} else {
				if (distancia > 10 && distancia < 20){
				opcion = 2;
				}
				else {
					if (distancia > 20 && distancia < 30){
						opcion = 3;
					}
					else {
						opcion = 4;
					}
				}
				
			}

			switch (opcion)
			{
			case 1:
				LedsOffAll();
			break;
			case 2:
				LedOn (LED_1);
				LedOff (LED_2);
				LedOff (LED_3);
			break;
			case 3:
				LedOn (LED_1);
				LedOn (LED_2);
				LedOff (LED_3);
			break;
			case 4:
				LedOn (LED_1);
				LedOn (LED_2);
				LedOn (LED_3);
			break;
			default:
			break;
			}
		
		}
		else {
			LedsOffAll();
			
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
		uint8_t *ptrDatos = UartItoa (distancia, BASE);
		UartSendString(UART_PC, (char *) ptrDatos);
		if (pulgadas == false){
			UartSendString(UART_PC, " cm\r\n");
		}
		if (pulgadas == true){
			UartSendString(UART_PC, " pul\r\n");
		}
	}
}

/** @fn static void uartTaskRead (void *vParameter)
* @brief Tarea que permite recibir datos a traves de puerto serie.
* @param[in] vParameter puntero tipo void.
*/
static void uartRead (void *vParameter){
		
		uint8_t dato_leido = 0; 
		UartReadByte (UART_PC, &dato_leido);
		uint8_t asciiO = 79;
		uint8_t asciiH = 72;

		if (dato_leido == asciiO){
			cambia_Iniciar();
		}
		if (dato_leido == asciiH){
			cambiar_Hold();
		}
        
		if (dato_leido == 'I'){
			pulgadas =! pulgadas;
		}
			
	
}

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	SwitchesInit();
	HcSr04Init(3, 2);
	LcdItsE0803Init();

	timer_config_t timerA = {
		.timer = TIMER_A,
		.period = CONFIG_TIMER1_US,
		.func_p = FuncTimerA,
		.param_p = NULL
	};
	TimerInit (&timerA);

	serial_config_t puertoSerie = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = uartRead,
		.param_p = NULL
	};
	UartInit (&puertoSerie);

	xTaskCreate(&medirTask, "MEDIR", 512, NULL, 5, &medir_task_handle);
	xTaskCreate(&muestraLCD, "LCD", 512, NULL, 5, &lcd_task_handle);
	xTaskCreate(&muestraLEDS, "LEDS", 512, NULL, 5, &led_task_handle);
	xTaskCreate(&uartTaskSend, "UARTSEND", 512, NULL, 5, &uart_tasksend_handle);

	TimerStart (timerA.timer);
}
/*==================[end of file]============================================*/