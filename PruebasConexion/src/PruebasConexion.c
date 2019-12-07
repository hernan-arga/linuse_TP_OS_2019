#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libmuse.h"

char* pasa_palabra(int cod)
{
	switch(cod)
	{
	case 1:
		return strdup("No sabes que sufro?\n");
	case 2:
		return strdup("No escuchas mi plato girar?\n");
	case 3:
		return strdup("Cuanto tiempo hasta hallarte?\n");
	case 4:
	case 5:
		return strdup("Uh, haces mi motor andar\n");
	case 6:
		return strdup("Y mis cilindros rotar\n");
	default:
	{
		if(cod % 2)
			return strdup("Oh si\n");
		else
			return strdup("un Archivo de swap supermasivo\n");
	}
	}
}

void recursiva(int num)
{
	if(num == 0)
		return;
	char* estrofa = pasa_palabra(num);
	int longitud = strlen(estrofa)+1;
	uint32_t ptr = muse_alloc(longitud);

	muse_cpy(ptr, estrofa, longitud);
	recursiva(num - 1);
<<<<<<< HEAD
	muse_get(estrofa, ptr, longitud);

	puts(estrofa);
=======
	num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
	muse_get(&num, ptr, 4);
	printf("%d\n", num);
>>>>>>> 1b83fd5a0cefa5501189e4efa0f83ca216bba35a

	muse_free(ptr);
	free(estrofa);
	sleep(1);
}

int main(void)
{
	muse_init(getpid(), "127.0.0.1", 3306);
<<<<<<< HEAD
	recursiva(15);
=======
	recursiva(20);
>>>>>>> 1b83fd5a0cefa5501189e4efa0f83ca216bba35a
	muse_close();
}
