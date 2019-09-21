#include "libmuse.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <stdio.h>


int main(){

	printf( " %d ",muse_init(1,"127.0.0.1",5004));

	return 0;
}

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
