/*! @mainpage guia2_Ej2
 *
 * @section genDesc General Description
 *
 * Firmware que permite medir la distancia en centimetros con un sensor de ultrasonido e informar la misma a traves de una
 * pantalla LCD y el prendido y apagado de unos leds dependiendo de esta distancia. Se utilizan interrupciones con los switches
 * para comenzar y parar la medicion, y tambien para mantener la pantalla congelada. Además se utiliza un timer para 
 * realizar la medida a una velocidad deseada.
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
/*==================[macros and definitions]=================================*/

bool iniciar = false;
bool hold = false;
uint16_t distancia = 0;

#define CONFIG_TIMER1_US 500000

/*==================[internal data definition]===============================*/

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;

/*==================[internal functions declaration]=========================*/



void FuncTimerA (void* param){
	vTaskNotifyGiveFromISR (medir_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (lcd_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR (led_task_handle, pdFALSE);
}


static void cambia_Iniciar (){
	iniciar =! iniciar;
}

static void cambiar_Hold (){
	hold =! hold;
}

static void medirTask (void *vParameter){
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (iniciar){
			distancia = HcSr04ReadDistanceInCentimeters();
		}
		
	}
	
}

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

	SwitchActivInt (SWITCH_1, &cambia_Iniciar, NULL );
	SwitchActivInt (SWITCH_2, &cambiar_Hold, NULL );

	xTaskCreate(&medirTask, "MEDIR", 512, NULL, 5, &medir_task_handle);
	xTaskCreate(&muestraLCD, "LCD", 512, NULL, 5, &lcd_task_handle);
	xTaskCreate(&muestraLEDS, "LEDS", 512, NULL, 5, &led_task_handle);

	TimerStart (timerA.timer);
}
/*==================[end of file]============================================*/