#ifndef CONEXIONSERVER_H_
#define CONEXIONSERVER_H_
#define EXIT_FAILURE 1

#include "operaciones.h"
#include "estructuras.h"
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

void tomarPeticionCreate(int);
void tomarPeticionOpen(int);
void tomarPeticionRead(int);
void tomarPeticionReadDir(int);
void tomarPeticionGetAttr(int);
void tomarPeticionMkdir(int);
void tomarPeticionUnlink(int);
void tomarPeticionRmdir(int);
void tomarPeticionWrite(int);

t_log* logger;
pthread_t hiloLevantarConexion;



#endif /* CONEXIONSERVER_H_ */
