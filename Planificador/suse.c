/*
 * 1 - correr algoritmo fifo
 * 2 - correr algoritmo sjf
 * 3 - Respetar grado de mutiprog
 * 4 - mostrar resultados por pantalla
 * 5 - implementar cola extra de multiprog
 * */

#include <stdio.h>
#include "suse.h"

//lista de programas a planificar
t_list* suse_programlist = NULL;
t_queue* suse_new = NULL;
t_list* suse_exit = NULL;
t_list* suse_block = NULL;

int main(){
	suse_new = queue_create();
	suse_exit = list_create();
	suse_block = queue_create();
	suse_programlist = list_create();

	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(5 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->puerto), NULL);
	log_info(log, "SUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);


    return 0;

}
