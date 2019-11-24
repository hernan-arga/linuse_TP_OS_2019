#include "sac-server.h"


int main(){
	// Levanta archivo de configuracion
	config* pconfig = malloc(2 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);

	// Logueo
	loguearInfo(" + Levantado Sac-Server correctamente");

	iniciarMmap();
	bitArray = bitarray_create(mmapDeBitmap, tamanioEnBytesDelBitarray());

	pthread_join(hiloLevantarConexion, NULL);
	return 0;
}
