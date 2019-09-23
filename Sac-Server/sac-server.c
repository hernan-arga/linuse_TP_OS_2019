#include <stdio.h>
#include "utils.h"

int main(){
	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(2 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "SAC SERVER levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);
}
