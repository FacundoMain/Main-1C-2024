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
/*==================[macros and definitions]=================================*/

//Escriba una función que reciba un dato de 32 bits,  la cantidad de dígitos de salida y 
//un puntero a un arreglo donde se almacene los n dígitos. La función deberá convertir el dato recibido a BCD, 
//guardando cada uno de los dígitos de salida en el arreglo pasado como puntero

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
void  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	digits = 0;
	while (data != 0)
	{		
		bcd_number[digits] = data % 10;  //456 % 10 = 6
		data = data / 10;  // data ahora es 45
		++digits;
	}
	printf("termino");
}

/*==================[external functions definition]==========================*/
void app_main(void){
	
	uint32_t numero = 456;
	uint8_t cantidad_digitos = 3;
	uint8_t vector_bcd[cantidad_digitos];
	convertToBcdArray (numero, cantidad_digitos, vector_bcd);
	printf ("primerdigito: %d\nsegundodigito: %d\n tercerdigito: %d", vector_bcd[0], vector_bcd[1], vector_bcd[2]);
}
/*==================[end of file]============================================*/