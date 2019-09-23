/*
 * sac-server.c
 *
 *  Created on: 22 sep. 2019
 *      Author: utnso
 */

#include "sac-server.h"
#include <stdio.h>

int main(){

	// t_log * log = crear_log();
	t_bitarray *bitArray = crearBitmap();
	borrarBitmap(bitArray);
	return 0;
}
