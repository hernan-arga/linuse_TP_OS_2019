#include <stdio.h>
#include "muse.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

int main(){

	// Levanta archivo de configuracion - Se hace una unica vez
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
	cantidadFrames = tam_mem / tam_pagina;

	//Se hace una unica vez
	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	//Se hace una unica vez
	inicializarBitmapFrames(bitmapFrames[cantidadFrames]);
	//Se hace una unica vez
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

void crearTablaSegmentosProceso(char *idProceso){ //Id proceso = id + ip
	t_list *listaDeSegmentos = list_create();

	dictionary_put(tablasSegmentos,idProceso,listaDeSegmentos); //Creo la estructura, pero no tiene segmentos aun
}

///////////////Bitmap de Frames///////////////

void inicializarBitmapFrames(int bitmapFrames[cantidadFrames]){

	for (int i = 0; i < cantidadFrames; i++){
		bitmapFrames[i] = 0;
	}

}

bool estaLibre(int indiceFrame){

	if(bitmapFrames[indiceFrame] == 0){
		return true;
	} else{
		return false;
	}

}

int ocuparFrame(int indiceFrame, uint32_t tamanio){

	if(bitmapFrames[indiceFrame] == 0){ //Esta libre o tiene algo de espacio libre

		void *pos = &memoriaPrincipal + (indiceFrame * tam_pagina); //Me posiciono en el frame
		struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
		memcpy(metadata,pos,sizeof(struct HeapMetadata)); //Leo el primer header
		void *end = pos + tam_pagina; /*Calculo el final del frame para recorrer SOLO los headers del frame*/

		while(pos < end){

			if(metadata->isFree == true){ //Si esta libre y tiene el tamaño, ocupo el header
				if(metadata->size >= tamanio){
					metadata->isFree = false;
					int espacioFrameDisponible = metadata->size - tamanio;
					metadata->size = metadata->size-tamanio;

					if(espacioFrameDisponible >= sizeof(struct HeapMetadata)){
						//Veo si me entra un header, me muevo a la pos del prox header y lo creo
						int size = metadata->size;
						pos = pos + sizeof(struct HeapMetadata) + size;

						struct HeapMetadata *nuevoHeader = malloc(sizeof(struct HeapMetadata));
						nuevoHeader->isFree = true;
						nuevoHeader->size = espacioFrameDisponible - sizeof(struct HeapMetadata);

						pos = nuevoHeader;

						if(estaOcupadoCompleto(indiceFrame) == true){ /*Chequeo si el espacio ACTUAL
						restante es 0, de ser asi lo marco como ocupado*/
							bitmapFrames[indiceFrame] = 1;
						}


					}

					return 0; //Exito - se pudo asignar la data
				}
			} else{ //Como no esta libre, me muevo al proximo header
				int size = metadata->size;
				pos = pos + sizeof(struct HeapMetadata) + size; //Me muevo al siguiente header
				memcpy(metadata,pos,sizeof(struct HeapMetadata)); //Leo el siguiente header
			}
		}

		return -1; //Error - no encontro el espacio necesario

	} else{

		return -1; //Error

	}

}

bool estaOcupadoCompleto(int indiceFrame){
	void *pos = retornarPosicionMemoriaFrame(indiceFrame);
	void *end = &pos + tam_pagina; //Comienzo del proximo frame
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));

	while(pos < end){
		memcpy(metadata,pos,sizeof(struct HeapMetadata));

		if(metadata->isFree == true && metadata->size > 0){ //Si encuentra un espacio libre mayor a 0
			return false;
		}

		int size = metadata->size;
		pos = pos + sizeof(struct HeapMetadata) + size;
	} //Si sale de este while sin retorno, significa que no encontro espacio libre

	return true;

}

void liberarFrame(int indiceFrame){
	bitmapFrames [indiceFrame] = 0;
}

/* Recorre los frames en orden buscando uno libre y retorna el indice
 * del primero libre, de no haber libres retorna -1 (memoria virtual)
 * */
int buscarFrameLibre(){

	for(int i = 0; i < cantidadFrames; i++){
		if(bitmapFrames[i] == 0){
			return i;
		}
	}

	return -1;
}

void *retornarPosicionMemoriaFrame(int unFrame){
	int offset = tam_pagina * unFrame; //Los frames estan en orden y se recorren de may a men

	return ((&memoriaPrincipal) + offset + sizeof(struct HeapMetadata)); /*Le sumo los primeros
	bytes que se encuentran ocupados por una metadata indicando que está ocupado y cuánto*/
}

///////////////Funciones MUSE///////////////

//MUSE INIT
//La llamamos en muse init, le crea al proceso (id) la tabla de segmentos correspondiente

int museinit(int id, char* ip/*, int puerto*/){
	//Creo que no necesito el puerto - lo comento para consultar
	//Funcionara como id del proceso y sera la key en el diccioario
	char* idProceso = string_new();
	string_append(&idProceso,string_itoa(id));
	string_append(&idProceso,ip);

	crearTablaSegmentosProceso(idProceso);

	return 0; //Retorna -1 ante un error
}


//MUSE ALLOC

void *musemalloc(uint32_t tamanio, int idSocketCliente){
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, (char*)idSocketCliente);
	int cantidadARecorrer = list_size(segmentosProceso);

	if(list_is_empty(segmentosProceso)){

		struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
		unSegmento = crearSegmento(tamanio, idSocketCliente);  /*Se crea un segmento con el minimo
		de frames necesarios para alocar tamanio*/

		struct Pagina *primeraPagina = malloc(sizeof(struct Pagina));
		primeraPagina = list_get(unSegmento->tablaPaginas, 0);

		retornarPosicionMemoriaFrame(primeraPagina->numeroFrame);

	} else{

		for(int i = 0; i < cantidadARecorrer; i++){
			struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
			unSegmento = list_get(segmentosProceso,i);

			if(poseeTamanioLibre(unSegmento,tamanio)){
				asignarEspacioLibre(unSegmento, tamanio); //Se hace el return de la posicion asignada
			}

			return NULL; //Momentaneo para que no rompa
		}

		//Si sale del for sin retorno, tengo que buscar algun segmento de heap
		//que se pueda extender -siguiente for-

		for(int j = 0; j < cantidadARecorrer; j++){
			struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
			unSegmento = list_get(segmentosProceso,j);

			if(esExtendible(segmentosProceso,j)){ //Chequeo si este segmento puede extenderse
				//busco un frame libre en mi bitmap de frames
				//asigno la data y retorno la posicion donde COMIENZA LA DATA

				return NULL; //Momentaneo para que no rompa
			}
		}

		//Si sale de este for sin retorno, es que no encontro espacio libre ni
		//espacio que se pueda extender, por lo que debera crear un segmento nuevo

		//creo segmento nuevo y retorno la posicion asignada

		return NULL; //Momentaneo para que no rompa
	}

	return NULL;
}

struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente){
	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos, (char*)idSocketCliente);
	struct Segmento *nuevoSegmento = malloc(sizeof(struct Segmento));

	//Identificar segmento
	if(list_is_empty(listaSegmentosProceso)){
		nuevoSegmento->id = 0;
	} else{
		nuevoSegmento->id = list_size(listaSegmentosProceso) + 1;
	}

	//Nuevo segmento - base logica ??

	//Asignar frames necesarios para *tamanio*
	int paginasNecesarias;
	double paginas;
	paginas = tamanio/tam_pagina;
	paginasNecesarias = (int)(ceil(paginas)); /*Funcion techo para definir el minimo de
												paginas/frames que necesito, expresado
												paginas/frames en un numero entero*/
	nuevoSegmento->tablaPaginas = list_create();

	while(paginasNecesarias > 0){

		if(tamanio > tam_pagina){
			asignarNuevaPagina(nuevoSegmento,tam_pagina);
			tamanio = tamanio - tam_pagina; //headers?
		} else{
			asignarNuevaPagina(nuevoSegmento,tamanio);
		}

		paginasNecesarias --;

		//Gestion del tamaño ocupado en cada frame, ultimo heapmetadata a crear --ocuparFrame
	}

	list_add(listaSegmentosProceso, nuevoSegmento);
	return nuevoSegmento;
}

void asignarNuevaPagina(struct Segmento *unSegmento, uint32_t tamanio){
	struct Pagina *nuevaPagina = malloc(sizeof(struct Pagina));

	//CHEQUEAR VALORES
	nuevaPagina->modificado = false;
	nuevaPagina->presencia = 1;
	nuevaPagina->uso = false;

	int nuevoFrame = buscarFrameLibre(); //tamanio

	if(nuevoFrame == -1){
		//Memoria virtual?
	} else{
		nuevaPagina->numeroFrame = nuevoFrame;
		ocuparFrame(nuevoFrame,tamanio-5);
	}

}

/* Recorre el espacio del segmento buscando espacio libre (desde la base logica hasta
 * base logica + tamanio). Debe encontrar un header que isFree = true y
 * size >= tamanio + 5 (5 para el proximo header) */

int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio){
	/*DESARROLLAR - consulta mail*/

	return 0;
}

void asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio){

}

int esExtendible(t_list *segmentosProceso,int unIndice){
	return 0;
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

	return 0; //Momentaneo para que no joda
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

	return 0; //Momentaneo para que no joda
}
