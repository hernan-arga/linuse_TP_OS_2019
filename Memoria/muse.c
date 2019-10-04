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
	tam_mem = pconfig->tamanio_memoria; //Ver de poner como define
	tam_pagina = pconfig->tamanio_pag; //Ver de poner como define

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
	//crearTablaPaginas(segmentoInicial); -En un futuro creo se haria asi
	//por ahora dejo un char* en el segmento para ocuparme de otras cosas

	*(&memoriaPrincipal + sizeof(metadata)) = segmentoInicial;

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

void *musemalloc(uint32_t tamanio){
	return NULL;
}

//Funcion recorre buscando heapmetadata libre mayor o igual a cierto size

void *buscarEspacioLibre(uint32_t tamanio){
	void *pos;
	pos = &memoriaPrincipal;
	void *end;
	end = &memoriaPrincipal + tam_mem;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	uint32_t tamanioData;

	while(pos != end){
		memcpy(&memoriaPrincipal,metadata,sizeof(struct HeapMetadata));

		if((*metadata).isFree == true && (*metadata).size <= tamanio){
			//el espacio me sirve y lo asigno
			//cambio valor isFree y retorno posicion
		}
		else{
			tamanioData = (*metadata).size;
			pos = &memoriaPrincipal + sizeof(metadata) + tamanioData; //Me muevo al proximo header
		}
	}

	return NULL;
}

//Funcion para unificar dos headers - se la llama despues de un free. Unifica desde el header 1
//recorriendo todos los headers DEL SEGMENTO. Le envio como parametro el id del segmento que
//tiene que recorrer

/*void unificarHeaders(struct HeapMetadata *header1, struct HeapMetadata *header2){ //
	uint32_t size1 = header1->size;
	uint32_t size2 = header2->size;

	header1->size = size1 + size2;

	free(header2); //Creo que no es necesario hacer free de loque corresponde a la data
	//ya que se pisaria despues y listo - ver
}*/

void unificarHeaders(int idSegmento){
	int tamanioSegmento = calcularTamanioSegmento(idSegmento); //va a devolver el tamanio del segmento
	//que es el tamanio que voy a tener que recorrer sin salirme. Dentro del while

	void *pos;
	pos = buscarPosicionSegmento(idSegmento);
	void *end;
	end = pos + tamanioSegmento;
	struct HeapMetadata *header;
	struct HeapMetadata *siguienteHeader;

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

void *buscarPosicionSegmento(int idSegmento){
	//recorre los segmentos anteriores calculando su size con calcularSizeSegmento(unSegmento)
	//size de su t_list de paginas por su cantidad de paginas y la posicion del segmento
	//es el inicio de la memoria + los tamaños de sus segmentos anteriores
	void *pos;
	pos = &memoriaPrincipal;
	int cantSegmentos = list_size(tablaSegmentos);

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
	int cantidadSegmentos = list_size(tablaSegmentos);

	for(int i = 0; i < cantidadSegmentos; i++){
//		if(tablaSegmentos[i].pid = idSegmento){
//			return sizeof(int) + espacioPaginas(idSegmento); //Falta chequeo que el espacio de paginas no sea -1
//		}
//		else{
//			return -1; //Error
//		}
	}
}

int espacioPaginas(int idSegmento){
	int cantidadSegmentos = list_size(tablaSegmentos);

	for(int i = 0; i < cantidadSegmentos; i++){
//		if(tablaSegmentos[i].pid = idSegmento){
//			return (list_size(tablaSegmentos[i].paginas) * tam_pagina);
//		} else{
//			return -1; //Error
//		}
	}

}


