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


void recursiva()
{
    /*if(num == 0)
        return;*/
	char *frase = "nacho es gay";

    uint32_t ptr = muse_alloc(15);
    muse_cpy(ptr, frase, 15);
    printf("MIRA, LA REALIDAD ES QUE... %s\n", frase);

    //recursiva(num - 1);
    //num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
    //muse_get(&num, ptr, 4);
    //printf("%d\n", num);
    //muse_free(ptr);
}

int main(void)
{
    muse_init(getpid(), "127.0.0.1", 3306);
    recursiva("nacho es gay");
    muse_close();
}
