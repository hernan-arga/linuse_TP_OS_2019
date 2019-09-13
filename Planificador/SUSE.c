#include <stdio.h>
#include "suse.h"
#include "utils.h"

int main(){

	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(5 * sizeof(int)); //sumarle al malloc los arrays
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "SUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);
    return 0;
}


