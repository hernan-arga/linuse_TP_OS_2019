#include "libmuse.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int muse_init(int id, char* ip, int puerto) { //Case 1

	clienteMUSE = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressMUSE.sin_family = AF_INET;
	serverAddressMUSE.sin_port = htons(puerto);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int resultado = connect(clienteMUSE, (struct sockaddr *) &serverAddressMUSE,
			sizeof(serverAddressMUSE));
	/*
	 int *tamanioValue = malloc(sizeof(int));
	 recv(clienteMUSE, tamanioValue, sizeof(int), 0);

	 memcpy(&tamanioValue, tamanioValue, sizeof(int));
	 */

	return resultado;
}

void muse_close() { /* Does nothing :) */
} //Case 2

uint32_t muse_alloc(uint32_t tam) { //Case 3

	return (uint32_t) malloc(tam);

	//Serializo peticion (3) y uint32_t tam tamaño a reservar

	//2 size de tamanio, 1 size de peticion, 1 size de tam (parametro)
	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 3;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioTam = sizeof(tam);
	memcpy(buffer + 2 * sizeof(int), &tamanioTam, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &tam, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	send(clienteMUSE, buffer, 3 * sizeof(int) + sizeof(uint32_t), 0);

}

void muse_free(uint32_t dir) { //Case 4
	free((void*) dir);

	//Serializo peticion (4) y uint32_t dir

	//2 size de tamanio, 1 size de peticion, 1 size de dir (parametro)
	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 4;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDir = sizeof(dir);
	memcpy(buffer + 2 * sizeof(int), &tamanioDir, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dir, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	send(clienteMUSE, buffer, 3 * sizeof(int) + sizeof(uint32_t), 0);

}

int muse_get(void* dst, uint32_t src, size_t n) { //Case 5

	//Serializo peticion (5), void* dst ,uint32_t src y size_t n

	//4 size de tamanio, 1 size de peticion, 1 size de dst, 1 size de src y 1 size de n
	char *buffer = malloc(5 * sizeof(int) + sizeof(dst) + sizeof(uint32_t) + sizeof(size_t));

	int peticion = 5;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDst = sizeof(dst);
	memcpy(buffer + 2 * sizeof(int), &tamanioDst, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dst, sizeof(dst));

	int tamanioSrc = sizeof(uint32_t);
	memcpy(buffer + 3 * sizeof(int), &tamanioSrc, sizeof(int));
	memcpy(buffer + 4 * sizeof(int), &src, sizeof(uint32_t));

	int tamanioN = sizeof(size_t);
	memcpy(buffer + 4 * sizeof(int), &tamanioN, sizeof(int));
	memcpy(buffer + 5 * sizeof(int), &n, sizeof(size_t));

	//Falta conexion y se hace envio a MUSE
	send(clienteMUSE, buffer, 5 * sizeof(int) + sizeof(dst) + sizeof(uint32_t) + sizeof(size_t), 0);

	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n){//Case 6
	//Serializo peticion (6) y parametros (uint32_t dst, void* src, int n)

	char *buffer = malloc(6 * sizeof(int) + sizeof(uint32_t) + sizeof(src));

	int peticion = 6;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDst = sizeof(dst);
	memcpy(buffer + 2 * sizeof(int), &tamanioDst, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dst, sizeof(uint32_t));

	int tamanioSrc = sizeof(src);
	memcpy(buffer + 3 * sizeof(int) + sizeof(uint32_t), &tamanioSrc, sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + sizeof(uint32_t), &src, sizeof(src));

	int tamanioN = sizeof(n);
	memcpy(buffer + 4 * sizeof(int) + sizeof(uint32_t), &tamanioN, sizeof(int));
	memcpy(buffer + 5 * sizeof(int) + sizeof(uint32_t), &n, sizeof(int));


	//Falta conexion y se hace envio a MUSE
	//send(clienteMUSE, buffer, , 0);

    return 0;

}

uint32_t muse_map(char *path, size_t length, int flags) { //Case 7
	return 0;
}

int muse_sync(uint32_t addr, size_t len) { //Case 8
	return 0;
}

int muse_unmap(uint32_t dir) { //Case 9
	//Serializo peticion (9) y uint32_t dir

	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 9;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDir = sizeof(dir);
	memcpy(buffer + 2 * sizeof(int), &tamanioDir, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dir, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	//send(clienteMUSE, buffer, , 0);

	return 0;
}
