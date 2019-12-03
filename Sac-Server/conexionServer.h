#ifndef CONEXIONSERVER_H_
#define CONEXIONSERVER_H_

#include "operaciones.h"


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
void* socketThread(void *arg);

void tomarPeticionCreate(int);
void tomarPeticionOpen(int);
void tomarPeticionRead(int);
void tomarPeticionReadDir(int);
void tomarPeticionGetAttr(int);
void tomarPeticionMkdir(int);
void tomarPeticionUnlink(int);
void tomarPeticionRmdir(int);
void tomarPeticionWrite(int);
void tomarPeticionTruncate(int);
void tomarPeticionRename(int);


t_log* logger;
pthread_t hiloLevantarConexion;



#endif /* CONEXIONSERVER_H_ */
