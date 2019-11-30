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

int main(void) {
	printf("%d", muse_init(1,"127.0.0.1",8003)); // ok
	printf("\n");
	//printf("%" PRIu32 "\n",muse_alloc(10)); //ok
	//uint32_t a = 40;
	//printf("%p",&a);
	//muse_free(a); //ok
	//muse_close() //preguntar que onda, que hace
	int x = 10;
	void *puntero = &x;
	printf("%d", muse_get(puntero,1,1));
	printf("%d", muse_cpy(1,puntero,1));

	//printf("%" PRIu32 "\n", muse_map("archivo.txt",1,1)); //falla
	//printf("%d", muse_sync(1,1)); //ok
	//printf("%d", muse_unmap(1)); //ok
	return EXIT_SUCCESS;
}
