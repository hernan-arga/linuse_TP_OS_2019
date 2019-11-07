#include <stdio.h>
#include <unistd.h>
#include <hilolay/hilolay.h>

void recursiva(int cant) {
    if(cant > 0) recursiva(cant - 1);
}

void *test3(void *arg) {
    int i, tid;

    for (i = 10; i >= 0; i--) {
        tid = hilolay_get_tid();
        //printf("Aqui ult %d, contador en: %d \n", tid, i);
        printf("hola xd: countdown %d\n", i);
        usleep(30000 * i); /* Randomizes the sleep, so it gets larger after a few iterations */

        recursiva(i);

        // Round Robin will yield the CPU
        hilolay_yield();
    }

    return 0;
}

void *test2(void *arg) {
    int i, tid;

    for (i = 10; i >= 0; i--) {
        tid = hilolay_get_tid();
        //printf("Aqui ult %d, contador en: %d \n", tid, i);
        printf("Major Tom: countdown %d\n", i);
        usleep(30000 * i); /* Randomizes the sleep, so it gets larger after a few iterations */

        recursiva(i);

        // Round Robin will yield the CPU
        hilolay_yield();
    }

    printf("Major Tom: Lift off\n");

    return 0;
}

void *test1(void *arg) {
    int i, tid;

    for (i = 1; i <= 50; i++) {
        tid = hilolay_get_tid();
        printf("Can you hear me, Major Tom? X%d \n", i);
        usleep(10 * i); /* Randomizes the sleep, so it gets larger after a few iterations */
        recursiva(i);
        hilolay_yield();
    }

    return 0;
}


/* Main program */
int main() {
    int i;

    hilolay_init();
    struct hilolay_t th1;
    struct hilolay_t th2;
    struct hilolay_t th3;

	hilolay_create(&th1, NULL, &test1, NULL);
	hilolay_create(&th2, NULL, &test2, NULL);
	//hilolay_create(&th3, NULL, &test3, NULL);

	//hilolay_join(&th3);
	hilolay_join(&th2);
	hilolay_join(&th1);

	return 0;
}
