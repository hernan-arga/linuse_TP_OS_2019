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
	//t_log *log = crear_log();
	pconfig = malloc(sizeof(config));
	levantarConfigFile(pconfig);

	//log_info(log, "MUSE levantado correctamente\n");

	arrancarMemoria(pconfig);

	printf("%d \n", (int) (&memoriaPrincipal));

	printf("%d \n", sizeof(uint32_t));

	printf("%d \n", sizeof(bool));

	printf("%d \n", sizeof(struct HeapMetadata));


	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL,
			(void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);



	pthread_join(hiloLevantarConexion, NULL);

	//printf("%d", (int) (&memoriaPrincipal));

	return 0;
}


//////////////Funciones inicio recursos////////////////////

void arrancarMemoria(config* pconfig) {

	tam_mem = pconfig->tamanio_memoria; //Ver de poner como define
	tam_pagina = pconfig->tamanio_pag; //Ver de poner como define
	tam_swap = pconfig->tamanio_swap;
	punteroClock = 0; //Asi cuando se ejecute el algoritmo por primera vez, va a estar iniciado en 0
	cantidadFrames = tam_mem / tam_pagina;
	cantidadPaginasSwap = tam_swap / tam_pagina;

	//Se hace una unica vez
	reservarMemoriaPrincipal(pconfig->tamanio_memoria);
	//Se hace una unica vez
	crearBitmapFrames();
	//Se hace una unica vez
	tablasSegmentos = dictionary_create();
	//Abro archivo swap
	swap = fopen("swap.txt", "a+"); //Validar modo apertura y limite tamaño tam_swap

	char *bitmap = malloc(cantidadPaginasSwap);
	bitmapSwap = bitarray_create(bitmap, cantidadPaginasSwap); //size - cantidad de bits del bitarray, expresado en bytes
	//verificar 1er parametro, swap?
	inicializarBitmapSwap();
}

void reservarMemoriaPrincipal(int tamanio) {

	memoriaPrincipal = malloc(tamanio);

}

/* Crea la tabla de segmentos -vacia- para un proceso. Solo la crea si
 * no la encuentra previamente en el diccionario (tema hilos) */
void crearTablaSegmentosProceso(int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);

	if (!dictionary_has_key(tablasSegmentos, stringIdSocketCliente)) {
		t_list *listaDeSegmentos = list_create();

		//Creo la estructura, pero no tiene segmentos aun
		dictionary_put(tablasSegmentos, stringIdSocketCliente,
				listaDeSegmentos);
	}

	free(stringIdSocketCliente);

}

///////////////Bitmap de Frames///////////////

void crearBitmapFrames() {
	//Creo una t_list y le agrego sus cantidadFrames elementos que van a quedar estaticos
	bitmapFrames = list_create();

	for (int i = 0; i < cantidadFrames; i++) {

		struct Frame *nuevoFrame = malloc(sizeof(struct Frame));

		nuevoFrame->modificado = 0;
		nuevoFrame->uso = 0;
		nuevoFrame->listaMetadata = list_create();

		list_add(bitmapFrames, nuevoFrame);

	}

}

bool frameEstaLibre(int indiceFrame) {
	struct Frame *frame = malloc(sizeof(struct Frame));
	frame = list_get(bitmapFrames, indiceFrame);

	if (frame->uso == 0) {
		return true;
	} else {
		return false;
	}
}

void ocuparFrame(int unFrame) {
	struct Frame *frame = malloc(sizeof(struct Frame));
	frame = list_get(bitmapFrames, unFrame);

	frame->uso = 1;

	list_replace(bitmapFrames, unFrame, frame); //Lo reemplazo modificado, unFrame es el indice del frame
}

void liberarFrame(int unFrame) {
	struct Frame *frame = malloc(sizeof(struct Frame));
	frame = list_get(bitmapFrames, unFrame);

	frame->uso = 0;

	list_replace(bitmapFrames, unFrame, frame); //Lo reemplazo modificado
}

int buscarFrameLibre() {
	struct Frame *frame = malloc(sizeof(struct Frame));

	for (int i = 0; i < cantidadFrames; i++) {
		frame = list_get(bitmapFrames, i);

		if (frame->uso == 0) {
			return i;
		}
	}

	return -1;
}

bool hayFramesLibres() {
	int frameLibre = buscarFrameLibre();

	if (frameLibre == -1) {
		return false;
	} else {
		return true;
	}

}

/*Retorna la posicion de memoria donde comienza un frame en particular*/
void *retornarPosicionMemoriaFrame(int unFrame) {
	int offset = tam_pagina * unFrame; //Los frames estan en orden y se recorren de menor a mayor

	int comienzoMemoria = (int) (&memoriaPrincipal);

	//return ((&memoriaPrincipal) + offset);
	return (void*)
			(comienzoMemoria + offset);
}

///////////////Funciones MUSE///////////////

//MUSE INIT
//La llamamos en muse init, le crea al proceso (id) la tabla de segmentos correspondiente

int museinit(int idSocketCliente) {

	if (!hayMemoriaDisponible()) {
		crearTablaSegmentosProceso(idSocketCliente); //Se le crea la tabla de segmentos igual, pero no hay mm

		return -1;
	} else {
		crearTablaSegmentosProceso(idSocketCliente);

		return 0;
	}

}

//Chequea si hay mm ppal o mm virtual disponible - A DESARROLLAR
bool hayMemoriaDisponible() {
	//Para la memoria principal nos va a convenir llevar un "contador" de
	//bytes libres o algo asi, va a servir despues para las metricas

	return true;
}

//MUSE ALLOC

void *musemalloc(uint32_t tamanio, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);

	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);
	int cantidadSegmentosARecorrer = list_size(segmentosProceso);
	int metadataLibre;

	if (list_is_empty(segmentosProceso)) { //Si no tiene ningun segmento, se lo creo

		struct Segmento *unSegmento;
		unSegmento = crearSegmento(tamanio, idSocketCliente); /*Se crea un segmento con el minimo
		 *de frames necesarios para alocar
		 *tamanio*/

		struct Pagina *primeraPagina; //= malloc(sizeof(struct Pagina));
		primeraPagina = list_get(unSegmento->tablaPaginas, 0);

		void *comienzoDatos = malloc(sizeof(int));
		comienzoDatos = retornarPosicionMemoriaFrame(primeraPagina->numeroFrame) + sizeof(struct HeapMetadata);



		free(stringIdSocketCliente);
		return comienzoDatos;

	} else { //Sino, recorro segmentos buscando ultimo header free
		struct Segmento *segmento = malloc(sizeof(struct Segmento));

		for (int i = 0; i < cantidadSegmentosARecorrer; i++) {

			segmento = list_get(segmentosProceso, i);

			if (poseeTamanioLibreSegmento(segmento, tamanio + sizeof(struct HeapMetadata))) {

				metadataLibre = retornarMetadataTamanioLibre(segmento, tamanio + sizeof(struct HeapMetadata));

				return (void *)(metadataLibre + sizeof(struct HeapMetadata));

			}

		}


		int ultimaMetadata;
		struct Pagina *ultimaPagina = malloc(sizeof(struct Pagina));
		struct Frame *ultimoFrame = malloc(sizeof(struct Frame));
		void *pos;
		//Si sale del for sin retorno, tengo que buscar algun segmento de heap
		//que se pueda extender -siguiente for-

		for (int j = 0; j < cantidadSegmentosARecorrer; j++) {
			struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
			unSegmento = list_get(segmentosProceso, j);

			if (esExtendible(segmentosProceso, j)) {

				unSegmento = extenderSegmento(unSegmento, tamanio);

				free(stringIdSocketCliente);

				//Retorno posicion ultima metadata + 5
				ultimaPagina = list_get(unSegmento->tablaPaginas, list_size(unSegmento->tablaPaginas));
				ultimoFrame = list_get(bitmapFrames, ultimaPagina->numeroFrame);
				ultimaMetadata = (int)list_get(ultimoFrame->listaMetadata, list_size(ultimoFrame->listaMetadata));
				pos = retornarPosicionMemoriaFrame(ultimaPagina->numeroFrame) + ultimaMetadata;

				return pos;
			}
		}
	}

	//Si sale de todas las condiciones sin retorno, es que no encontro espacio libre ni
	//espacio que se pueda extender, por lo que debera crear un segmento nuevo

	//Creo segmento nuevo y retorno la posicion asignada
	struct Segmento *nuevoSegmento = crearSegmento(tamanio, idSocketCliente);

	free(stringIdSocketCliente);
	return posicionMemoriaUnSegmento(nuevoSegmento) + sizeof(struct HeapMetadata);

}

//Crea el nuevo segmento Y ACTUALIZA todas las estructuras (lista segmentos y diccionario)
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);

	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);
	struct Segmento *nuevoSegmento = malloc(sizeof(struct Segmento));

	//Identificar segmento
	if (list_is_empty(listaSegmentosProceso)) { //Si es el primer segmento le pongo id 0, sino el id incremental que le corresponda
		nuevoSegmento->id = 0;
	} else {
		nuevoSegmento->id = list_size(listaSegmentosProceso);
	}

	if (list_is_empty(listaSegmentosProceso)) { //Si es el primer segmento, su base logica es 0

		nuevoSegmento->baseLogica = 0;

	} else { //Obtengo el tamaño del ultimo segmento

		int idUltimoSegmento = list_size(listaSegmentosProceso) - 1; //Id ultimo segmento
		nuevoSegmento->baseLogica = obtenerTamanioSegmento(idUltimoSegmento, idSocketCliente) + 1;

	}

	/*Asignar frames necesarios para *tamanio*, calculo paginas necesarias y le calculo
	 *el techo, asigno paginas y sus correspondientes frames*/
	int paginasNecesarias;
	double paginas = (double)tamanio / (double)tam_pagina;
	paginasNecesarias = (int) (ceil(paginas));
	nuevoSegmento->tablaPaginas = list_create();
	int tamanioAlocado = tamanio;

	while (paginasNecesarias > 0) {

		if (paginasNecesarias == (int) (ceil(paginas))
				|| paginasNecesarias == 1) { //Si es la primera pagina o la ultima(tener en cuenta tamaño metadata)

			if (paginasNecesarias == (int) (ceil(paginas))) { //Si es la primera pagina

				nuevoSegmento = asignarPrimeraPaginaSegmento(nuevoSegmento, tamanio);
				tamanioAlocado -= ( pconfig->tamanio_pag - sizeof(struct HeapMetadata) );

			} else {

				nuevoSegmento = asignarUltimaPaginaSegmento(nuevoSegmento, (tam_pagina - (tamanio % tam_pagina) - sizeof(struct HeapMetadata))); //tamanio prox metadata?
				tamanioAlocado = 0;

			}

		} else {

			nuevoSegmento = asignarNuevaPagina(nuevoSegmento, tam_pagina);
			tamanioAlocado = tamanioAlocado - pconfig->tamanio_pag;

		}

		paginasNecesarias--;

	}

	list_add(listaSegmentosProceso, nuevoSegmento);

	//Actualizo la data en el diccionario
	dictionary_put(tablasSegmentos, stringIdSocketCliente, listaSegmentosProceso);

	free(stringIdSocketCliente);
	return nuevoSegmento;
}

struct Segmento *extenderSegmento(struct Segmento *segmento, uint32_t tamanio) {

	//busco ultima metadata (que tiene que estar libre) y chequeo si hay que extender
	//o si alcanza, en to do caso se creara la nueva metadata que separe

	t_list *paginas = segmento->tablaPaginas;
	struct Pagina *ultimaPagina = malloc(sizeof(struct Pagina));
	ultimaPagina = list_get(paginas, list_size(paginas) - 1);
	struct Frame *frame = list_get(bitmapFrames, ultimaPagina->numeroFrame); //y si la pag no esta en mm ppal?

	int ultimaMetadata = (int)list_get(frame->listaMetadata, list_size(frame->listaMetadata));
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	memcpy(metadata, retornarPosicionMemoriaFrame(ultimaPagina->numeroFrame) + ultimaMetadata, sizeof(struct HeapMetadata));

	int paginasNecesarias;
	int bytesAAgregar;

	struct HeapMetadata *nuevaUltimaMetadata = malloc(sizeof(struct HeapMetadata));
	nuevaUltimaMetadata->isFree = true;
	nuevaUltimaMetadata->size = tamanio;

	//Modifico la ultima metadata con el nuevo size que se esta pidiendo
	memcpy(retornarPosicionMemoriaFrame(ultimaPagina->numeroFrame) + ultimaMetadata, nuevaUltimaMetadata, sizeof(struct HeapMetadata));

	bytesAAgregar = tamanio + sizeof(struct HeapMetadata) - metadata->size;
	paginasNecesarias = (int)(ceil((double)bytesAAgregar/(double)tam_pagina));

	while(paginasNecesarias > 0){

		if(bytesAAgregar >= tam_pagina){

			segmento = asignarNuevaPagina(segmento, tam_pagina);
			bytesAAgregar = bytesAAgregar - tam_pagina;

		} else{

			//asignar ultima pagina ya me crea la ultima metadata necesaria
			segmento = asignarUltimaPaginaSegmento(segmento, bytesAAgregar);
			bytesAAgregar = 0;

		}

		paginasNecesarias--;

	}

	free(ultimaPagina);
	free(frame);
	free(nuevaUltimaMetadata);
	free(nuevaUltimaMetadata);

	return segmento;
}

int retornarMetadataTamanioLibre(struct Segmento *segmento, uint32_t tamanio){

	t_list *paginas = segmento->tablaPaginas;
	t_list *metadatas;
	struct Pagina *pagina;
	struct Frame *frame;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	int desplazamiento;
	void *pos;

	for(int i = 0; i < list_size(paginas); i++){
		pagina = list_get(paginas, i);
		frame = list_get(bitmapFrames, pagina->numeroFrame);
		metadatas = frame->listaMetadata;

		for(int j = 0; j < list_size(metadatas); j++){

			desplazamiento = (int)list_get(metadatas, j);
			memcpy(metadata, retornarPosicionMemoriaFrame(pagina->numeroFrame) + desplazamiento, sizeof(struct HeapMetadata));

			if(metadata->isFree == true && metadata->size >= tamanio){
				pos = retornarPosicionMemoriaFrame(pagina->numeroFrame) + desplazamiento + sizeof(struct HeapMetadata);

				return (int)pos;
			}

		}

	}

	free(metadata);

	return -1; //nunca llega, porque antes de llamar a esta funcion, chequee que exista metadata libre
}

/*Le asigna la primera pagina que va a contener la metadata al segmento*/
struct Segmento *asignarPrimeraPaginaSegmento(struct Segmento *segmento, int tamanioMetadata) {

	struct Pagina *primeraPagina = malloc(sizeof(struct Pagina));
	primeraPagina->numeroFrame = asignarUnFrame();
	void *pos = retornarPosicionMemoriaFrame(primeraPagina->numeroFrame);

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	metadata->isFree = false;
	metadata->size = tamanioMetadata;

	//Pone la metadata en el frame correspondiente
	memcpy(pos, metadata, sizeof(struct HeapMetadata));

	list_add(segmento->tablaPaginas, primeraPagina);

	segmento->tamanio += pconfig->tamanio_pag;

	return segmento;
}

int asignarUnFrame() {

	int frame;

	if (hayFramesLibres() == true) {
		frame = buscarFrameLibre();
	} else {
		frame = clockModificado();
	}

	//FALTA
	//HACER SWAPPEO
	//DE PAGINA DESALOJADA

	struct Frame *frameReemplazo = malloc(sizeof(struct Frame));
	frameReemplazo = list_get(bitmapFrames, frame);
	frameReemplazo->modificado = 0;
	frameReemplazo->uso = 1;
	list_clean(frameReemplazo->listaMetadata); //limpia la lista para la prox pag

	list_replace(bitmapFrames, frame, frameReemplazo);

	return frame;
}

/*Le asigna la ultima pagina a un segmento, poniendo la siguiente metadata en el frame*/
struct Segmento *asignarUltimaPaginaSegmento(struct Segmento *segmento, int tamanioUltimaMetadata) {

	struct Pagina *ultimaPagina = malloc(sizeof(struct Pagina));
	ultimaPagina->numeroFrame = asignarUnFrame();
	void *pos = retornarPosicionMemoriaFrame(ultimaPagina->numeroFrame) - tamanioUltimaMetadata - sizeof(struct HeapMetadata);

	struct HeapMetadata *ultimaMetadata = malloc(sizeof(struct HeapMetadata));
	ultimaMetadata->isFree = true;
	ultimaMetadata->size = tamanioUltimaMetadata;

	//Pone la metadata en el frame correspondiente
	memcpy(pos, ultimaMetadata, sizeof(struct HeapMetadata));

	t_list *paginas = segmento->tablaPaginas;
	list_add(paginas, ultimaPagina);

	segmento->tablaPaginas = paginas;

	return segmento;
}

/*Le asigna una pagina intermedia a un segmento, no hay metadata inicial ni final*/
struct Segmento *asignarNuevaPagina(struct Segmento *segmento, int tamanio) {

	struct Pagina *nuevaPagina = malloc(sizeof(struct Pagina));
	nuevaPagina->numeroFrame = asignarUnFrame();
	nuevaPagina->presencia = 1;

	//Ocupo frame y lo reemplazo - modifico - en el bitmap de frames
	struct Frame *frame = malloc(sizeof(struct Frame));
	frame = list_get(bitmapFrames, nuevaPagina->numeroFrame);
	frame->modificado = 0;
	frame->uso = 1;
	frame->listaMetadata = list_create();
	list_replace(bitmapFrames, nuevaPagina->numeroFrame, frame);

	t_list *paginas = segmento->tablaPaginas;
	list_add(paginas, nuevaPagina);

	segmento->tablaPaginas = paginas;

	return segmento;

}

/* Recorre el espacio del segmento buscando espacio libre. Debe encontrar un header
 * que isFree = true y size >= tamanio (si se busca espacio tambien para metadata, pasar
 * toda la suma en el parametro)*/
bool poseeTamanioLibreSegmento(struct Segmento *segmento, uint32_t tamanio) {

	t_list *paginas = segmento->tablaPaginas;
	struct Pagina *pagina = malloc(sizeof(struct Pagina));
	struct Frame *frame = malloc(sizeof(struct Frame));
	t_list *metadatas = list_create();
	void *pos;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));

	for (int i = 0; i < list_size(paginas); i++) {

		pagina = list_get(paginas, i);
		frame = list_get(bitmapFrames, pagina->numeroFrame);
		pos = retornarPosicionMemoriaFrame(pagina->numeroFrame);
		int desplazamiento;
		metadatas = frame->listaMetadata;

		for(int k = 0; k < list_size(metadatas); k++) {

			desplazamiento = (int)list_get(metadatas, k);
			pos = pos + desplazamiento; //me posiciono donde esta la metadata a leer
			memcpy(metadata, pos, sizeof(struct HeapMetadata));
			int desplazamiento;

			for (int k = 0; k < list_size(metadatas); k++) {

				desplazamiento = (int)list_get(metadatas, k);
				pos = pos + desplazamiento; //me posiciono donde esta la metadata a leer
				memcpy(metadata, pos, sizeof(struct HeapMetadata));

				if (metadata->isFree == true && metadata->size <= tamanio) {
					return true;
				}

			}

		}

	}
	//free pagina, frame, metadata

	return false; //si sale sin retornar true, no encontro metadata util

}

struct Segmento *asignarTamanioLibreASegmento(struct Segmento *segmento, uint32_t tamanio) {

	t_list *paginas = segmento->tablaPaginas;
	struct Pagina *pagina = malloc(sizeof(struct Pagina));
	struct Frame *frame = malloc(sizeof(struct Frame));
	t_list *metadatas = list_create();
	void *pos;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	struct HeapMetadata *nuevaMetadata = malloc(sizeof(struct HeapMetadata));
	int espacioLibreEnPagina;
	int bytesAMoverme = -1;

	for (int i = 0; i < list_size(paginas); i++) { //Recorro las paginas buscando metadata libre

		pagina = list_get(paginas, i);
		frame = list_get(bitmapFrames, pagina->numeroFrame);
		pos = retornarPosicionMemoriaFrame(pagina->numeroFrame);
		metadatas = frame->listaMetadata;

		if (bytesAMoverme != -1) {

			if (bytesAMoverme < tam_pagina) {
				pos = pos + bytesAMoverme;

				int menorMetadata = (int)list_get(metadatas, 0); //estando siempre ordenadas

				//Agregar metadata a la lista del frame y modificar frame
				nuevaMetadata->isFree = true;
				nuevaMetadata->size = menorMetadata - bytesAMoverme
						- sizeof(struct HeapMetadata);
				memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));

			}

		}

		int desplazamiento;

		for(int k = 0; k < list_size(metadatas); k++) {

			desplazamiento = (int)list_get(metadatas, k);
			pos = pos + desplazamiento; //me posiciono donde esta la metadata a leer
			memcpy(metadata, pos, sizeof(struct HeapMetadata)); //Leo la metadata

			if(metadata->isFree == true && metadata->size <= tamanio){

				//Veo si me entra todo en el espacio libre de la pagina o si debo moverme entre paginas
				espacioLibreEnPagina = tam_pagina - desplazamiento /*desplazamiento hasta la metadata*/ - sizeof(struct HeapMetadata);

				if(espacioLibreEnPagina >= tamanio){ //me entra en esta pagina, lo ubico
					metadata->isFree = false;
					metadata->size = metadata->size - tamanio;

					//Creo la nueva metadata (siguiente metadata) en el frame
					nuevaMetadata->isFree = true;
					nuevaMetadata->size = tam_pagina - desplazamiento - metadata->size - (2*sizeof(struct HeapMetadata));
					//AGREGAR METADATA al frame y modificar estructuras

					pos = pos + sizeof(struct HeapMetadata) + metadata->size;
					memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));

					if (metadata->isFree == true && metadata->size <= tamanio) {
						//Veo si me entra t odo en el espacio libre de la pagina o si debo moverme entre paginas
						espacioLibreEnPagina = tam_pagina - desplazamiento /*desplazamiento hasta la metadata*/ - sizeof(struct HeapMetadata);

						if (espacioLibreEnPagina >= tamanio) { //me entra en esta pagina, lo ubico
							metadata->isFree = false;
							metadata->size = metadata->size - tamanio;

							//Creo la nueva metadata (siguiente metadata) en el frame
							nuevaMetadata->isFree = true;
							nuevaMetadata->size = tam_pagina - desplazamiento - metadata->size - (2 * sizeof(struct HeapMetadata));
							//AGREGAR METADATA al frame y modificar estructuras

							pos = pos + sizeof(struct HeapMetadata) + metadata->size;
							memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));

						} else { //si no me entra en esta pagina, me muevo entre paginas hasta donde deba ubicar (primero ocupo la metadata)
							bytesAMoverme = tamanio - sizeof(struct HeapMetadata);

							//No me entra en esta pagina, me muevo a las siguientes, descontando lo que me movi en esta pagina
							bytesAMoverme = bytesAMoverme - espacioLibreEnPagina;

						}

					}
				}
			}
		}
	}
	//free pagina, frame, metadata

	return NULL; //nunca llegaria aca, porque siempre se llama a esta funcion chequeando antes que posea tamaño libre

}

/*Un segmento es extendible si no tiene otro segmento a continuacion y si es un segmento comun*/
bool esExtendible(t_list *segmentosProceso, int unIndice) {
	//Momentaneamente, es extendible si es el ultimo de la lista de segmentos
	int indiceUltimoSegmento = list_size(segmentosProceso) - 1;

	struct Segmento *ultimoSegmento = malloc(sizeof(struct Segmento));
	ultimoSegmento = list_get(segmentosProceso, indiceUltimoSegmento);

	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	segmento = list_get(segmentosProceso, unIndice);

	if (ultimoSegmento->id == segmento->id && segmento->esComun == true) {
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

/*Funcion recorre buscando heapmetadatas libre - mayor o igual - a cierto size*/
void *buscarEspacioLibreProceso(int idSocketCliente, uint32_t tamanio) {
	/* Siempre tengo que buscar espacio libre para el tamaño que necesito
	 * mas el tamaño que ocupa la nueva metadata (5 bytes)*/
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);
	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	struct Pagina *pagina = malloc(sizeof(struct Pagina));
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	void *pos;
	int numeroPagina;

	int segmentosARecorrer = list_size(segmentosProceso);

	for (int i = 0; i < segmentosARecorrer; i++) { //Recorro todos los segmentos del proceso

		segmento = list_get(segmentosProceso, i);

		t_list *paginas = segmento->tablaPaginas;
		int paginasARecorrer = list_size(paginas);

		int frame = pagina->numeroFrame;
		pos = retornarPosicionMemoriaFrame(frame);
		numeroPagina = 0;

		//Antes de entrar a este while, estoy en la pagina 0, se que tengo metadata que leer
		memcpy(metadata, pos, sizeof(struct HeapMetadata));

		while (numeroPagina <= paginasARecorrer) {
			//int sizeARecorrer = metadata->size;

			if (metadata->isFree == true) {

				if (metadata->size >= (tamanio + sizeof(struct HeapMetadata))) { //Si la metadata libre alcanza, la retorno

					return (pos + sizeof(struct HeapMetadata)); //Retorno la posicion donde comienza la data libre

				} else { //Si la metadata libre no alcanza, me muevo al proximo header

					if (metadata->size
							< (tam_pagina - sizeof(struct HeapMetadata))) { //Si el proximo header esta dentro de la misma pag

						pos = pos + sizeof(struct HeapMetadata)
								+ metadata->size;

						struct HeapMetadata *otraMetadata = malloc(
								sizeof(struct HeapMetadata));
						memcpy(otraMetadata, pos, sizeof(struct HeapMetadata));

						if (otraMetadata->isFree == true) { //Y si ese proximo header esta libre

							if (metadata->size
									>= (tamanio + sizeof(struct HeapMetadata))) { //Si la metadata libre alcanza, la retorno

								return (pos + sizeof(struct HeapMetadata)); //Retorno la posicion donde comienza la data libre

							}

						}

					} else { //Si el proximo header esta en otra pagina de las siguientes

						int tamanioAMoverme = metadata->size;
						int paginasQueContienenSizeMetadata = (ceil)(
								tamanioAMoverme / tam_pagina);

						//Me muevo al proximo frame (consumo una pagina entera)
						if (paginasQueContienenSizeMetadata > 1) {
							tamanioAMoverme =
									tamanioAMoverme
											- (tam_pagina
													- sizeof(struct HeapMetadata));
							paginasQueContienenSizeMetadata--;

							struct Pagina *proximaPagina = malloc(
									sizeof(struct Pagina));
							numeroPagina++;
							proximaPagina = list_get(paginas, numeroPagina);
							pos = retornarPosicionMemoriaFrame(
									proximaPagina->numeroFrame);
						}

						for (int k = paginasQueContienenSizeMetadata; k > 0;
								k--) {

							if (tamanioAMoverme < tam_pagina) { //significa que ya es la ultima pagina

								pos = pos + tamanioAMoverme;
								//leer heapmetadata siguiente
								struct HeapMetadata *metadataFinal = malloc(
										sizeof(struct HeapMetadata));
								memcpy(metadataFinal, pos,
										sizeof(struct HeapMetadata));

								if (metadataFinal->isFree == true) {

									if (metadataFinal->size
											>= (tamanio
													+ sizeof(struct HeapMetadata))) {

										return (pos
												+ sizeof(struct HeapMetadata));

									}

								}

							} else { //Me muevo de pagina

								tamanioAMoverme = tamanioAMoverme - tam_pagina;
								struct Pagina *proximaPagina = malloc(
										sizeof(struct Pagina));
								numeroPagina++;
								proximaPagina = list_get(paginas, numeroPagina);
								pos = retornarPosicionMemoriaFrame(
										proximaPagina->numeroFrame);

							}

						}

					}

				}

			}

		}

	}

	//Si recorre todos los segmentos, todas sus paginas y no encuentra espacio libre, retorna NULL
	return NULL;
}

/*Asigna cierto tamanio a una posicion de memoria, como la posicion es la retornada en buscar espacio
 *libre, se sabe que el tamanio de la posicion alcanza (y quizas sobra) para lo que se tiene*/
void asignarTamanioADireccion(uint32_t tamanio, void* src, int idSocketCliente) { //Cambiar id, se agarra de utils
	//Averiguo en que frame - pagina - segmento estoy, porque quizas necesito moverme entre pags

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
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*) src);
	unSegmento = list_get(listaSegmentos, idSegmento);

	//Obtencion pagina y frame
	struct Pagina *unaPagina; //SAME CON EL MALLOC
	unaPagina = paginaQueContieneDireccion(unSegmento, (void*) src); //Me retorna directamente la pagina
	frame = unaPagina->numeroFrame;

	//Me paro en donde esta la metadata para averiguar el size que tengo (src - 5)
	void *pos = retornarPosicionMemoriaFrame(frame) + desplazamiento - 5;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	metadata = (void *) pos;
	struct HeapMetadata *nuevaMetadata = malloc(sizeof(struct HeapMetadata));

	//Size disponible en el frame actual
	int sizePrimerFrame = tam_pagina
			- ((int) src - (int) retornarPosicionMemoriaFrame(frame));

	//Size que tengo para guardar tamanio y proximo heapmetadata
	int sizeDisponible = metadata->size;

	//Averiguo cantidad de paginas por si el size esta distribuido en mas de una pagina
	t_list *paginas = unSegmento->tablaPaginas;
	int paginasARecorrer = list_size(paginas);

	int numeroPagina = obtenerIndicePagina(unSegmento->tablaPaginas, unaPagina);

	while (numeroPagina <= paginasARecorrer) {

		/*RECORRO PAGINAS Y ASIGNO TAMAÑO*/
		if (sizePrimerFrame >= tamanio + sizeof(struct HeapMetadata)) { //Si el tamaño disponible en el primer frame me alcanza, lo asigno
			src = src + tamanio;
			nuevaMetadata->isFree = true;
			nuevaMetadata->size = sizePrimerFrame - tamanio - sizeof(struct HeapMetadata);

			src = (struct HeapMetadata *) nuevaMetadata;
		} else { //Si el tamaño del primer frame no me alcanza, me muevo hasta donde deba poner el proximo heapmetadata

			int tamanioAMoverme = tamanio - sizePrimerFrame; //este espacio disponible ya lo uso
			int paginasNecesariasParaTamanio = (ceil)(
					tamanioAMoverme / tam_pagina);
			int paginasRecorridas = 0;

			while (paginasRecorridas < paginasNecesariasParaTamanio) {

				/*ME MUEVO TODAS LAS PAGINAS NECESARIAS HASTA LLEGAR AL DESPLAZAMIENTO QUE SEA
				 *MENOR A TAM PAGINA, ME MUEVO ESE DESPLAZAMIENTO Y PONGO EL HEAPMETADATA NUEVO*/

			}

		}

	}

}

//FALTA REVISAR que se esten guardando todos los cambios en el segmento antes de retornar
/*Unificar headers con nueva implementacion, me retorna el segmento modificado*/
struct Segmento *unificarHeaders2(int idSegmento, int idSocketCliente) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentos = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	//Segmento a unificar
	struct Segmento *segmento = malloc(sizeof(struct Segmento));
	segmento = list_get(segmentos, idSegmento);

	//Obtengo las paginas a recorrer
	t_list *paginas = segmento->tablaPaginas;
	struct Pagina *pagina = malloc(sizeof(struct Pagina));
	struct Frame *frame = malloc(sizeof(struct Frame));

	//Metadatas
	t_list *metadatas = list_create();
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	struct HeapMetadata *siguienteMetadata = malloc(sizeof(struct HeapMetadata));
	metadata = NULL;
	int nuevoSize;
	int desplazamiento = -1;

	//Recorro solo las metadatas que esten en mm ppal?
	for(int i = 0; i < list_size(paginas); i++){ //Recorro las paginas y me guardo todas las metadatas
		pagina = list_get(paginas, i);
		frame = list_get(bitmapFrames, pagina->numeroFrame);
		metadatas = frame->listaMetadata;

		for(int j = 0; j < list_size(metadatas); j++){
			memcpy(siguienteMetadata, retornarPosicionMemoriaFrame(pagina->numeroFrame) + (int)list_get(metadatas, j), sizeof(struct HeapMetadata));

			if(metadata != NULL && (metadata->isFree == true && siguienteMetadata->isFree == true)) {
				//unifico en metadata
				nuevoSize = metadata->size + siguienteMetadata->size;
				metadata->size = nuevoSize;

				list_remove(frame->listaMetadata, j); //elimina la metadata unificada

				//Modifico la metadata en el frame en si
				memcpy(retornarPosicionMemoriaFrame(pagina->numeroFrame) + desplazamiento, metadata, sizeof(struct HeapMetadata));

			} else {

				metadata = siguienteMetadata;

			}

			desplazamiento = (int)list_get(metadatas, j);
		}



	}
	free(stringIdSocketCliente);
	return segmento;
}

/**Funcion para unificar dos headers - se la llama despues de un free. Unifica desde el header 1
 * recorriendo todos los headers DEL SEGMENTO. Le envio como parametro el id del segmento que
 * tiene que recorrer*/
void unificarHeaders(int idSocketCliente, int idSegmento) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

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
	memcpy(metadata, posInicio, sizeof(struct HeapMetadata));
	while (numeroPagina <= paginasARecorrer) {
		//int sizeARecorrer = metadata->size;

		if (metadata->isFree == true) {

			if (metadata->size < (tam_pagina - sizeof(struct HeapMetadata))) { //Si el proximo header esta dentro de la misma pag
				pos = pos + sizeof(struct HeapMetadata) + metadata->size;

				struct HeapMetadata *otraMetadata = malloc(
						sizeof(struct HeapMetadata));
				memcpy(otraMetadata, pos, sizeof(struct HeapMetadata));

				if (otraMetadata->isFree == true) { //Y si ese proximo header esta libre
					metadata->size = metadata->size + otraMetadata->size; //Los unifico
					//Se esta borrando la metadata vieja? VER CON NACHIN
				}
			} else { //Si el proximo header esta en otra pagina de las siguientes
				int tamanioAMoverme = metadata->size;
				int paginasQueContienenSizeMetadata = (ceil)(
						tamanioAMoverme / tam_pagina);

				//Me muevo al proximo frame (consumo una pagina entera)
				if (paginasQueContienenSizeMetadata > 1) {
					tamanioAMoverme = tamanioAMoverme
							- (tam_pagina - sizeof(struct HeapMetadata));
					paginasQueContienenSizeMetadata--;

					struct Pagina *proximaPagina = malloc(
							sizeof(struct Pagina));
					numeroPagina++;
					proximaPagina = list_get(paginasSegmento, numeroPagina);
					pos = retornarPosicionMemoriaFrame(
							proximaPagina->numeroFrame);
				}

				for (int i = paginasQueContienenSizeMetadata; i > 0; i--) {

					if (tamanioAMoverme < tam_pagina) { //significa que ya es la ultima pagina

						pos = pos + tamanioAMoverme;
						//leer heapmetadata siguiente
						struct HeapMetadata *metadataFinal = malloc(
								sizeof(struct HeapMetadata));
						memcpy(metadataFinal, pos, sizeof(struct HeapMetadata));

						if (metadataFinal->isFree == true) {
							//Agrupo en la metadata original
							metadata->size = metadata->size
									+ metadataFinal->size;
							//borrar el metadata final ?????????????
						}

					} else { //Me muevo de pagina
						tamanioAMoverme = tamanioAMoverme - tam_pagina;
						struct Pagina *proximaPagina = malloc(
								sizeof(struct Pagina));
						numeroPagina++;
						proximaPagina = list_get(paginasSegmento, numeroPagina);
						pos = retornarPosicionMemoriaFrame(
								proximaPagina->numeroFrame);
					}

				}

			}

		}

	}

	free(stringIdSocketCliente);
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
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);
	struct Segmento *unSegmento = malloc(sizeof(struct HeapMetadata));

	unSegmento = list_get(listaSegmentosProceso, idSegmento);

	free(stringIdSocketCliente);
	return unSegmento->tamanio;
}

/*Funcion que me retorna el espacio ocupado por las paginas de un segmento de un
 *proceso determinado*/
int espacioPaginas(int idSocketCliente, int idSegmento) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);
	struct Segmento *unSegmento = list_get(segmentosProceso, idSegmento); //list find segmento

	free(stringIdSocketCliente);
	return (list_size(unSegmento->tablaPaginas) * tam_pagina);
}

/**
 * Find minimum between two numbers.
 */
int min(int num1, int num2) {
	return (num1 > num2) ? num2 : num1;
}

//MUSE GET

/**
 * Copia una cantidad `n` de bytes desde una posición de memoria de MUSE a una `dst` local.
 * @param dst Posición de memoria local con tamaño suficiente para almacenar `n` bytes.
 * @param src Posición de memoria de MUSE de donde leer los `n` bytes.
 * @param n Cantidad de bytes a copiar.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */

void *museget(void* dst, uint32_t src, size_t n, int idSocketCliente) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentos = dictionary_get(tablasSegmentos, stringIdSocketCliente);
	void *resultado = NULL;

	if(listaSegmentos == NULL){
		//Hacer log
		return NULL;
	}

	int idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*)src);;
	struct Segmento *segmento = list_get(listaSegmentos, idSegmento);

	if(segmento == NULL){
		//Hacer log
		return NULL;
	}

	int direccionSeg = src - segmento->baseLogica;
	int primeraPagina = direccionSeg / tam_pagina;
	int ultimaPagina = (int)ceil((double)(direccionSeg + n) / tam_pagina) - 1;
	int desplazamiento = direccionSeg % tam_pagina;

	if(segmento->tamanio - direccionSeg >= n && primeraPagina * tam_pagina + desplazamiento + n <= segmento->tamanio){
		int cantidadPaginas = ultimaPagina - primeraPagina + 1;
		resultado = malloc(n);
		void *recorridoPaginas = malloc(cantidadPaginas * tam_pagina);
		int puntero = 0;

		if(segmento->esComun == false){

			//traer paginas necesarias a mm

		}

		for(int i = 0; i < cantidadPaginas; i++, puntero += tam_pagina){
			struct Pagina *pagina = list_get(segmento->tablaPaginas, primeraPagina + i);
			struct Frame *frame = list_get(bitmapFrames, pagina->numeroFrame);
			pagina->presencia = 1;

			void *pos = obtenerPosicionMemoriaPagina(pagina);

			frame->uso = 1;

			list_replace(bitmapFrames, pagina->numeroFrame, frame);
			list_replace(segmento->tablaPaginas, primeraPagina + i, pagina);

			memcpy(recorridoPaginas + puntero, pos, tam_pagina);
		}

		memcpy(resultado, recorridoPaginas + desplazamiento, n);
		free(recorridoPaginas);
	}

	return resultado;
}

/*Retorna la posicion de memoria del frame en caso de estar en mm ppal y en caso de no estar en mm ppal, la trae de swap*/
void *obtenerPosicionMemoriaPagina(struct Pagina *pagina){

	if(pagina->presencia == 0){

		if(pagina->indiceSwap != -1){

			void* paginaReemplazo = malloc(tam_pagina);
			memcpy(paginaReemplazo, swap + pagina->indiceSwap * tam_pagina, tam_pagina);

			bitarray_clean_bit(bitmapSwap, pagina->indiceSwap); //libero el indice en swap
			pagina->indiceSwap = -1;

			int frame = buscarFrameLibre();
			pagina->numeroFrame = frame;
			pagina->presencia = 1;

			//Paso la pagina a memoria
			memcpy(retornarPosicionMemoriaFrame(frame), paginaReemplazo, tam_pagina);

		} else{

			//No deberia llegar aca (?)

		}
	}

	struct Frame *frame = list_get(bitmapFrames, pagina->numeroFrame);
	frame->uso = 1;

	list_replace(bitmapFrames, pagina->numeroFrame, frame);

	return retornarPosicionMemoriaFrame(pagina->numeroFrame);
}

int idSegmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion) {
	int segmentosARecorrer = list_size(listaSegmentos);
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));

	for (int i = 0; i < segmentosARecorrer; i++) {
		unSegmento = list_get(listaSegmentos, i);

		if (((int)direccion - unSegmento->baseLogica) < unSegmento->tamanio) {
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

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,stringIdSocketCliente);

	//Obtencion segmento, pagina, frame, desplazamiento
	int idSegmento;
	int frame;
	int desplazamiento;

	int direccion = (int)dst;

	//Obtencion desplazamiento
	desplazamiento = direccion % tam_pagina;

	//Obtencion idSegmento y segmento
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*) dst);
	unSegmento = list_get(listaSegmentos, idSegmento);

	//Obtencion pagina y frame
	unaPagina = paginaQueContieneDireccion(unSegmento, (void*) dst); //Me retorna directamente la pagina
	frame = unaPagina->numeroFrame;
	//////////////////////////////////////////////////

	/*Ya se tienen todos los datos necesarios y se puede ir a mm ppal a buscar la data*/

	//PRIMERO se debe chequear que no haya un segmentation fault
	//para esto, leo heapmetadata de la posicion
	void *pos;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	pos = retornarPosicionMemoriaFrame(frame) + desplazamiento - 5;
	pos = (struct HeapMetadata *) metadata; //REVISAR ESTE CASTEO

	if (metadata->size < n) {
		//SEGMENTATION FAULT
		//Informar a libmuse para que lo avise
		//muse close?

		return -1;

	} else {

		pos = retornarPosicionMemoriaFrame(frame) + desplazamiento;
		memcpy(pos, src, n);

		return 0;

	}

}

//MUSE FREE

int musefree(int idSocketCliente, uint32_t dir) {

	//busco los segmentos del proceso que me pide el free
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));

	struct Pagina *unaPagina = malloc(sizeof(struct Pagina));

	//busco el segmento especifico que contiene la direccion
	unSegmento = segmentoQueContieneDireccion(segmentosProceso, (void*) dir);

	//voy a la pagina de ese segmento donde esta la direccion
	unaPagina = paginaQueContieneDireccion(unSegmento, (void*) dir);

	int desplazamiento = (int) dir % tam_pagina;

	//retorno la posicion memoria dentro del frame
	void *pos = retornarPosicionMemoriaFrame(unaPagina->numeroFrame)
			+ desplazamiento;

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	memcpy(metadata, pos, sizeof(struct HeapMetadata));

	//almaceno cuanta memoria esta liberando - representa cuanto tendra que achicarse el segmento
	int tamanio_liberado = metadata->size;

	//se cambia la metadata para indicar que esta parte esta libre
	metadata->isFree = true;

	//modifico el tamanio del segmento en el que se hizo free
	unSegmento->tamanio = unSegmento->tamanio - tamanio_liberado; //Consulta, tamanio liberado siempre multiplo de tam pag?

	//ACTUALIZAR tabla segmentos de idproceso con el nuevo segmento modificado (unSegmento)

	//una vez que realizo el free tengo que verifico que si la porcion anterior a la direccion o la siguiente se encuentran libres
	//si lo estan unifico
	unificarHeaders(idSocketCliente, unSegmento->id);

	return 1;
}

struct Segmento *segmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion) {

	struct Segmento *unSegmento;

	for (int i = 0; i < list_size(listaSegmentos); i++) {
		unSegmento = list_get(listaSegmentos, i);

		if (((int) direccion - unSegmento->baseLogica) < unSegmento->tamanio) {
			return unSegmento;
		}
	}

	//Si sale del for sin retorno, no hay ningun segmento que contenga esa direc
	return NULL; //error
}

//MEMORIA VIRTUAL

//Algoritmo clock modificado

/*Retorna un entero que es el marco elegido para el reemplazo*/

/*Funcionamiento:
 * PASO 1 - Empezando desde la posición actual del puntero, recorrer la lista de marcos.
 * Durante el recorrido, dejar el bit de uso (U) intacto. El primer marco que se
 * encuentre con U = 0 y M = 0 se elige para el reemplazo.
 * PASO 2 - Si el paso 1 falla, recorrer nuevamente, buscando un marco con U = 0 y M = 1.
 * El primer marco que cumpla la condición es seleccionado para el reemplazo. Durante
 * este recorrido, cambiar el bit de uso a 0 de todos los marcos que no se elijan.
 * PASO 3 - Si el paso 2 falla, volver al paso 1.
 * */

int clockModificado() {
	struct Frame *frame = malloc(sizeof(struct Frame));
	int framesRecorridos = 0;

	//Paso 1
	while (framesRecorridos < cantidadFrames) {
		frame = list_get(bitmapFrames, punteroClock);

		if (frame->uso == 0 && frame->modificado == 0) {

			incrementarPunteroClockModificado();
			return punteroClock;

		}

		framesRecorridos++;
		incrementarPunteroClockModificado();

	}

	framesRecorridos = 0;

	//Paso 2
	while (framesRecorridos < cantidadFrames) {
		frame = list_get(bitmapFrames, punteroClock);

		if (frame->uso == 0 && frame->modificado == 1) {

			incrementarPunteroClockModificado();
			return punteroClock;

		}

		//Si no se lo elige, se le pone u = 0 (se lo libera)
		liberarFrame(punteroClock);

		framesRecorridos++;
		incrementarPunteroClockModificado();

	}

	//Paso 3
	while (framesRecorridos < cantidadFrames) {
		frame = list_get(bitmapFrames, punteroClock);

		if (frame->uso == 0 && frame->modificado == 0) {

			incrementarPunteroClockModificado();
			return punteroClock;

		}

		framesRecorridos++;
		incrementarPunteroClockModificado();

	}

	return -1; //Nunca deberia llegar aca. Consultar.
}

//Inicializa cada bit del bitmap de swap en 0
void inicializarBitmapSwap() {

	for (int i = 0; i < bitarray_get_max_bit(bitmapSwap); i++) {
		bitarray_clean_bit(bitmapSwap, i);
	}

}

void incrementarPunteroClockModificado() {

	if (punteroClock < (cantidadFrames - 1)) {

		punteroClock++;

	} else {

		punteroClock = 0;

	}

}

//MUSE MAP

/**Devuelve un puntero a una posición mappeada de páginas por una cantidad `length` de bytes
 * el archivo del `path` dado.
 * @param path Path a un archivo en el FileSystem de MUSE a mappear.
 * @param length Cantidad de bytes de memoria a usar para mappear el archivo.
 * @param flags
 *          MAP_PRIVATE     Solo un proceso/hilo puede mappear el archivo.
 *          MAP_SHARED      El segmento asociado al archivo es compartido.
 * @return Retorna la posición de memoria de MUSE mappeada.
 * @note: Si `length` sobrepasa el tamaño del archivo, toda extensión deberá estar llena de "\0".
 * @note: muse_free no libera la memoria mappeada. @see muse_unmap
 */
uint32_t musemap(char *path, size_t length, int flags, int idSocketCliente) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	struct Segmento *segmentoMappeado = malloc(sizeof(struct Segmento));

	segmentoMappeado->esComun = false; //Indica que es un segmento mmappeado
	segmentoMappeado->filePath = path;
	segmentoMappeado->id = list_size(segmentosProceso) + 1;

	//Obtengo el tamaño del ultimo segmento
	int idSegmento = list_size(segmentosProceso) - 1; //Id ultimo segmento
	segmentoMappeado->baseLogica = obtenerTamanioSegmento(idSegmento,
			idSocketCliente) + 1;

	segmentoMappeado->tablaPaginas = list_create();

	//No se esta utilizando actualmente, pero necesito guardarlo para sync? - ver
	FILE *archivoMap = fopen(path, "r");

	int paginasNecesarias = ceil(length / tam_pagina);

	for (int i = paginasNecesarias; i < paginasNecesarias; i--) {
		struct Pagina *unaPagina = malloc(sizeof(struct Pagina));

		//frame se le asigna cuando se referencia
		unaPagina->numeroFrame = -1;
		list_add(segmentoMappeado->tablaPaginas, unaPagina);

	}

	//Agrego el nuevo segmentoMap a la lista de segmentos del proceso
	list_add(segmentosProceso, segmentoMappeado);
	//Modifico el diccionario agregando el nuevo segmento
	dictionary_put(tablasSegmentos, stringIdSocketCliente, segmentosProceso);

	return 0;
}

/*Trae a memoria principal una pagina de un segmento de un proceso en particular
 *y retorna el numero de frame en el que la ubico*/
int traerAMemoriaPrincipal(int indicePagina, int indiceSegmento, int idSocketCliente) {
	//Obtengo pagina swapeada
	struct Pagina *paginaSwapeada = malloc(sizeof(struct Pagina));
	struct Segmento *segmentoQueContienePagina = malloc(sizeof(struct Segmento));

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	char *datosEnSwap = malloc(sizeof(struct Pagina));

	segmentoQueContienePagina = list_get(segmentosProceso, indiceSegmento);
	paginaSwapeada = list_get(segmentoQueContienePagina->tablaPaginas, indicePagina);

	//Si se llamo a esta funcion, antes se chequeo que el frame tiene P = 0
	//por lo que la pagina esta en swap

	//Obtengo indice de swap donde se encuentra
	int indiceSwap = paginaSwapeada->indiceSwap;

	swap = fopen("swap.txt", "a+");
	fseek(swap, indiceSwap * tam_pagina, SEEK_SET);
	fread(datosEnSwap, sizeof(char), tam_pagina, swap);

	//Busco frame donde traer la pagina - ejecuta el algoritmo de reemplazo en caso de
	//ser necesario
	int frameReemplazo;

	if (hayFramesLibres() == true) {
		frameReemplazo = buscarFrameLibre();
	} else {
		frameReemplazo = clockModificado();
		//llevarASwap(frameReemplazo);
	}

	struct Frame *nuevoFrame = malloc(sizeof(struct Frame));

	paginaSwapeada->indiceSwap = -1; //ya no esta en swap
	paginaSwapeada->numeroFrame = frameReemplazo;
	paginaSwapeada->presencia = 1;

	//"Libero" la posicion de swap en el bitarray de swap
	bitarray_clean_bit(bitmapSwap, indiceSwap);

	nuevoFrame = list_get(bitmapFrames, frameReemplazo);
	nuevoFrame->modificado = 0; //No esta modificado, recien se carga
	nuevoFrame->uso = 1;

	//Cargo la data swappeada en el frame de mm ppal asignado a la pagina
	cargarDatosEnFrame(frameReemplazo, datosEnSwap);

	//Modifico las estructuras
	list_replace(bitmapFrames, frameReemplazo, nuevoFrame);
	list_replace(segmentoQueContienePagina->tablaPaginas, indicePagina,
			paginaSwapeada);
	list_replace(segmentosProceso, indiceSegmento, segmentoQueContienePagina);
	dictionary_put(tablasSegmentos, stringIdSocketCliente, segmentosProceso);

	return frameReemplazo;
}

void cargarDatosEnFrame(int indiceFrame, char *datos) {

	void *pos = retornarPosicionMemoriaFrame(indiceFrame);

	//Se me esta copiando la data sin formato? - Consultar
	//pos = datos;
	memcpy(pos, datos, tam_pagina);

}

/*Lleva a swap una pagina en particular de un segmento de un proceso
 *liberando el frame donde se encontraba y retornando el indice de swap
 *donde la ubico*/
int llevarASwapUnaPagina(int indicePagina, int indiceSegmento, int idSocketCliente) {
	struct Pagina *paginaASwappear = malloc(sizeof(struct Pagina));
	struct Segmento *segmentoQueContienePagina = malloc(sizeof(struct Segmento));

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	char *datosASwappear = malloc(sizeof(struct Pagina));

	segmentoQueContienePagina = list_get(segmentosProceso, indiceSegmento);
	paginaASwappear = list_get(segmentoQueContienePagina->tablaPaginas, indicePagina);

	//Obtengo la data a swappear que se encuentra en el frame asignado a la pagina
	int frameQueContieneData = paginaASwappear->numeroFrame;
	void *pos = retornarPosicionMemoriaFrame(frameQueContieneData);

	//Guardo en la variable los datos a swappear
	memcpy(datosASwappear, pos, tam_pagina);

	//Obtengo un lugar libre en swap y escribo los datos ahi
	int indiceSwap = buscarIndiceSwapLibre();

	swap = fopen("swap.txt", "a+");
	fseek(swap, indiceSwap * tam_pagina, SEEK_SET);
	//CHEQUEAR que este escribiendo en el lugar correcto
	fwrite(datosASwappear, sizeof(char), tam_pagina, swap);

	//Debo liberar el frame que contenia la pagina que lleve a swap y tambien la pagina
	paginaASwappear->indiceSwap = indiceSwap;
	paginaASwappear->numeroFrame = -1;
	paginaASwappear->presencia = 0;

	struct Frame *frameModificado = malloc(sizeof(struct Frame));
	frameModificado = list_get(bitmapFrames, frameQueContieneData);
	frameModificado->modificado = 0;
	frameModificado->uso = 0;

	//Modifico todas las estructuras con lo nuevo
	list_replace(bitmapFrames, frameQueContieneData, frameModificado);
	list_replace(segmentoQueContienePagina->tablaPaginas, indicePagina,
			paginaASwappear);
	list_replace(segmentosProceso, indiceSegmento, segmentoQueContienePagina);
	dictionary_put(tablasSegmentos, stringIdSocketCliente, segmentosProceso);

	return indiceSwap;
}

int buscarIndiceSwapLibre() {

	for (int i = 0; i < bitarray_get_max_bit(bitmapSwap); i++) {

		if (bitarray_test_bit(bitmapSwap, i) == 0) {
			return i;
		}
	}

	//Nunca se me acaba swap?
	return -1;
}

//MUSE SYNC

/**Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
 * @param addr Dirección a memoria mappeada.
 * @param len Cantidad de bytes a escribir.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 * @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
 */
int musesync(uint32_t addr, size_t len, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentos = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	//Obtencion segmento, pagina, frame, desplazamiento
	int idSegmento;
	int frame;
	int desplazamiento;

	//Obtencion desplazamiento
	desplazamiento = addr % tam_pagina;

	//Obtencion idSegmento y segmento
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*) addr);
	unSegmento = list_get(listaSegmentos, idSegmento);

	//Obtencion PRIMERA pagina y frame
	struct Pagina *unaPagina = malloc(sizeof(struct Pagina));
	unaPagina = paginaQueContieneDireccion(unSegmento, (void*) addr); //Me retorna directamente la pagina
	frame = unaPagina->numeroFrame;
	int indicePrimeraPagina = obtenerIndicePagina(unSegmento->tablaPaginas,
			unaPagina);

	void *datosAActualizar = malloc(sizeof(len));

	//FIJARME SI ES UN SEGMENTO MAP

	if (unSegmento->esComun == true) {

		return -1; //Error, no se puede realizar un sync

	} else {

		//Obtengo el archivo donde actualizar
		char *path = unSegmento->filePath; //Si llegue aca, ya se que es un segmento mmap y que tiene un path (no es null)

		//Obtengo los datos a actualizar (REVISAR que esten llegando bien los datos cuando se testee)
		datosAActualizar = obtenerDatosActualizados(frame, desplazamiento, len,
				unSegmento, unaPagina);
		//Si len es menor debo llenar con /0/0/0/0/0

		//Abro el archivo y lo actualizo (CONSULTAR posicion comienzo modificacion)
		FILE *archivoMap = fopen(path, "a+");
		//posicion a escribir en el archivo: indice primera pagina + desplazamiento
		fseek(archivoMap, indicePrimeraPagina * tam_pagina + desplazamiento,
				SEEK_SET);
		int resultadoEscritura = fwrite(datosAActualizar, sizeof(char), len,
				archivoMap);

		if (resultadoEscritura != EOF) {

			return 0;

		} else {

			return -1; //fwrite retorna EOF en caso de error en la escritura

		}

	}

	return -1;

}

int obtenerIndicePagina(t_list *listaPaginas, struct Pagina *pagina) {

	struct Pagina *paginaPivote;

	for (int i = 0; i < list_size(listaPaginas); i++) {
		paginaPivote = list_get(listaPaginas, i);

		if (!memcmp(pagina, paginaPivote, sizeof(struct Pagina))) {
			return i;
		}

	}

	return -1;
}

/**Obtiene los datos a actualizar que se requieren en el sync. Parametros:
 * --frame a copiar --desplazamiento --len --unSegmento --unaPagina */

void *obtenerDatosActualizados(int frame, int desplazamiento, size_t len,
		struct Segmento *unSegmento, struct Pagina *unaPagina) {

	void *pos = retornarPosicionMemoriaFrame(frame);
	pos = pos + desplazamiento;
	void *datosAActualizar = malloc(sizeof(len));

	int bytesACopiar = (int) len;

	if ((tam_pagina - desplazamiento) > len) {

		memcpy(datosAActualizar, pos, len);
		return datosAActualizar;

	} else {

		int cantidadPaginasALeer = (ceil)(len / tam_pagina);
		memcpy(datosAActualizar, pos, (tam_pagina - desplazamiento));
		cantidadPaginasALeer--;

		bytesACopiar = bytesACopiar - (tam_pagina - desplazamiento);

		//Me muevo a la proxima pagina (PRIMERO me tengo que fijar que haya pags para leer)
		while (list_get(unSegmento->tablaPaginas,
				obtenerIndicePagina(unSegmento->tablaPaginas, unaPagina) + 1)
				!= NULL && bytesACopiar > 0) {

			unaPagina = list_get(unSegmento->tablaPaginas,
					obtenerIndicePagina(unSegmento->tablaPaginas, unaPagina)
							+ 1);
			pos = retornarPosicionMemoriaFrame(unaPagina->numeroFrame);

			if (bytesACopiar >= tam_pagina) {

				memcpy(datosAActualizar, pos, tam_pagina);
				bytesACopiar = bytesACopiar - tam_pagina;

			} else {

				memcpy(datosAActualizar, pos, bytesACopiar);
				bytesACopiar = 0;

				return datosAActualizar;

			}

		}

	}

	//ver
	return NULL;
}

//MUSE UNMAP
/** Borra el mappeo a un archivo hecho por muse_map.
 * @param dir Dirección a memoria mappeada.
 * @param
 * @note Esto implicará que todas las futuras utilizaciones de direcciones basadas
 * en `dir` serán accesos inválidos.
 * @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan
 * liberado la misma cantidad de muse_unmap que muse_map.
 * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
 */
int muse_unmap(uint32_t dir, int idSocketCliente) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentos = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	//Obtencion segmento
	int idSegmento;

	//Obtencion idSegmento y segmento
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*) dir);
	unSegmento = list_get(listaSegmentos, idSegmento);

	if (unSegmento->esComun == true) { //Error

		return -1;

	} else {

		//Borro el segmento entero y actualizo el diccionario
		list_remove(listaSegmentos, idSegmento);
		dictionary_put(tablasSegmentos, (char*) idSocketCliente,
				listaSegmentos);
		return 0;

	}

	return -1;
}
