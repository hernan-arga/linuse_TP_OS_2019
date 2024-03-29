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
#include "unistd.h"
#include <stdio.h>

#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define HEADER 1
#define BLOQUESINDIRECTOS 1000
#define TAMANIO_BLOQUE 4096
#define TAMANIO_TABLA_DE_NODOS 1024
#define PTRGBLOQUE_SIZE 1024
#define GFILENAMELENGTH 71
#define BLOQUESINDIRECTOS 1000

typedef uint32_t ptrGBloque;
typedef ptrGBloque pointer_data_block [PTRGBLOQUE_SIZE];

// Se guardara aqui la cantidad de bloques libres en el bitmap
int bitmap_free_blocks;

// Se utiliza esta variable para saber si se encuentra en modo "borrar". Esto afecta, principalmente, al delete_nodes_upto
#define DELETE_MODE _del_mode
#define ENABLE_DELETE_MODE _del_mode=1
#define DISABLE_DELETE_MODE _del_mode=0
int _del_mode;

typedef enum __attribute__((packed)){
	BORRADO = '\0',
	OCUPADO = '\1',
	DIRECTORIO = '\2'
}estado_archivo;

typedef struct sac_header_t{ // un bloque (4096 bytes)
	unsigned char identificador[3]; // valor "SAC"
	uint32_t version; // valor 1
	uint32_t bloque_inicio_bitmap; // valor 1
	uint32_t tamanio_bitmap; // valor n (bloques)
	unsigned char padding[4081]; // relleno
} gheader;

typedef struct sac_file_t{
	uint8_t estado; // 0: borrado, 1: archivo, 2: directorio
	unsigned char nombre_archivo[GFILENAMELENGTH];
	uint32_t bloque_padre;
	uint32_t tamanio_archivo;
	uint64_t fecha_creacion;
	uint64_t fecha_modificacion;
	ptrGBloque bloques_indirectos[BLOQUESINDIRECTOS];
} gfile;

struct sac_header_t *header_start;
struct sac_file_t *inicioTablaDeNodos, *inicioBloquesDeDatos, *bitmap_start;
struct sac_header_t Header_Data;

int fuse_disc_size;
// Macros que definen los tamanios de los bloques.
#define TAMANIO_TABLA_DE_NODOS 1024
#define NODE_TABLE_SIZE_B ((int) TAMANIO_TABLA_DE_NODOS * TAMANIO_BLOQUE)

#define BITMAP_SIZE_B (int) (get_size() / CHAR_BIT)
#define BITMAP_SIZE_BITS get_size()
#define HEADER_SIZE_B ((int) HEADER * TAMANIO_BLOQUE)
#define TAMANIO_BITMAP Header_Data.tamanio_bitmap

#define DISC_PATH "/sac-tools/disco.bin"
#define ARRAY64SIZE _bitarray_64
size_t _bitarray_64;
#define ARRAY64LEAK _bitarray_64_leak
size_t _bitarray_64_leak;
t_log *logger;

// Se guardara aqui la ruta al disco. Tiene un tamanio maximo.
char fuse_disc_path[1000];

// Se guardara aqui el tamanio del disco
int fuse_disc_size;

//#define DISC_SIZE_B(p) path_size_in_bytes(p)
#define ACTUAL_DISC_SIZE_B fuse_disc_size

#define INT64MAX _max

pthread_rwlock_t superLockeador;
#define THELARGESTFILE (uint32_t) (BLOQUESINDIRECTOS*PTRGBLOQUE_SIZE*TAMANIO_BLOQUE)

t_bitarray * crearBitmap();
unsigned long long getMicrotime();
char* obtenerNombreArchivo(char*);
int asignarBloqueLibre();
void loguearBloqueQueCambio(int);
int tamanioEnBytesDelBitarray();
int dividirRuta(const char*, char**, char**);
int agregarBloqueLibre(struct sac_file_t*, int);
int obtenerBloqueLibre(void);
ptrGBloque dameNodoDe(const char*);
int get_size(void);
int lastchar(const char*, char);
int settearPosicion (int *, int *, size_t, off_t);
int eliminarNodos (struct sac_file_t *, int, int);
int darleEspacioNuevo (struct sac_file_t *file_data, int size);
int obtenerBloquesLibres(void);

t_bitarray* bitArray;
char *mmapDeBitmap;

int discDescriptor;
int fd;
#define LOG_PATH fuse_log_path
char fuse_log_path[1000];

#endif /* ESTRUCTURAS_H_ */
