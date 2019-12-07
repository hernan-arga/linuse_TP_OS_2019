#include <stdio.h>
#include <stdlib.h>
#include "libmuse.h"
#include <unistd.h>

char* pasa_palabra(int cod) {
    switch(cod)
    {
    case 1:
        return strdup("hola\n");

    default:
    {
        if(cod % 2)
            return strdup("hola\n");
        else
            return strdup("hola\n");
    }
    }
}

void recursiva(int num) {
if(num == 0)
		return;

	char* estrofa = pasa_palabra(1);
	int longitud = strlen(estrofa);
	uint32_t ptr = muse_alloc(longitud);

	muse_cpy(ptr, estrofa, longitud);
	recursiva(num - 1);
	//muse_get(estrofa, ptr, longitud);

	puts(estrofa);

	//muse_free(ptr);
	free(estrofa);
}

int main(void)
{
	muse_init(getpid(), "127.0.0.1", 3306);
	recursiva(20);
	muse_close();
}
