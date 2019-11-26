#include <stdio.h>
#include "muse.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

int main() {

	// Levanta archivo de configuracion - Se hace una unica vez
	t_log *log = crear_log();
	config* pconfig = malloc(5 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL,
			(void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "MUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);

	/////////////////////////
	tam_mem = pconfig->tamanio_memoria; //Ver de poner como define
	tam_pagina = pconfig->tamanio_pag; //Ver de poner como define
	tam_swap = pconfig->tamanio_swap;
	cantidadFrames = tam_mem / tam_pagina;
	cantidadPaginasSwap = tam_swap / tam_pagina;

	//Se hace una unica vez
	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	//Se hace una unica vez
	crearBitmapFrames();
	//Se hace una unica vez
	tablasSegmentos = dictionary_create();
	//Abro archivo swap
	swap = fopen("swap.txt","a+"); //Validar modo apertura y limite tamaño tam_swap

	bitmapSwap = bitarray_create((char*)bitmapSwap, cantidadPaginasSwap);  //size - cantidad de bits del bitarray, expresado en bytes
	//verificar 1er parametro, swap?
	//inicializarBitmapSwap();

	return 0;
}

//////////////Funciones inicio recursos////////////////////

void reservarMemoriaPrincipal(int tamanio) {
	memoriaPrincipal = malloc(tamanio);
	//crearHeaderInicial(tamanio - sizeof(struct HeapMetadata)); Se haria por cada primer malloc
}

void *crearHeaderInicial(uint32_t tamanio) {
	struct HeapMetadata *metadata = malloc(sizeof(tam_mem));
	metadata->isFree = true;
	metadata->size = tamanio;

	memoriaPrincipal = metadata;

	return NULL;
}

/* Crea la tabla de segmentos -vacia- para un proceso. Solo la crea si
 * no la encuentra previamente en el diccionario (tema hilos) */
void crearTablaSegmentosProceso(int idSocketCliente) {

	if (dictionary_has_key(tablasSegmentos, (char*)idSocketCliente) == false) {
		t_list *listaDeSegmentos = list_create();

		//Creo la estructura, pero no tiene segmentos aun
		dictionary_put(tablasSegmentos, (char*)idSocketCliente, listaDeSegmentos);
	}

}

void crearBitmapFrames() {
	//Creo una t_list y le agrego sus cantidadFrames elementos que van a quedar estaticos
	bitmapFrames = list_create();

	for(int i = 0; i < cantidadFrames; i++){

		struct Frame *nuevoFrame = malloc(sizeof(struct Frame));

		nuevoFrame->modificado = 0;
		nuevoFrame->presencia = 1;
		nuevoFrame->uso = 0;
		//nuevoFrame indiceswap

		list_add(bitmapFrames, nuevoFrame);

	}

}

//MUSE ALLOC
void *musemalloc(uint32_t tamanio, int idSocketCliente){
	//Obtener el id del cliente y que no llegue como parametro

	//Obtengo el espacio de direccionamiento logico del proceso
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, (char*)idSocketCliente);

	//¿Hay algun segmento del proceso donde entre la data?
	if(existeSegmentoConTamanioDisponible(segmentosProceso, (tamanio + sizeof(struct HeapMetadata))) == true){

		struct Segmento *segmentoConTamanioDisponible = buscarSegmentoConTamanioDisponible(segmentosProceso, (tamanio + sizeof(struct HeapMetadata)));
		segmentoConTamanioDisponible = asignarTamanioASegmento(tamanio, segmentoConTamanioDisponible, idSocketCliente);

		list_replace(segmentosProceso, segmentoConTamanioDisponible->id, segmentoConTamanioDisponible);
		//Tener en cuenta que dictionary put no libera la memoria de la data original
		dictionary_put(tablasSegmentos, (char*)idSocketCliente, segmentosProceso);

	}

	//¿Hay algun segmento que se pueda extender?
	else{

		if(existeSegmentoExtendible(segmentosProceso) == true){

			//Extiendo el segmento las paginas necesarias, busco frames y modifico la tabla de paginas
			//del segmento, la lista de segmentos del proceso y el bitmap de frames

		}

		else{

			//Creo un nuevo segmento que me permita alocar tamaño solicitado + tamaño de 2 metadatas

		}

	}
}

bool existeSegmentoConTamanioDisponible(t_list *listaSegmentos, uint32_t tamanioRequerido){

	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	for(int i = 0; i < list_size(listaSegmentos); i++){
		segmento = list_get(listaSegmentos, i);

		//CREO que solo deberia revisar los segmentos "comunes"
		if(segmento->esComun == true && poseeTamanioDisponible(segmento, tamanioRequerido) == true){
			return true;
		}

		//Free segmento?
	}

	return false;

}

//INCOMPLETA
bool poseeTamanioDisponible(struct Segmento *segmento, uint32_t tamanioRequerido){ //TamanioRequerido tiene incluido el heapmetadata

	t_list *paginasSegmento = segmento->tablaPaginas;
	int paginasRecorridas = 0; //Paginas recorridas a la vez me indica la pagina actual
	struct Pagina *pagina = malloc(sizeof(struct Pagina));
	struct Frame *frame = malloc(sizeof(struct Frame));
	void *pos;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	int tamanioAMoverse;

	while(paginasRecorridas < list_size(paginasSegmento)){
		pagina = list_get(paginasSegmento, paginasRecorridas);
		frame = list_get(bitmapFrames, pagina->numeroFrame);

		//Tengo que chequear que el frame este en mm ppal?
		if(paginasRecorridas == 0){
			pos = retornarPosicionMemoriaFrame(pagina->numeroFrame);
			metadata = pos;

			if(metadata->isFree == true && metadata->size >= tamanioRequerido){
				return true;
			}

			//Si no le alcanza la metadata, tiene que moverse a la proxima
			tamanioAMoverse = metadata->size;

		} else {

			//Moverse de metadata en metadata
			//buscando metadata free con size may o igual al requerido

			return false; //para que no rompa hasta completarla
		}

		paginasRecorridas++;
	}

	return false; //para que no rompa hasta completarla
	//Free pagina, frame, metadata?
}

bool existeSegmentoExtendible(t_list *listaSegmentos){
	//un segmento es extendible siempre y cuando a continuación del mismo no exista otro segmento.

	for(int i = 0; i < list_size(listaSegmentos); i++){

		if(esExtendible(listaSegmentos, i) == true){
			return true;
		}

	}

	return false;
}

//A DESARROLLAR
struct Segmento *asignarTamanioASegmento(uint32_t tamanio, struct Segmento *segmentoConTamanioDisponible, int idSocketCliente){
	return NULL;
}

//A DESARROLLAR
struct Segmento *buscarSegmentoConTamanioDisponible(t_list *segmentosProceso, uint32_t tamanio){
	return NULL;
}

/*Un segmento es extendible si no tiene otro segmento a continuacion*/
bool esExtendible(t_list *segmentosProceso, int unIndice) {
	//Momentaneamente, es extendible si es el ultimo de la lista de segmentos
	int indiceUltimoSegmento = list_size(segmentosProceso) - 1;

	struct Segmento *ultimoSegmento = malloc(sizeof(struct Segmento));
	ultimoSegmento = list_get(segmentosProceso, indiceUltimoSegmento);

	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	segmento = list_get(segmentosProceso, unIndice);

	if (ultimoSegmento->id == segmento->id) {
		return true;
	} else {
		return false;
	}

}

/*Retorna la posicion de memoria donde comienza un frame en particular*/
void *retornarPosicionMemoriaFrame(int unFrame) {
	int offset = tam_pagina * unFrame; //Los frames estan en orden y se recorren de may a men

	return ((&memoriaPrincipal) + offset);
}
