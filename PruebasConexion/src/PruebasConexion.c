#include <stdio.h>
#include <stdlib.h>
#include "libmuse.h"
#include <unistd.h>

void recursiva(int num)
{
	if(num == 0)
		return;

    /*if(num == 0)
        return;*/
	//char *frase = "nacho es gay";

    uint32_t ptr = muse_alloc(4);

    muse_cpy(ptr, &num, 4);
    //printf("MIRA, LA REALIDAD ES QUE... %i\n", *ptr);

    //recursiva(num - 1);
    //num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
    muse_get(&num, ptr, 4);
    printf("numero de mierda %d\n", num);

    int num2 = 18;
    uint32_t ptr2 = muse_alloc(4);

    muse_cpy(ptr2, &num2, 4);
    //printf("MIRA, LA REALIDAD ES QUE... %i\n", *ptr);

    //recursiva(num - 1);
    //num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
    muse_get(&num2, ptr2, 4);
    printf("numero de mierda2222222222 %d\n", num2);

    //muse_free(ptr);

}

int main(void)
{
    muse_init(getpid(), "127.0.0.1", 3306);
    recursiva(11);
    muse_close();
}
