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
	cantidadFrames = tam_mem / tam_pagina;

	//Se hace una unica vez
	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	//Se hace una unica vez
	crearBitmapFrames();
	//Se hace una unica vez
	tablasSegmentos = dictionary_create();

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

	if (dictionary_has_key(tablasSegmentos, (char*) idSocketCliente) == false) {
		t_list *listaDeSegmentos = list_create();

		//Creo la estructura, pero no tiene segmentos aun
		dictionary_put(tablasSegmentos, (char*) idSocketCliente,
				listaDeSegmentos);
	}

}

///////////////Bitmap de Frames///////////////

void crearBitmapFrames() {
	//Bitarray de cantidad frames posiciones - en bytes
	double framesEnBytes = cantidadFrames / 8;
	int bytesNecesarios = ceil(framesEnBytes);
	void* bitsBitmap = malloc(bytesNecesarios);

	bitmapFrames = bitarray_create(bitsBitmap, bytesNecesarios);
}

/*Pone todos los bits del bitarray en 0*/
void inicializarBitmapFrames() {

	for (int i = 0; i < bitarray_get_max_bit(bitmapFrames); i++) {
		bitarray_clean_bit(bitmapFrames, i);
	}

}

bool frameEstaLibre(int indiceFrame) {

	if (bitarray_test_bit(bitmapFrames, indiceFrame) == 0) {
		return true;
	} else {
		return false;
	}

}

/*int ocuparFrame(int indiceFrame, uint32_t tamanio) {

	if (frameEstaLibre(indiceFrame)) { //Esta libre o tiene algo de espacio libre

		void *pos = &memoriaPrincipal + (indiceFrame * tam_pagina); //Me posiciono en el frame
		struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
		memcpy(metadata, pos, sizeof(struct HeapMetadata)); //Leo el primer header
		void *end = pos + tam_pagina; //Calculo el final del frame para recorrer SOLO los headers del frame

		while (pos < end) {

			if (metadata->isFree == true) { //Si esta libre y tiene el tamaño, ocupo el header
				if (metadata->size >= tamanio) {
					metadata->isFree = false;
					int espacioFrameDisponible = metadata->size - tamanio;
					metadata->size = metadata->size - tamanio;

					if (espacioFrameDisponible >= sizeof(struct HeapMetadata)) {
						//Veo si me entra un header, me muevo a la pos del prox header y lo creo
						int size = metadata->size;
						pos = pos + sizeof(struct HeapMetadata) + size;

						struct HeapMetadata *nuevoHeader = malloc(
								sizeof(struct HeapMetadata));
						nuevoHeader->isFree = true;
						nuevoHeader->size = espacioFrameDisponible
								- sizeof(struct HeapMetadata);

						pos = nuevoHeader;

						if (estaOcupadoCompleto(indiceFrame) == true) { //Chequeo si el espacio ACTUAL
						 //restante es 0, de ser asi lo marco
						 //como ocupado
							//Ocupar posicion bitarray
						}

					}

					return 0; //Exito - se pudo asignar la data
				}
			} else { //Como no esta libre, me muevo al proximo header
				int size = metadata->size;
				pos = pos + sizeof(struct HeapMetadata) + size; //Me muevo al siguiente header
				memcpy(metadata, pos, sizeof(struct HeapMetadata)); //Leo el siguiente header
			}
		}

		return -1; //Error - no encontro el espacio necesario

	} else {

		return -1; //Error

	}

}*/

void ocuparFrame(int unFrame) {
	bitarray_set_bit(bitmapFrames, unFrame);
}

void liberarFrame(int unFrame) {
	bitarray_clean_bit(bitmapFrames, unFrame);
}

//No sirve con lo nuevo, pero lo dejo para funciones heapmetadata - puede servir
bool estaOcupadoCompleto(int indiceFrame) {
	void *pos = retornarPosicionMemoriaFrame(indiceFrame);
	void *end = &pos + tam_pagina; //Comienzo del proximo frame
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));

	while (pos < end) {
		memcpy(metadata, pos, sizeof(struct HeapMetadata));

		if (metadata->isFree == true && metadata->size > 0) { //Si encuentra un espacio libre mayor a 0
			return false;
		}

		int size = metadata->size;
		pos = pos + sizeof(struct HeapMetadata) + size;
	} //Si sale de este while sin retorno, significa que no encontro espacio libre

	return true;

}

/* Recorre los frames en orden buscando uno libre y retorna el indice
 * del primero libre, de no haber libres retorna -1 (memoria virtual)
 * */
int buscarFrameLibre() {

	for (int i = 0; i < bitarray_get_max_bit(bitmapFrames); i++) {
		if (bitarray_test_bit(bitmapFrames, i) == 0) {
			return i;
		}
	}

	return -1;
}

/*Retorna la posicion de memoria donde comienza un frame en particular*/
void *retornarPosicionMemoriaFrame(int unFrame) {
	int offset = tam_pagina * unFrame; //Los frames estan en orden y se recorren de may a men

	return ((&memoriaPrincipal) + offset);
}

int espacioLibreFrame(int unFrame) {
	void *pos = retornarPosicionMemoriaFrame(unFrame);
	void *end = retornarPosicionMemoriaFrame(unFrame + 1); //Comienzo del proximo frame
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));

	while (pos < end) {
		memcpy(metadata, pos, sizeof(struct HeapMetadata));

		if (metadata->isFree == true) {
			return metadata->size;
		} else {
			int unSize = metadata->size;

			pos = pos + sizeof(struct HeapMetadata) + unSize; //Se mueve al proximo heapmetadata
		}
	}

	//Si sale sin retornar, no encontro espacio libre
	return -1;
}

///////////////Funciones MUSE///////////////

//MUSE INIT
//La llamamos en muse init, le crea al proceso (id) la tabla de segmentos correspondiente

int museinit(int idSocketCliente) {
	//int idSocketCliente = getpeername();

	if (hayMemoriaDisponible() == false) { /*Si no hay mas mm ppal ni mm virtual, error*/
		crearTablaSegmentosProceso(idSocketCliente); //Se le crea la tabla de segmentos igual, complicara mas adelante

		return -1;
	} else {
		crearTablaSegmentosProceso(idSocketCliente);

		return 0;
	}

}

//Chequea si hay mm ppal o mm virtual disponible - A DESARROLLAR
bool hayMemoriaDisponible() {

	return true;
}

//MUSE ALLOC

void *musemalloc(uint32_t tamanio, int idSocketCliente) {
	//hago una respuesta fija para que ya tengamos bien los tipos que tengamos que devolver
	void *respuesta = NULL;
	//int idSocketCliente = getpeername();

	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);
	int cantidadSegmentosARecorrer = list_size(segmentosProceso);

	if (list_is_empty(segmentosProceso)) {
		//Si no tiene ningun segmento, se lo creo
		struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
		unSegmento = crearSegmento(tamanio, idSocketCliente); /*Se crea un segmento con el minimo
		 *de frames necesarios para alocar
		 *tamanio*/

		struct Pagina *primeraPagina = malloc(sizeof(struct Pagina));
		primeraPagina = list_get(unSegmento->tablaPaginas, 0);

		retornarPosicionMemoriaFrame(primeraPagina->numeroFrame);

	} else {
		//Sino, recorro segmentos buscando ultimo header free
		for (int i = 0; i < cantidadSegmentosARecorrer; i++) {
			struct Segmento *unSegmento2 = malloc(sizeof(struct Segmento));
			unSegmento2 = list_get(segmentosProceso, i);

			if (poseeTamanioLibre(unSegmento2, tamanio)) {
				asignarEspacioLibre(unSegmento2, tamanio); //Se hace el return de la posicion asignada
			}

		}

		//Si sale del for sin retorno, tengo que buscar algun segmento de heap
		//que se pueda extender -siguiente for-

		for (int j = 0; j < cantidadSegmentosARecorrer; j++) {
			struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
			unSegmento = list_get(segmentosProceso, j);

			if (esExtendible(segmentosProceso, j)) { //Chequeo si este segmento puede extenderse
				//busco un frame libre en mi bitmap de frames
				//asigno la data y retorno la posicion donde COMIENZA LA DATA

				return respuesta; //Momentaneo para que no rompa
			}
		}
	}

	//Si sale de todas las condiciones sin retorno, es que no encontro espacio libre ni
	//espacio que se pueda extender, por lo que debera crear un segmento nuevo

	//Creo segmento nuevo y retorno la posicion asignada
	struct Segmento *nuevoSegmento = crearSegmento(tamanio, idSocketCliente);

	return posicionMemoriaUnSegmento(nuevoSegmento); //Funcion temporal - tengo que consultar
}

struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente) {
	//Comentado porque creo nos manejaremos con idSocket
	//char* idProceso = string_new();
	//string_append(&idProceso, string_itoa(idSocketCliente));

	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);
	struct Segmento *nuevoSegmento = malloc(sizeof(struct Segmento));

	//Identificar segmento
	if (list_is_empty(listaSegmentosProceso)) {
		//Si es el primer segmento le pongo id 0, sino el id incremental que le corresponda
		nuevoSegmento->id = 0;
	} else {
		nuevoSegmento->id = list_size(listaSegmentosProceso) + 1;
	}

	if (list_is_empty(listaSegmentosProceso)) {
		//Si es el primer segmento, su base logica es 0
		nuevoSegmento->baseLogica = 0;
	} else {
		//Obtengo el tamaño del ultimo segmento
		int idSegmento = list_size(listaSegmentosProceso) - 1; //Id ultimo segmento
		nuevoSegmento->baseLogica = obtenerTamanioSegmento(idSegmento,
				idSocketCliente) + 1;
	}

	/*Asignar frames necesarios para *tamanio*, calculo paginas necesarias y le calculo
	 *el techo, asigno paginas y sus correspondientes frames*/
	int paginasNecesarias;
	double paginas;
	paginas = tamanio / tam_pagina;
	paginasNecesarias = (int) (ceil(paginas)); /*Funcion techo para definir el minimo de
	 *paginas/frames que necesito, expresando
	 *paginas/frames en un numero entero*/
	nuevoSegmento->tablaPaginas = list_create();

	while (paginasNecesarias > 0) {

		if (tamanio > tam_pagina) {
			asignarNuevaPagina(nuevoSegmento, tam_pagina);
			tamanio = tamanio - tam_pagina; //headers?
		} else {
			asignarNuevaPagina(nuevoSegmento, tamanio);
		}

		paginasNecesarias--;

		//Gestion del tamaño ocupado en cada frame, ultimo heapmetadata a crear --ocuparFrame
	}

	list_add(listaSegmentosProceso, nuevoSegmento);

	return nuevoSegmento;
}

void asignarNuevaPagina(struct Segmento *unSegmento, uint32_t tamanio) {
	struct Pagina *nuevaPagina = malloc(sizeof(struct Pagina));

	//CHEQUEAR VALORES
	nuevaPagina->modificado = false;
	nuevaPagina->presencia = 1;
	nuevaPagina->uso = false;

	int nuevoFrame = buscarFrameLibre(); //tamanio

	if (nuevoFrame == -1) {
		//Memoria virtual?
	} else {
		nuevaPagina->numeroFrame = nuevoFrame;
		ocuparFrame(nuevoFrame);
		//gestionar tamanio y heapmetadata
	}

}

/* Recorre el espacio del segmento buscando espacio libre. Debe encontrar un header
 * que isFree = true y size >= tamanio + 5 (5 para el proximo header) */

int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio) {
	t_list *paginasSegmento = unSegmento->tablaPaginas;

	for (int i = 0; i < list_size(paginasSegmento); i++) {
		struct Pagina *pagina = list_get(paginasSegmento, i);

		if (frameEstaLibre(pagina->numeroFrame)
				&& espacioLibreFrame(pagina->numeroFrame) >= (tamanio + 5)) {
			return true;
		}
	}

	//Si sale del false sin retorno, no encontro ninguna pagina en el segmento que sirva
	return false;
}

void *asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio) {
	t_list *paginasSegmento = unSegmento->tablaPaginas;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	void *pos;
	void *end;

	for (int i = 0; i < list_size(paginasSegmento); i++) {
		struct Pagina *pagina = list_get(paginasSegmento, i);

		if (frameEstaLibre(pagina->numeroFrame)
				&& espacioLibreFrame(pagina->numeroFrame) >= (tamanio + 5)) {
			//Busco el espacio libre del frame
			pos = retornarPosicionMemoriaFrame(pagina->numeroFrame);
			end = retornarPosicionMemoriaFrame(pagina->numeroFrame + 1);

			//Recorro el frame actual buscando la posicion libre
			while (pos < end) {
				memcpy(metadata, pos, sizeof(struct HeapMetadata));

				if (metadata->isFree == true
						&& metadata->size >= (tamanio + 5)) {
					int unSize = metadata->size;

					//Ocupo el heapmetadata que encontre libre
					metadata->isFree = false;
					metadata->size = tamanio;

					//Me muevo adonde debo crear el proximo heapmetadata
					pos = pos + sizeof(struct HeapMetadata) + unSize;

					//Creo el proximo heapmetadata
					struct HeapMetadata *nuevoMetadata = malloc(
							sizeof(struct HeapMetadata));
					nuevoMetadata->isFree = true;
					nuevoMetadata->size = unSize - tamanio
							- sizeof(struct HeapMetadata);
				}
			}

			return retornarPosicionMemoriaFrame(pagina->numeroFrame);
		}
	}

	return NULL; //Nunca deberia llegar a retornar NULL pq ya se chequeo anteriorimente que hubiera espacio libre
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

/*Retorna la posicion de memoria donde comienza la primera pagina de un segmento*/
void *posicionMemoriaUnSegmento(struct Segmento *unSegmento) {
	t_list *paginas = unSegmento->tablaPaginas;
	struct Pagina *primeraPagina = list_get(paginas, 0);

	return retornarPosicionMemoriaFrame(primeraPagina->numeroFrame);
}

/*Funcion recorre buscando heapmetadata libre - mayor o igual - a cierto size*/
void *buscarEspacioLibre(uint32_t tamanio) {
	void *pos;
	pos = &memoriaPrincipal;
	void *end;
	end = &memoriaPrincipal + tam_mem;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	uint32_t tamanioData;

	while (pos != end) {
		memcpy(&memoriaPrincipal, metadata, sizeof(struct HeapMetadata));
		tamanioData = (*metadata).size;

		if ((*metadata).isFree == true && (*metadata).size <= tamanio) {
			//el espacio me sirve y lo asigno
			//cambio valor isFree y retorno posicion
			(*metadata).isFree = false;
			(*metadata).size = tamanio;

			if ((tamanioData - tamanio) >= sizeof(struct HeapMetadata)) {
				struct HeapMetadata *nuevoHeader = malloc(
						sizeof(struct HeapMetadata));
				nuevoHeader->isFree = true;
				nuevoHeader->size = tamanioData - tamanio
						- sizeof(struct HeapMetadata);

				pos = pos + sizeof(struct HeapMetadata) + tamanio;
				*(&pos) = nuevoHeader; //ver
			}
		} else {
			pos = &memoriaPrincipal + sizeof(struct HeapMetadata) + tamanioData; //Me muevo al proximo header
		}
	}

	return NULL;
}

/**Funcion para unificar dos headers - se la llama despues de un free. Unifica desde el header 1
 * recorriendo todos los headers DEL SEGMENTO. Le envio como parametro el id del segmento que
 * tiene que recorrer*/
void unificarHeaders(int idSocketCliente, int idSegmento) {
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, (char*)idSocketCliente);

	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	segmento = list_get(segmentosProceso, idSegmento);

	t_list *paginasSegmento = segmento->tablaPaginas;
	int numeroPagina = 0;
	int paginasARecorrer = list_size(paginasSegmento);

	struct Pagina *primeraPagina = malloc(sizeof(struct Pagina));
	primeraPagina = list_get(paginasSegmento, 0);

	void *posInicio = retornarPosicionMemoriaFrame(primeraPagina->numeroFrame);
	void *pos = posInicio;

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));

	//Antes de entrar a este for, estoy en la pagina 0, se que tengo metadata que leer
	memcpy(metadata,posInicio,sizeof(struct HeapMetadata));
	while(numeroPagina <= paginasARecorrer){
		int sizeARecorrer = metadata->size;

		if(metadata->isFree == true){

			if(metadata->size < (tam_pagina - sizeof(struct HeapMetadata)) ){ //Si el proximo header esta dentro de la misma pag
				pos = pos + sizeof(struct HeapMetadata) + metadata->size;

				struct HeapMetadata *otraMetadata = malloc(sizeof(struct HeapMetadata));
				memcpy(otraMetadata,pos,sizeof(struct HeapMetadata));

				if(otraMetadata->isFree == true){ //Y si ese proximo header esta libre
					metadata->size = metadata->size + otraMetadata->size; //Los unifico
					//Se esta borrando la metadata vieja? VER CON NACHIN
				}
			} else{ //Si el proximo header esta en otra pagina de las siguientes
				int tamanioAMoverme = metadata->size;
				int paginasQueContienenSizeMetadata = (ceil)(tamanioAMoverme/tam_pagina);

				//Me muevo al proximo frame (consumo una pagina entera)
				if(paginasQueContienenSizeMetadata > 1){
					tamanioAMoverme = tamanioAMoverme - (tam_pagina - sizeof(struct HeapMetadata));
					paginasQueContienenSizeMetadata--;

					struct Pagina *proximaPagina = malloc(sizeof(struct Pagina));
					numeroPagina++;
					proximaPagina = list_get(paginasSegmento, numeroPagina);
					pos = retornarPosicionMemoriaFrame(proximaPagina->numeroFrame);
				}

				for(int i = paginasQueContienenSizeMetadata; i = 0; i--){

					if(tamanioAMoverme < tam_pagina ) { //significa que ya es la ultima pagina

						pos = pos + tamanioAMoverme;
						//leer heapmetadata siguiente
						struct HeapMetadata *metadataFinal = malloc(sizeof(struct HeapMetadata));

						if(metadataFinal->isFree == true){
							//Agrupo en la metadata original
							metadata->size = metadata->size + metadataFinal->size;
							//borrar el metadata final ?????????????
						}

					} else { //Me muevo de pagina
						tamanioAMoverme = tamanioAMoverme - tam_pagina;
						struct Pagina *proximaPagina = malloc(sizeof(struct Pagina));
						numeroPagina++;
						proximaPagina = list_get(paginasSegmento, numeroPagina);
						pos = retornarPosicionMemoriaFrame(proximaPagina->numeroFrame);
					}

				}

			}

		}

	}

}


/*
 Recorre los segmentos anteriores calculando su size con calcularSizeSegmento(unSegmento)
 size de su t_list de paginas por su cantidad de paginas y la posicion del segmento
 es el inicio de la memoria + los tamaños de sus segmentos anteriores

 void *buscarPosicionSegmento(int idSocketCliente, int idSegmento) {
 t_list *tablaSegmentosProceso = dictionary_get(tablasSegmentos, (char*)idSocketCliente);
 int offset = 0;
 int indice = 0;

 struct Segmento *unSegmento = list_get(tablaSegmentosProceso, indice);

 while (unSegmento->id != idSegmento) {
 offset = offset + obtenerTamanioSegmento(idSocketCliente, unSegmento->id); Sumo el tamaño del segmento
 indice++;
 unSegmento = list_get(tablaSegmentosProceso, indice); Leo el proximo segmento
 }

 return &memoriaPrincipal + offset; //Esta MAL - modificar - respuesta mail
 }
 */

/*Funcion que me retorna el tamaño -actual- de un segmento determinado de un proceso determinado*/
uint32_t obtenerTamanioSegmento(int idSegmento, int idSocketCliente) {
	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);
	struct Segmento *unSegmento = malloc(sizeof(struct HeapMetadata));

	unSegmento = list_get(listaSegmentosProceso, idSegmento);

	return unSegmento->tamanio;
}

/*Funcion que me retorna el espacio ocupado por las paginas de un segmento de un
 *proceso determinado*/
int espacioPaginas(int idSocketCliente, int idSegmento) {
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);
	struct Segmento *unSegmento = list_get(segmentosProceso, idSegmento); //list find segmento

	return (list_size(unSegmento->tablaPaginas) * tam_pagina);
}

void *buscarEspacioLibreProceso(int idSocketCliente, uint32_t tamanio) {
	/* Si un segmento posee espacio libre, es en el frame de la ultima pagina
	 * de algun segmento (que es donde se presenta fragmentacion interna)*/

	/* Siempre tengo que buscar espacio libre para el tamaño que necesito
	 * mas el tamaño que ocupa la nueva metadata (5 bytes)*/
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);
	int cantidadARecorrer = list_size(segmentosProceso);

	for (int i = 0; i < cantidadARecorrer; i++) {
		struct Segmento *segmento = malloc(sizeof(struct Segmento));
		segmento = list_get(segmentosProceso, i);

		struct Pagina *ultimaPagina = malloc(sizeof(struct Pagina));
		int cantidadPaginas = list_size(segmento->tablaPaginas);
		ultimaPagina = list_get(segmento->tablaPaginas, cantidadPaginas - 1);

		int frame = ultimaPagina->numeroFrame;

		/*FALTA DESARROLLAR ACA*/
	}

	return NULL; //Momentaneo
}

//MUSE GET

/**
 * Copia una cantidad `n` de bytes desde una posición de memoria de MUSE a una `dst` local.
 * @param dst Posición de memoria local con tamaño suficiente para almacenar `n` bytes.
 * @param src Posición de memoria de MUSE de donde leer los `n` bytes.
 * @param n Cantidad de bytes a copiar.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */

int museget(void* dst, uint32_t src, size_t n, int idSocketCliente) {
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
	struct Pagina *unaPagina = malloc(sizeof(struct Pagina));

	//int idSocketCliente /*= obtenerlo (?)*/;  //El socketCliente lo recibimos directo desde el utils.
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);

	//Obtencion segmento, pagina, frame, desplazamiento
	int idSegmento;
	int frame;
	int desplazamiento;

	int direccion = (int) src; //Casteo a int la direccion recibida

	//Obtencion desplazamiento
	desplazamiento = direccion % tam_pagina;

	//Obtencion idSegmento y segmento
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, src);
	unSegmento = list_get(listaSegmentos, idSegmento);

	//Obtencion pagina y frame
	unaPagina = paginaQueContieneDireccion(unSegmento, src); //Me retorna directamente la pagina
	frame = unaPagina->numeroFrame;
	//////////////////////////////////////////////////

	/*Ya se tienen todos los datos necesarios y se puede ir a mm ppal a buscar la data*/
	void *pos = retornarPosicionMemoriaFrame(frame) + desplazamiento;
	memcpy((void*) dst, pos, n);

	return 0;
}

int idSegmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion) {
	int segmentosARecorrer = list_size(listaSegmentos);
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));

	for (int i = 0; i < segmentosARecorrer; i++) {
		unSegmento = list_get(listaSegmentos, i);

		if (((int) direccion) > unSegmento->baseLogica
				&& ((int) direccion) < unSegmento->tamanio) { //Chequear si en base logica tengo en cuenta heap o no
			return unSegmento->id;
		}
	}

	//Si sale del for sin retorno, no hay ningun segmento que contenga esa direc
	return -1; //error
}

struct Pagina *paginaQueContieneDireccion(struct Segmento *unSegmento,
		void *direccion) {
	t_list *listaPaginas = unSegmento->tablaPaginas;
	int indicePagina;

	/*Obtencion Pagina: (base logica segmento - direccion) / tam_pagina (pag es el resultado piso)*/
	indicePagina = floor(
			((unSegmento->baseLogica) - ((int) direccion)) / tam_pagina);

	struct Pagina *pagina = list_get(listaPaginas, indicePagina);

	return pagina;
}

//MUSE CPY

int musecpy(uint32_t dst, void* src, int n, int idSocketCliente) {
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
	struct Pagina *unaPagina = malloc(sizeof(struct Pagina));

	//int idSocketCliente /*= obtenerlo (?)*/; //El socketCliente lo recibimos directo desde el utils.

	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			(char*) idSocketCliente);

	//Obtencion segmento, pagina, frame, desplazamiento
	int idSegmento;
	int frame;
	int desplazamiento;

	int direccion = (int) dst; //Casteo a int la direccion recibida

	//Obtencion desplazamiento
	desplazamiento = direccion % tam_pagina;

	//Obtencion idSegmento y segmento
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, dst);
	unSegmento = list_get(listaSegmentos, idSegmento);

	//Obtencion pagina y frame
	unaPagina = paginaQueContieneDireccion(unSegmento, dst); //Me retorna directamente la pagina
	frame = unaPagina->numeroFrame;
	//////////////////////////////////////////////////

	/*Ya se tienen todos los datos necesarios y se puede ir a mm ppal a buscar la data*/
	void *pos = retornarPosicionMemoriaFrame(frame) + desplazamiento;
	memcpy(pos, src, n);

	return 0;
}

