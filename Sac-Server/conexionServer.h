#ifndef CONEXIONSERVER_H_
#define CONEXIONSERVER_H_
#define EXIT_FAILURE 1

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

typedef struct header_t { // un bloque (4096 bytes)
	unsigned char identificador[3]; // valor "SAC"
	uint32_t version; // valor 1
	ptrGBloque bloque_inicio_bitmap; // valor 1
	uint32_t tamanio_bitmap; // valor n (bloques)
	unsigned char relleno[4081]; // padding
} GHeader;

typedef struct file_t {
	estado_archivo estado; // 0: borrado, 1: archivo, 2: directorio
	unsigned char nombre_archivo[GFILENAMELENGTH]; // valor nombre del archivo
	ptrGBloque bloque_padre; // nro de bloque del directorio padre, 0 si esta en el raiz
	uint32_t tamanio_archivo; // maximo tamanio de archivo 4GB
	uint64_t fecha_creacion;
	uint64_t fecha_modificacion;
	ptrGBloque bloques_indirectos[BLKINDIRECT];
} GFile;

typedef struct {
	int ip;
	int puerto;
} config;

int iniciar_conexion(int, int);
void levantarConfigFile();
t_config* leer_config(void);
void loguearInfo(char *);
void loguearError();
t_bitarray * crearBitmap();
void borrarBitmap(t_bitarray*);

t_log* logger;
pthread_t hiloLevantarConexion;



#endif /* CONEXIONSERVER_H_ */
