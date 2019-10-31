#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hilolay/hilolay.h>

int offside=0;
hilolay_sem_t *consumidor;
hilolay_sem_t *productor;

void recursiva(int cant) {
    if(cant > 0) recursiva(cant - 1);
}

void *test1(void *arg) {
    int i, tid;

    for (i = 0; i < 50; i++) {
    	hilolay_wait(consumidor);

        //tid = hilolay_get_tid();
        offside++;
        printf("Borre entro en offside. cantidad acumulada %d \n", offside);
        //printf("Soy el ult %d mostrando el numero %d \n", tid, i);
        usleep(9000 * i * tid); /* Randomizes the sleep, so it gets larger after a few iterations */


        recursiva(i);

        hilolay_signal(productor);

        // Round Robin will yield the CPU
        hilolay_yield();
    }

    return 0;
}

void *test2(void *arg) {
    int i, tid;

    for (i = 0; i < 50; i++) {
    	hilolay_wait(productor);

        tid = hilolay_get_tid();
        offside--;
        printf("Lunati anula el offside. cantidad acumulada %d \n", offside);
        usleep(1000 * i * tid); /* Randomizes the sleep, so it gets larger after a few iterations */

        recursiva(i);
        hilolay_signal(consumidor);

        hilolay_yield();
    }

    return 0;
}


/* Main program */
int main() {
    int i;
    consumidor = malloc(sizeof(hilolay_sem_t));
    consumidor->name = "CONSUMIDOR";
    productor = malloc(sizeof(hilolay_sem_t));
    productor->name = "PRODUCTOR";

    hilolay_init();
    struct hilolay_t th1;
    struct hilolay_t th2;

	hilolay_create(&th1, NULL, &test1, NULL);
	hilolay_create(&th2, NULL, &test2, NULL);

	hilolay_join(&th2);
	hilolay_join(&th1);

	free(consumidor);
	free(productor);
	return 0;
}
