/*! @mainpage guia2_Ej1
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
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 6/4/2023 | Document creation		                         |
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
/*==================[macros and definitions]=================================*/

bool iniciar = false;
bool hold = false;
uint16_t distancia = 0;

#define CONFIG_MEDIR_DELAY 1000
#define CONFIG_TECLAS_DELAY 200
#define CONFIG_LCD_DELAY 1000
#define CONFIG_LEDS_DELAY 1000

/*==================[internal data definition]===============================*/

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;

/*==================[internal functions declaration]=========================*/

static void teclasTask (void *vParameter){
	uint8_t teclas;
	while (true){
		teclas = SwitchesRead();

		if (teclas & SWITCH_1){
			iniciar =! iniciar;
		}
		else if (teclas & SWITCH_2){
			hold =! hold;
		}
		vTaskDelay (CONFIG_TECLAS_DELAY / portTICK_PERIOD_MS);
	}
	
}

static void medirTask (void *vParameter){
	while (true){
		
		if (iniciar){
			distancia = HcSr04ReadDistanceInCentimeters();
		}
		
		vTaskDelay (CONFIG_MEDIR_DELAY / portTICK_PERIOD_MS);
	}
	
}

static void muestraLCD (void *vParameter){
	while (true)
	{
		if (iniciar){
			//LcdItsE0803Write (distancia);
			if (hold){

			} else {
				LcdItsE0803Write (distancia);
			}
		}
		else {
			LcdItsE0803Off();
		}
		vTaskDelay (CONFIG_LCD_DELAY / portTICK_PERIOD_MS);
	}
	
}

static void muestraLEDS (void *vParameter){

	while (true){

		if (iniciar){
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
		vTaskDelay (CONFIG_LEDS_DELAY / portTICK_PERIOD_MS);
		}
		else {
			LedsOffAll();
			vTaskDelay (CONFIG_LEDS_DELAY / portTICK_PERIOD_MS);
		}

	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	SwitchesInit();
	HcSr04Init(3, 2);
	LcdItsE0803Init();
	xTaskCreate(&teclasTask, "TECLAS", 512, NULL, 5, &teclas_task_handle);
	xTaskCreate(&medirTask, "MEDIR", 512, NULL, 5, &medir_task_handle);
	xTaskCreate(&muestraLCD, "LCD", 512, NULL, 5, &lcd_task_handle);
	xTaskCreate(&muestraLEDS, "LEDS", 512, NULL, 5, &led_task_handle);
}
/*==================[end of file]============================================*/