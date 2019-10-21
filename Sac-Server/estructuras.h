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
#include <commons/collections/list.h>
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
int split_path(const char*, char**, char**);


#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define GHEADERBLOCKS 1
#define BLKINDIRECT 1000
#define BLOCK_SIZE 4096
#define NODE_TABLE_SIZE 1024
#define PTRGBLOQUE_SIZE 1024
#define GFILENAMELENGTH 71
#define BLKINDIRECT 1000

typedef uint32_t ptrGBloque;
t_list* tablaNodos;

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

typedef ptrGBloque pointer_data_block [PTRGBLOQUE_SIZE];

typedef struct gfile{
	uint8_t estado; // 0: borrado, 1: archivo, 2: directorio
	unsigned char nombre_archivo[GFILENAMELENGTH];
	uint32_t bloque_padre;
	uint32_t tamanio_archivo;
	uint64_t fecha_creacion;
	uint64_t fecha_modificacion;
	ptrGBloque bloques_indirectos[BLKINDIRECT];
} gfile;

struct gfile *node_table_start;

#endif /* ESTRUCTURAS_H_ */
