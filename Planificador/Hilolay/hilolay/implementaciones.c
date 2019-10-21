#include <hilolay/alumnos.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <ctype.h>
#include <sys/stat.h>

int32_t servidorSUSE;
struct sockaddr_in serverAddressSUSE;

void suse_create(int tid){
	//Mandarle msj a SUSE
	printf("Mandar create con tid:%i\n", tid);
	char* buffer = malloc(2*sizeof(int));
	int operacion = 1;

	memcpy(buffer, &operacion, sizeof(int));
	memcpy(buffer + sizeof(int), &tid, sizeof(int));

	send(servidorSUSE, buffer, 2*sizeof(int), 0);
	free(buffer);
}

int suse_schedule_next(void){

	printf("Pedir proximo hilo a ejecutar\n");
	char* buffer = malloc(sizeof(int));
	int operacion = 2;

	memcpy(buffer, &operacion, sizeof(int));
	send(servidorSUSE, buffer, sizeof(int), 0);

	int *threadId = malloc(sizeof(int));
	recv(servidorSUSE, threadId, sizeof(int), 0);
	int tid = *threadId;

	free(threadId);
	free(buffer);
	printf("Proximo hilo: %i\n", tid);
	return tid;
}

int suse_join(int tid){
	printf("Llamaste a join\n");
	return 0;
}

int suse_close(int tid){
	printf("Closed thread %i\n", tid);
	//max_tid--;
	return 0;
}

void suse_wait(int tid, char *semID){
	printf("Pedir wait\n");
	char* buffer = malloc(3*sizeof(int) + strlen(semID) + 1);
	int operacion = 3;
	int longitudIDSemaforo = strlen(semID);

	memcpy(buffer, &operacion, sizeof(int));
	memcpy(buffer + sizeof(int), &tid, sizeof(int));
	memcpy(buffer + 2*sizeof(int), &longitudIDSemaforo, sizeof(int));
	memcpy(buffer + 3*sizeof(int), semID, longitudIDSemaforo+1);

	send(servidorSUSE, buffer, 3*sizeof(int) + strlen(semID) + 1, 0);
	free(buffer);
}

void suse_signal(int tid, char *semID){
	printf("Pedir signal\n");
}

void suse_init(){

	servidorSUSE = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressSUSE.sin_family = AF_INET;
	serverAddressSUSE.sin_port = htons(5005);
	serverAddressSUSE.sin_addr.s_addr = INADDR_ANY;
	//serverAddress.sin_addr.s_addr = inet_addr(IP);
		if (connect(servidorSUSE, (struct sockaddr *) &serverAddressSUSE,
				sizeof(serverAddressSUSE)) == -1) {
			perror("Hubo un error en la conexion");
			exit(-1);
		}
}

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
};

void hilolay_init(void){
	//Iniciar socket para comunicarse con suse?
	//verificar que suse este levantado?
	suse_init();
	suse_wait(2, "c3");
	init_internal(&hiloops);

}
