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
#include <gpio_mcu.h>
/*==================[macros and definitions]=================================*/

//Escribir una función que reciba 
//como parámetro un dígito BCD y un vector de estructuras del tipo gpioConf_t. Incluya el archivo de cabecera gpio_mcu.h

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/

void muestradigito (uint8_t digito, gpioConf_t *gpios){
	
	gpios[0].pin = GPIO_20;
	gpios[1].pin = GPIO_21;
	gpios[2].pin = GPIO_22;
	gpios[3].pin = GPIO_23;

	for (uint8_t i = 0 ; i < 4 ; ++i){
		gpios[i].dir = GPIO_OUTPUT;
		GPIOInit (gpios[i].pin, gpios[i].dir);
	}

	for (uint8_t i = 0 ; i<4 ; ++i ){
		if ((digito & (1 << i)) == 0)   
		{
			GPIOOff (gpios[i].pin);
		}
		else GPIOOn (gpios[i].pin);
		
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){
	
	uint8_t numero = 6 ;
	gpioConf_t gpios[4];
	muestradigito (numero, gpios);

}
/*==================[end of file]============================================*/