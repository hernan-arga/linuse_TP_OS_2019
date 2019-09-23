#include <stdio.h>
#include "muse.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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
	crearSegmentoInicial(tamanio-5); //Le resto los 5 bytes que van a ser del header
}

void crearTablaSegmentos(){
	tablaSegmentos = list_create();
}
/////////REVISAR/////////////
void *crearSegmentoInicial(uint32_t tamanio){
	struct HeapMetadata metadata;
	metadata.isFree = true;
	metadata.size = tamanio;

	struct Segmento* segmentoInicial;
	(*segmentoInicial).metadata = metadata;
	//crearTablaPaginas(segmentoInicial); -En un futuro creo se haria asi
	//por ahora dejo un char* en el segmento para ocuparme de otras cosas

	(*segmentoInicial).data = malloc(tamanio);
	(*segmentoInicial).data = NULL;

	memoriaPrincipal = segmentoInicial;

	return &memoriaPrincipal + sizeof(metadata); //Chequear si no hay manera menos horrible de hacerlo
}

/*void crearTablaPaginas(struct Segmento segmento){
	segmento.paginas = list_create();
}*/

///////////////Funciones MUSE///////////////////////////

//MALLOC NAC & POP
//Funcion que llamaria libMuse por medio de muse_alloc(uint32_t tam)
//le va a retornar un void* tal como hace malloc y dentro de muse_alloc
// nos encargaremos de retornar la posicion de memoria como lo pide su firma

//void *musemalloc(uint32_t tamanio) {   //Adaptar esto a NUESTRO ESPACIO de mm (UPCM)
  /*void *ptr = sbrk(0);
  void *pedido = sbrk(tamanio);
  if (pedido == (void*) -1) {
    return NULL; // Fallo sbrk
  } else {
    assert(ptr == pedido); // Not thread safe.
    return ptr;
  }*/

//}




