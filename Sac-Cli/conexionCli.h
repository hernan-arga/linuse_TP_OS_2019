#ifndef CONEXIONCLI_H_
#define CONEXIONCLI_H_
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

int32_t sacServer;

typedef struct {
	char* ip;
	int puerto;
} config;

void conectarseASacServer(char* ,int);
t_log * crear_log();
void levantarConfigFile(config* pconfig);
t_config* leer_config();



t_log* logger;



#endif /* CONEXIONCLI_H_ */
