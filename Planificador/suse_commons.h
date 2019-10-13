#ifndef SUSE_COMMONS_
#define SUSE_COMMONS_

#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <stdint.h>

typedef struct{
	int puerto;
	int metrics_timer;
	int max_multiprog;
	int alpha;
} config;

typedef enum
{
	INIT,
	CREATE,
	NEXT,
	WAIT,
	SIGNAL,
	JOIN
}op_code;

//********************************************************************//
//agregar todos los parametros de metricas para los threads y programas
typedef struct {
	int pid;
	t_queue * ready;
	t_queue * exec;
} t_programa;

typedef struct {
	int tid;
	int pid;
} t_hilo;
//********************************************************************//

void levantarConfigFile(config*);
t_config* leer_config(void);
t_log * crear_log();

pthread_t hiloLevantarConexion;

void suse_init(void*); //crea un programa en base a un pid recibido del cliente y lo agrega a la lista de programas
void suse_create(void*);

void show_program_list();
void limpiarColasYListas();

void addProgramList(t_programa*);
void suse_new_event(t_hilo*);

t_programa* buscar_programa(int);

#endif /* SUSE_COMMONS_ */

















