/*! @mainpage Template
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
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Facundo Main (facundo.main@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led.h>

/*==================[macros and definitions]=================================*/
#define ON 1
#define OFF 2
#define TOGGLE 3
#define CONFIG_BLINK_PERIOD 100

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
struct leds
{
    uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;       // indica el número de led a controlar
	uint8_t n_ciclos;    //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
} my_leds;

void controlleds (struct leds * led_ptr){
	// diagrama de flujo 
	if (led_ptr->mode == ON){
		if (led_ptr -> n_led == 1)
		LedOn (led_ptr -> n_led);
		else if (led_ptr -> n_led == 2)
		LedOn (led_ptr -> n_led);
		else LedOn (led_ptr -> n_led);
	
	}
	else {
		if (led_ptr -> mode == OFF){
			if (led_ptr -> n_led == 1)
			LedOff (led_ptr -> n_led);
			else if (led_ptr -> n_led == 2)
			LedOff (led_ptr -> n_led);
			else LedOff (led_ptr -> n_led);
			
		}
		else{ //falta probar que funcione bien el toggle.
			if (led_ptr -> mode == TOGGLE){	
				//printf("modo toggle");		
				for (int i = 0; i < led_ptr->n_ciclos ; ++i){
					if (led_ptr -> n_led == 1){
					LedToggle (led_ptr->n_led);											
					} 
					else if (led_ptr -> n_led == 2){
						LedToggle (led_ptr->n_led);					
						}
						else {						
						LedToggle (led_ptr->n_led);
						}
					//printf ("led toggle");			
					for (int j = 0; j < led_ptr->periodo/100 ; ++j){
							vTaskDelay (CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
						}				
				}
				
					
			}
		}
	}
	

}
/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	my_leds.mode = TOGGLE;
	my_leds.n_led = 2;
	my_leds.n_ciclos = 10;
	my_leds.periodo = 500;

	controlleds (& my_leds);

	//printf("Hello world!\n");
}
/*==================[end of file]============================================*/