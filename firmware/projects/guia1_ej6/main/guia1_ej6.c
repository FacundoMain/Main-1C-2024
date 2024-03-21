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
 * | 21/03/2024 | Document creation		                         |
 *
 * @author Facundo Main (facundo.main@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <gpio_mcu.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
void muestradigito (uint8_t digito, gpioConf_t *gpios){
	
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

void muestra_numero ( uint32_t pData, uint8_t pDigitos, gpioConf_t *gpios_digito, gpioConf_t *seleccion_bcd ){

	pDigitos = 0;
	uint8_t bcd_number[3];
	while (pData != 0)
	{		
		bcd_number[pDigitos] = pData % 10;  //456 % 10 = 6
		pData = pData / 10;  // data ahora es 45
		++pDigitos;
	}

	gpios_digito[0].pin = GPIO_20;
	gpios_digito[1].pin = GPIO_21;
	gpios_digito[2].pin = GPIO_22;
	gpios_digito[3].pin = GPIO_23;

	seleccion_bcd[0].pin = GPIO_19;
	seleccion_bcd[1].pin = GPIO_18;  //esto puede quedar alreves
	seleccion_bcd[2].pin = GPIO_9;

	for (uint8_t i = 0 ; i < 3 ; ++i){
		
		seleccion_bcd[i].dir = GPIO_OUTPUT;
		GPIOInit (seleccion_bcd[i].pin, seleccion_bcd[i].dir);
		muestradigito (bcd_number[i], gpios_digito);
		GPIOOn (seleccion_bcd[i].pin);
		GPIOOff (seleccion_bcd[i].pin);  //esto antes o despues del muestra digito

	}
}

/*==================[external functions definition]==========================*/
void app_main(void){

	uint32_t numero = 456;
	uint8_t digitos = 0;
	gpioConf_t gpios[4];
	gpioConf_t seleccion_bcd[3];

	muestra_numero (numero, digitos, gpios, seleccion_bcd);
}
/*==================[end of file]============================================*/