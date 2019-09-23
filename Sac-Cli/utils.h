#ifndef UTILS_H_
#define UTILS_H_
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


typedef struct {
	int ip;
	int puerto;
} config;

void iniciar_conexion();
void levantarConfigFile();
t_config* leer_config(void);
t_log * crear_log();


t_log* logger;



#endif /* UTILS_H_ */
