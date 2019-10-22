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

#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define GHEADERBLOCKS 1
#define BLKINDIRECT 1000
#define BLOCKSIZE 4096
#define NODE_TABLE_SIZE 1024
#define PTRGBLOQUE_SIZE 1024
#define GFILENAMELENGTH 71
#define BLKINDIRECT 1000

typedef uint32_t ptrGBloque;
typedef ptrGBloque pointer_data_block [PTRGBLOQUE_SIZE];

typedef enum __attribute__((packed)){
	BORRADO = '\0',
	OCUPADO = '\1',
	DIRECTORIO = '\2'
}estado_archivo;

typedef struct gheader{ // un bloque (4096 bytes)
	unsigned char identificador[3]; // valor "SAC"
	uint32_t version; // valor 1
	uint32_t bloque_inicio_bitmap; // valor 1
	uint32_t tamanio_bitmap; // valor n (bloques)
	unsigned char padding[4081]; // relleno
} gheader;

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
struct gheader *header_start;
struct gfile *node_table_start, *data_block_start, *bitmap_start;
struct gheader Header_Data;

int fuse_disc_size;
// Macros que definen los tamanios de los bloques.
#define NODE_TABLE_SIZE 1024
#define NODE_TABLE_SIZE_B ((int) NODE_TABLE_SIZE * BLOCKSIZE)
#define DISC_PATH fuse_disc_path
//#define DISC_SIZE_B(p) path_size_in_bytes(p)
#define ACTUAL_DISC_SIZE_B fuse_disc_size
#define BITMAP_SIZE_B (int) (get_size() / CHAR_BIT)
#define BITMAP_SIZE_BITS get_size()
#define HEADER_SIZE_B ((int) GHEADERBLOCKS * BLOCKSIZE)
#define BITMAP_BLOCK_SIZE Header_Data.tamanio_bitmap

#define ARRAY64SIZE _bitarray_64
size_t _bitarray_64;
#define ARRAY64LEAK _bitarray_64_leak
size_t _bitarray_64_leak;
t_log *logger;

pthread_rwlock_t rwlock;

t_bitarray * crearBitmap();
unsigned long long getMicrotime();
char* obtenerNombreArchivo(char*);
int asignarBloqueLibre();
void loguearBloqueQueCambio(int);
int tamanioEnBytesDelBitarray();
int split_path(const char*, char**, char**);
int add_node(struct gfile*, int);
int get_node(void);
ptrGBloque determinar_nodo(const char*);
int get_size(void);

#endif /* ESTRUCTURAS_H_ */
