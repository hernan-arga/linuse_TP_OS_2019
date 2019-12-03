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


void recursiva(int num);

int main(void) {
	muse_init(getpid(), "127.0.0.1", 3306);
		recursiva(10);
		muse_close();

	//printf("%" PRIu32 "\n", muse_map("archivo.txt",1,1)); //falla
	//printf("%d", muse_sync(1,1)); //ok
	//printf("%d", muse_unmap(1)); //ok
	return 0;
}

void recursiva(int num)
{
	/*if(num == 0)
		return;*/

	uint32_t ptr = muse_alloc(10);
	//muse_cpy(ptr, &num, 4);
	printf("num %d\n", num);
	printf("ptr %u\n", ptr);
	//printf("%" PRIu32 "\n",ptr);

	muse_free(ptr);

	//uint32_t ptr2 = muse_alloc(5);

	//printf("%" PRIu32 "\n",ptr2);

	/*
	recursiva(num - 1);
	num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
	muse_get(&num, ptr, 4);
	printf("%d\n", num);
	muse_free(ptr);
	*/
}
