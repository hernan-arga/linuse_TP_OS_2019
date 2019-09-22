#include "libmuse.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/*
int main(){

	printf( muse_init(1,"127.0.0.1",5003) );

	return 0;
}
*/
int muse_init(int id, char* ip, int puerto){ //Case 1

	clienteMUSE = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressMUSE.sin_family = AF_INET;
	serverAddressMUSE.sin_port = htons(puerto);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int resultado = connect(clienteMUSE, (struct sockaddr *) &serverAddressMUSE, sizeof(serverAddressMUSE));
	/*
	int *tamanioValue = malloc(sizeof(int));
	recv(clienteMUSE, tamanioValue, sizeof(int), 0);

	memcpy(&tamanioValue, tamanioValue, sizeof(int));
	*/

	return resultado;
}

void muse_close(){ /* Does nothing :) */ } //Case 2

uint32_t muse_alloc(uint32_t tam){ //Case 3


    return (uint32_t) malloc(tam);

    //Serializo peticion (2) y uint32_t tam tamaño a reservar

    //2 size de tamanio, 1 size de peticion, 1 size de tam (parametro)
    char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

    int peticion = 3;
    int tamanioPeticion = sizeof(int);
    memcpy(buffer, &tamanioPeticion, sizeof(int));
    memcpy(buffer + sizeof(int), &peticion, sizeof(int));

    int tamanioTam = sizeof(tam);
    memcpy(buffer + 2 * sizeof(int), &tamanioTam, sizeof(int));
    memcpy(buffer + 3 * sizeof(int), tam, sizeof(uint32_t));

    //Falta conexion y se hace envio a MUSE
    //send(clienteMUSE, buffer, strlen(tabla) + 5 * sizeof(int), 0);

    //musemaloc(tam)
}

void muse_free(uint32_t dir) { //Case 4
    free((void*) dir);
}

int muse_get(void* dst, uint32_t src, size_t n){ //Case 5
    memcpy(dst, (void*) src, n);
    return 0;
}

int muse_cpy(uint32_t dst, void* src, int n){//Case 6
    memcpy((void*) dst, src, n);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
uint32_t muse_map(char *path, size_t length, int flags){ //Case 7
    return 0;
}

int muse_sync(uint32_t addr, size_t len){ //Case 8
    return 0;
}

int muse_unmap(uint32_t dir){ //Case 9
    return 0;
}
