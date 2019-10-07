#include <stdio.h>
#include "muse.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

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
	tam_mem = pconfig->tamanio_memoria; //Ver de poner como define
	tam_pagina = pconfig->tamanio_pag; //Ver de poner como define

	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	//crearTablaSegmentos();
	tablasSegmentos = dictionary_create();


    return 0;
}


//////////////Funciones inicio recursos////////////////////

void reservarMemoriaPrincipal(int tamanio){
	memoriaPrincipal = malloc(tamanio);
	crearHeaderInicial(tamanio-sizeof(struct HeapMetadata));
}

void *crearHeaderInicial(uint32_t tamanio){
	struct HeapMetadata *metadata = malloc(sizeof(tam_mem));
	metadata->isFree = true;
	metadata->size = tamanio;

	memoriaPrincipal = metadata;

	return NULL;
}

///////////////Funciones MUSE///////////////

//MUSE INIT
//La llamamos en muse init, le crea al proceso (id) la tabla de segmentos correspondiente

void museinit(int id, char* ip/*, int puerto*/){ //Creo que no necesito el puerto - lo comento para consultar
	//Funcionara como id del proceso y sera la key en el diccioario
	char* idProceso = string_new();// = string_itoa(id) ++ ip;
	string_append(&idProceso,string_itoa(id));
	string_append(&idProceso,ip);

	dictionary_put(tablasSegmentos, idProceso, NULL);
}


//MUSE ALLOC

void *musemalloc(uint32_t tamanio){
	return NULL;
}

//Funcion recorre buscando heapmetadata libre - mayor o igual - a cierto size

void *buscarEspacioLibre(uint32_t tamanio){
	void *pos;
	pos = &memoriaPrincipal;
	void *end;
	end = &memoriaPrincipal + tam_mem;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	uint32_t tamanioData;

	while(pos != end){
		memcpy(&memoriaPrincipal,metadata,sizeof(struct HeapMetadata));
		tamanioData = (*metadata).size;

		if((*metadata).isFree == true && (*metadata).size <= tamanio){
			//el espacio me sirve y lo asigno
			//cambio valor isFree y retorno posicion
			(*metadata).isFree = false;
			(*metadata).size = tamanio;

			if((tamanioData - tamanio) >= sizeof(struct HeapMetadata)){
				struct HeapMetadata *nuevoHeader = malloc(sizeof(struct HeapMetadata));
				nuevoHeader->isFree = true;
				nuevoHeader->size= tamanioData - tamanio - sizeof(struct HeapMetadata);

				pos = pos + sizeof(struct HeapMetadata) + tamanio;
				*(&pos) = nuevoHeader; //ver
			}
		}
		else{
			pos = &memoriaPrincipal + sizeof(struct HeapMetadata) + tamanioData; //Me muevo al proximo header
		}
	}

	return NULL;
}

/**Funcion para unificar dos headers - se la llama despues de un free. Unifica desde el header 1
 * recorriendo todos los headers DEL SEGMENTO. Le envio como parametro el id del segmento que
 * tiene que recorrer
*/

void unificarHeaders(int idSegmento){
	int tamanioSegmento = calcularTamanioSegmento(idSegmento); //va a devolver el tamanio del segmento
	//que es el tamanio que voy a tener que recorrer sin salirme. Dentro del while

	void *pos;
	pos = buscarPosicionSegmento(idSegmento);
	void *end;
	end = pos + tamanioSegmento;
	struct HeapMetadata *header = malloc(sizeof(struct HeapMetadata));
	struct HeapMetadata *siguienteHeader = malloc(sizeof(struct HeapMetadata));

	memcpy(pos,header,sizeof(struct HeapMetadata));

	while(pos != end){

		if(header->isFree == true){
			pos = pos + sizeof(struct HeapMetadata) + header->size; //Me muevo al siguiente header
			memcpy(pos,siguienteHeader,sizeof(struct HeapMetadata)); //Leo el siguiente header

			if(siguienteHeader->isFree == true){
				int size2 = siguienteHeader->size;
				header->size = header->size + size2; //Unifico ambos headers en el primero
				free(siguienteHeader); //Libero el segundo header

				pos = pos + sizeof(struct HeapMetadata) + size2; //Me muevo al tercer header - proximo a leer
			}

		} else{
			pos = pos + sizeof(struct HeapMetadata) + header->size; //Me muevo al siguiente header
			memcpy(pos,header,sizeof(struct HeapMetadata)); //Leo el siguiente header
		}
	}
}

/**Recorre los segmentos anteriores calculando su size con calcularSizeSegmento(unSegmento)
 * size de su t_list de paginas por su cantidad de paginas y la posicion del segmento
 * es el inicio de la memoria + los tamaños de sus segmentos anteriores
*/
void *buscarPosicionSegmento(int idSegmento){
	void *pos;
	pos = &memoriaPrincipal;
	//int cantSegmentos = list_size(tablaSegmentos);
	int cantSegmentos;

	for(int i = 0; i < cantSegmentos; i++){ //COMENTADA PORQUE TENGO QUE LIGAR LA TLIST CON SEGMENTO
//		if (tablaSegmentos[i].pid = id){
//			return pos;
//		} else{
//			pos = pos + sizeof(int) /*segmento.id*/ + espacioPaginas(idSegmento);
//		}
	}

	return NULL;
}

int calcularTamanioSegmento(int idSegmento){
	int cantSegmentos;

	for(int i = 0; i < cantSegmentos; i++){
//		if(tablaSegmentos[i].pid = idSegmento){
//			return sizeof(int) + espacioPaginas(idSegmento); //Falta chequeo que el espacio de paginas no sea -1
//		}
//		else{
//			return -1; //Error
//		}
	}
}

int espacioPaginas(int idSegmento){
	int cantSegmentos;

	for(int i = 0; i < cantSegmentos; i++){
//		if(tablaSegmentos[i].pid = idSegmento){
//			return (list_size(tablaSegmentos[i].paginas) * tam_pagina);
//		} else{
//			return -1; //Error
//		}
	}
}


