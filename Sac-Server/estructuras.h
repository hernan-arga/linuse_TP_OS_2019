#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_
#define _GNU_SOURCE

#include <dirent.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>


t_bitarray * crearBitmap();
unsigned long long getMicrotime();
char* obtenerNombreArchivo(char*);
int asignarBloqueLibre();
void loguearBloqueQueCambio(int);
int tamanioEnBytesDelBitarray();


#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define GHEADERBLOCKS 1
#define BLKINDIRECT 1000
#define BLOCK_SIZE 4096

typedef uint32_t ptrGBloque;

typedef enum __attribute__((packed)){
	BORRADO = '\0',
	OCUPADO = '\1',
	DIRECTORIO = '\2'
}estado_archivo;

typedef struct { // un bloque (4096 bytes)
	unsigned char identificador[3]; // valor "SAC"
	uint32_t version; // valor 1
	ptrGBloque bloque_inicio_bitmap; // valor 1
	uint32_t tamanio_bitmap; // valor n (bloques)
	unsigned char relleno[4081]; // padding
} gheader;

typedef struct {
	char estado; // 0: borrado, 1: archivo, 2: directorio
	char* nombre_archivo; // maximo 71 bytes
	ptrGBloque bloque_padre; // nro de bloque del directorio padre, 0 si esta en el raiz
	int tamanio_archivo; // maximo tamanio de archivo 4GB
	unsigned long long fecha_creacion;
	unsigned long long fecha_modificacion;
	ptrGBloque* bloques_indirectos;
} gfile;


#endif /* ESTRUCTURAS_H_ */
