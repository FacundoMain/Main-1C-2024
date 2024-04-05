/*! @mainpage guia1_ej6
 *
 * @section genDesc General Description
 *
 * Ejercicio 6 de guia proyecto 1, el mismo consiste en mostrar un numero de 3 digitos
 * por una pantalla LCD.
 *
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_9		|
 * | 	PIN_X	 	| 	GPIO_18		|
 * | 	PIN_X	 	| 	GPIO_19		|
 * | 	PIN_X	 	| 	GPIO_20		|
 * | 	PIN_X	 	| 	GPIO_21		|
 * | 	PIN_X	 	| 	GPIO_22		|
 *| 	PIN_X	 	| 	GPIO_23		|
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

/** @struct gpioConf_t
 * @brief Estructura que guarda la informacion de un GPIO, pin asociado y direccion.
*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/

/** @fn void muestradigito (uint8_t digito, gpioConf_t *gpios)
 * @brief Los gpios se configuran de manera tal que sea correspondiente al digito recibido
 * como parametro, en binario
 * @param[in] digito corresponde a un digito del 1 al 9
 * @param[in] gpios puntero de tipo estructura gpioConf_t,
*/
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

/** @fn void muestra_numero (uint32_t pData, uint8_t pDigitos, gpioConf_t *gpios_digito, gpioConf_t *seleccion_bcd )
 * @brief funcion que recibe un numero de 3 digitos decimales mediante pData, y realiza la muestra de cada digito 
 * por la pantalla LCD
 * @param[in] pData referencia numero decimal de 3 digitos
 * @param[in] pDigitos parametro que representa la cantidad de digitos del numero deciamal, en este caso 3
 * @param[in] gpios_digito puntero del tipo estructura gpioConf_t referencia a cada gpio para representar en binario
 * @param[in] seleccion_bcd puntero del tipo estructura gpioConf_t referencia gpios que dara pulso para seleccionar cada display
*/
void muestra_numero ( uint32_t pData, uint8_t pDigitos, gpioConf_t *gpios_digito, gpioConf_t *seleccion_bcd ){

	pDigitos = 0;
	uint8_t bcd_number[3];
	while (pData != 0)
	{		
		bcd_number[pDigitos] = pData % 10;  
		pData = pData / 10;  
		++pDigitos;
	}

	gpios_digito[0].pin = GPIO_20;
	gpios_digito[1].pin = GPIO_21;
	gpios_digito[2].pin = GPIO_22;
	gpios_digito[3].pin = GPIO_23;

	seleccion_bcd[0].pin = GPIO_9;
	seleccion_bcd[1].pin = GPIO_18; 
	seleccion_bcd[2].pin = GPIO_19;

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

	uint32_t numero = 146;
	uint8_t digitos = 0;
	gpioConf_t gpios[4];
	gpioConf_t seleccion_bcd[3];

	muestra_numero (numero, digitos, gpios, seleccion_bcd);
}
/*==================[end of file]============================================*/