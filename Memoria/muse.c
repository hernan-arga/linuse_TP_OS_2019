#include <stdio.h>
#include "muse.h"
#include "utils.h"

int main(){

	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(5 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "MUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);

	/////////////////////////
	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	crearTablaSegmentos();


    return 0;
}


//////////////Funciones inicio recursos////////////////////

void reservarMemoriaPrincipal(int tamanio){
	memoriaPrincipal = malloc(tamanio);
}

void crearTablaSegmentos(){
	tablaSegmentos = list_create();
}





