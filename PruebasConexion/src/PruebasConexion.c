/*
 ============================================================================
 Name        : PruebasConexion.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <libmuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>


void recursiva(int num)
{

    /*if(num == 0)
        return;*/
	//char *frase = "nacho es gay";
	char *frase = "No sabes que sufro?";

    uint32_t ptr = muse_alloc(20);

    muse_cpy(ptr, frase, 20);
    //printf("MIRA, LA REALIDAD ES QUE... %i\n", *ptr);

    //recursiva(num - 1);
    //num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
    muse_get(frase, ptr, 20);
    printf("frase de mierda %s\n", frase);
    //muse_free(ptr);
}

int main(void)
{
    muse_init(getpid(), "127.0.0.1", 3306);
    recursiva(11);
    muse_close();
}
