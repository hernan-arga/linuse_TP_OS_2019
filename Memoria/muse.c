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

	printf("%d \n", (int) (memoriaPrincipal));

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
	printf("La direccion de la mm ppal es %i \n", memoriaPrincipal);

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
		nuevoFrame->estaLibre = true;

		list_add(bitmapFrames, nuevoFrame);

	}

}

bool frameEstaLibre(int indiceFrame) {
	struct Frame *frame = list_get(bitmapFrames, indiceFrame);

	if (frame->estaLibre == true) {
		return true;
	} else {
		return false;
	}
}

void ocuparFrame(int unFrame) {
	struct Frame *frame = list_get(bitmapFrames, unFrame);

	frame->uso = 1;
	frame->estaLibre = false;

	//TODO list replace innecesario?
	list_replace(bitmapFrames, unFrame, frame); //Lo reemplazo modificado, unFrame es el indice del frame
}

void liberarFrame(int unFrame) {
	struct Frame *frame = list_get(bitmapFrames, unFrame);

	//TODO no estoy segura de apagar el uso
	//frame->uso = 0;
	frame->estaLibre = false;

	//TODO list replace innecesario?
	list_replace(bitmapFrames, unFrame, frame); //Lo reemplazo modificado
}

int buscarFrameLibre() {
	struct Frame *frame;

	for (int i = 0; i < cantidadFrames; i++) {
		frame = list_get(bitmapFrames, i);

		if (frame->estaLibre == true) {
			return i;
		}
	}

	free(frame);

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

	//int comienzoMemoria = (int) (&memoriaPrincipal); //Revisar esto todo

	//return ((&memoriaPrincipal) + offset);
	return (void*) (memoriaPrincipal + offset);
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
	struct Frame *frame;

	for (int i = 0; i < list_size(bitmapFrames); i++) {
		frame = list_get(bitmapFrames, i);
		if (frame->estaLibre == true) {
			return true;
		}
	}

	int indiceSwap;
	for (int j = 0; j < bitarray_get_max_bit(bitmapSwap); j++) {
		indiceSwap = bitarray_test_bit(bitmapSwap, j);

		if (indiceSwap == 0) {
			return true;
		}
	}

	return false;
}

//MUSE ALLOC

void *musemalloc(uint32_t tamanio, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	// primero que nada validar si hay lugar disponible en la memoria global

	if (segmentosProceso == NULL) { //No tiene tabla de segmentos creada, el proceso no se inicio
		return NULL;
	}

	if (list_is_empty(segmentosProceso)) { //Si no tiene ningun segmento se lo creo

		struct Segmento *segmento;
		segmento = crearSegmento(tamanio, idSocketCliente); //Se crea segmento para tamanio

		struct Pagina *primeraPagina;
		primeraPagina = list_get(segmento->tablaPaginas, 0);

		void *comienzoDatos = malloc(sizeof(int));
		//comienzoDatos = retornarPosicionMemoriaFrame(primeraPagina->numeroFrame) + sizeof(struct HeapMetadata);
		comienzoDatos = obtenerPosicionMemoriaPagina(primeraPagina)
				+ sizeof(struct HeapMetadata);

		free(stringIdSocketCliente);
		return comienzoDatos;

	} else { //busco si entra en algun segmento existente (metadata libre y con size disponible)

		struct Segmento *segmento = buscarSegmentoConTamanioDisponible(
				segmentosProceso, tamanio + sizeof(struct HeapMetadata));

		if (segmento != NULL) {

			struct HeapMetadata *metadata;
			struct HeapLista *heap;

			for (int j = 0; j < list_size(segmento->metadatas); j++) { //Recorro las metadatas del segmento
				heap = list_get(segmento->metadatas, j);

				metadata = leerMetadata(segmento, heap);

				if (metadata->isFree == true && metadata->size >= tamanio) {
					if (heap->estaPartido == false) { //Si no esta partida la metadata
						int indicePagina = (floor)(
								(double) heap->direccionHeap
										/ (double) pconfig->tamanio_pag);
						int desplazamientoPagina = heap->direccionHeap
								% pconfig->tamanio_pag;
						struct Pagina *pagina = list_get(segmento->tablaPaginas,
								indicePagina);

						return obtenerPosicionMemoriaPagina(pagina)
								+ desplazamientoPagina
								+ sizeof(struct HeapMetadata);
					} else { //Si esta partida la heap metadata
						int indicePagina = ((floor)(
								(double) heap->direccionHeap
										/ (double) pconfig->tamanio_pag)) + 1;
						int bytesAMoversePagina = sizeof(struct HeapMetadata)
								- (pconfig->tamanio_pag
										- (heap->direccionHeap
												% pconfig->tamanio_pag));
						struct Pagina *pagina = list_get(segmento->tablaPaginas,
								indicePagina);

						return obtenerPosicionMemoriaPagina(pagina)
								+ bytesAMoversePagina;
					}
				}

			}
		}

		//cierra if

		//Si sale del for sin retorno, tengo que buscar algun segmento de heap
		//que se pueda extender -siguiente for-
		struct Pagina *paginaMetadata;
		void *pos;
		for (int j = 0; j < list_size(segmentosProceso); j++) {
			struct Segmento *unSegmento = list_get(segmentosProceso, j);

			if (esExtendible(segmentosProceso, j, tamanio)) {

				unSegmento = extenderSegmento(unSegmento, tamanio); //revisar a partir de aca cuando no te alcanza para meter el dato en la pagina

				free(stringIdSocketCliente);

				//Si la metadata se parte me muevo a la proxima pagina, pongo en 0(donde arranca el frame)
				//una metadata y ocupo lo necesario (asignar primera pagina) y creo la proxima metadata

				//Retorno posicion ultima metadata + 5
				struct HeapLista *ultimoHeap = malloc(sizeof(struct HeapLista));
				ultimoHeap = list_get(unSegmento->metadatas,
						list_size(unSegmento->metadatas) - 2);

				if (ultimoHeap->estaPartido == false) { //Si no esta partida la metadata
					int indicePagina = (floor)(
							(double) ultimoHeap->direccionHeap
									/ (double) pconfig->tamanio_pag);
					int desplazamientoPagina = ultimoHeap->direccionHeap
							% pconfig->tamanio_pag;
					struct Pagina *pagina = list_get(unSegmento->tablaPaginas,
							indicePagina);

					return obtenerPosicionMemoriaPagina(pagina)
							+ desplazamientoPagina + sizeof(struct HeapMetadata);

				} else { //Si esta partida la heap metadata
					int indicePagina = ((floor)(
							(double) ultimoHeap->direccionHeap
									/ (double) pconfig->tamanio_pag)) + 1;
					int bytesAMoversePagina = sizeof(struct HeapMetadata)
							- (pconfig->tamanio_pag
									- (ultimoHeap->direccionHeap
											% pconfig->tamanio_pag));
					struct Pagina *pagina = list_get(unSegmento->tablaPaginas,
							indicePagina);

					return obtenerPosicionMemoriaPagina(pagina)
							+ bytesAMoversePagina;
				}
			}
		}
	} //cierra else

	//Si sale de todas las condiciones sin retorno, es que no encontro espacio libre ni
	//espacio que se pueda extender, por lo que debera crear un segmento nuevo

	//Creo segmento nuevo y retorno la posicion asignada
	struct Segmento *nuevoSegmento = crearSegmento(tamanio, idSocketCliente);

	// free(stringIdSocketCliente);

	//TODO vas a nuevo segmento, agarras el primer heap, calculas la metadata y como arriba
	//ves si esta partida o no, te fijas desplazamiento y to do eso, y retornas
	//desplazamientoHeap + 5 como verga sea
	//Devolver la primera metadata + 5

	struct HeapLista *heapnuevo = list_get(nuevoSegmento->metadatas, 0);

	int indicePaginaNueva = heapnuevo->direccionHeap / pconfig->tamanio_pag;

	struct Pagina *paginaNueva = list_get(nuevoSegmento->tablaPaginas,
			indicePaginaNueva);

	int desplazamientoNuevo = heapnuevo->direccionHeap % pconfig->tamanio_pag;

	if (heapnuevo->estaPartido == false) {

		return obtenerPosicionMemoriaPagina(paginaNueva)
				+ sizeof(struct HeapMetadata) + desplazamientoNuevo;

	} else {
		int indicePaginaSiguiente = ((floor)(
				(double) heapnuevo->direccionHeap
						/ (double) pconfig->tamanio_pag)) + 1;
		int bytesAMoversePagina = sizeof(struct HeapMetadata)
				- (pconfig->tamanio_pag
						- (heapnuevo->direccionHeap % pconfig->tamanio_pag));
		struct Pagina *pagina = list_get(nuevoSegmento->tablaPaginas,
				indicePaginaSiguiente);

		return obtenerPosicionMemoriaPagina(pagina) + bytesAMoversePagina;
	}

	return NULL;

}

//Lee una metadata en particular de un segmento, contemplando el caso de que este partida y todas esas forradas LA CONCHA DE TU PUTA MADRE
struct HeapMetadata *leerMetadata(struct Segmento *segmento,
		struct HeapLista *heapLista) {

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	int direccionMetadata = heapLista->direccionHeap;
	int indicePrimeraPagina = (floor)(direccionMetadata / pconfig->tamanio_pag);
	struct Pagina *pagina = list_get(segmento->tablaPaginas,
			indicePrimeraPagina);
	int desplazamientoPrimeraPagina = direccionMetadata % pconfig->tamanio_pag;
	void *pos = obtenerPosicionMemoriaPagina(pagina);

	if (heapLista->estaPartido == true) {
		memcpy(metadata, pos + desplazamientoPrimeraPagina,
				heapLista->bytesPrimeraPagina);

		//Me muevo a la proxima pagina y leo lo que resta
		pagina = list_get(segmento->tablaPaginas, indicePrimeraPagina++);
		pos = obtenerPosicionMemoriaPagina(pagina);
		memcpy(metadata + heapLista->bytesPrimeraPagina, pos,
				sizeof(struct HeapMetadata) - heapLista->bytesPrimeraPagina);
	} else {
		memcpy(metadata, pos + desplazamientoPrimeraPagina,
				sizeof(struct HeapMetadata));
	}

	return metadata;
}

//Crea el nuevo segmento Y ACTUALIZA todas las estructuras (lista segmentos y diccionario)
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);

	t_list *listaSegmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);
	struct Segmento *nuevoSegmento = malloc(sizeof(struct Segmento));
	nuevoSegmento->esComun = true;
	nuevoSegmento->filePath = NULL;
	nuevoSegmento->paginasLiberadas = 0;
	nuevoSegmento->metadatas = list_create();

	//Identificar segmento
	if (list_is_empty(listaSegmentosProceso)) { //Si es el primer segmento le pongo id 0, sino el id incremental que le corresponda
		nuevoSegmento->id = 0;
	} else {
		nuevoSegmento->id = list_size(listaSegmentosProceso);
	}

	if (list_is_empty(listaSegmentosProceso)) { //Si es el primer segmento, su base logica es 0

		nuevoSegmento->baseLogica = 0 + (int) memoriaPrincipal;

	} else { //Obtengo el tamaño del ultimo segmento

		int idUltimoSegmento = list_size(listaSegmentosProceso) - 1; //Id ultimo segmento
		nuevoSegmento->baseLogica = obtenerTamanioSegmento(idUltimoSegmento,
				idSocketCliente) + 1 + (int) memoriaPrincipal;

	}

	/*Asignar frames necesarios para *tamanio*, calculo paginas necesarias y le calculo
	 *el techo, asigno paginas y sus correspondientes frames*/
	int paginasNecesarias;

	//PARA CALCULAR LAS PAGS NECESARIAS se tiene en cuenta primer metadata y ultimo
	//porque se esta creando un segmento nuevo
	double paginas = (double) (tamanio + 2 * sizeof(struct HeapMetadata))
			/ (double) tam_pagina;
	paginasNecesarias = (int) (ceil(paginas));
	nuevoSegmento->tablaPaginas = list_create();
	//CAMBIO ESTO, PRUEBO COMO DIJO GONZA
	//FIXME
	int tamanioAlocado = tamanio + 2 * sizeof(struct HeapMetadata);
	nuevoSegmento->tamanio = 0; //se va incrementando a medida que se agregan pags
	bool *sePartioUnaMetadata = malloc(sizeof(bool));
	*sePartioUnaMetadata = false;

	while (paginasNecesarias > 0) {

		if (paginasNecesarias == (int) (ceil(paginas))
				|| paginasNecesarias == 1) { //Si es la primera pagina o la ultima(tener en cuenta tamaño metadata)

			if (paginasNecesarias == (int) (ceil(paginas))) { //Si es la primera pagina
				nuevoSegmento = asignarPrimeraPaginaSegmento(nuevoSegmento,
						tamanio, sePartioUnaMetadata);

				//Disminuye el tamaño que se aloco
				int diferenciaTamanios = pconfig->tamanio_pag
						- sizeof(struct HeapMetadata);

				if (diferenciaTamanios < 0) { //Si se corta la metadata
					tamanioAlocado -= pconfig->tamanio_pag;

					if (((int) tamanio) < diferenciaTamanios) { //NO se cortan los datos
						tamanioAlocado -= tamanio;
						//no se parten los datos

						paginasNecesarias -= (ceil)(
								(double) sizeof(struct HeapMetadata)
										/ (double) pconfig->tamanio_pag);
					} else {
						tamanioAlocado -= fabs(
								pconfig->tamanio_pag
										- sizeof(struct HeapMetadata));
						//Aca se parten los datos, agrego una pagina
						struct Pagina *paginaExtra = malloc(
								sizeof(struct Pagina));
						paginaExtra->presencia = 1;
						paginaExtra->numeroFrame = asignarUnFrame();
						paginaExtra->indiceSwap = -1;

						list_add(nuevoSegmento->tablaPaginas, paginaExtra);

						paginasNecesarias -= (ceil)(
								(double) sizeof(struct HeapMetadata)
										/ (double) pconfig->tamanio_pag);
						paginasNecesarias -= (ceil)(
								(double) (tamanio
										- (sizeof(struct HeapMetadata)
												- pconfig->tamanio_pag))
										/ (double) pconfig->tamanio_pag);
					}

				} else { //Si no se corta la metadata

					if (tamanio < diferenciaTamanios) {
						tamanioAlocado -= tamanio;
						paginasNecesarias--;
						//no se parten los datos
					} else {
						tamanioAlocado -= (pconfig->tamanio_pag
								- sizeof(struct HeapMetadata));
						//Aca se parten los datos, agrego una pagina
						struct Pagina *paginaExtra = malloc(
								sizeof(struct Pagina));
						paginaExtra->presencia = 1;
						paginaExtra->numeroFrame = asignarUnFrame();
						paginaExtra->indiceSwap = -1;

						list_add(nuevoSegmento->tablaPaginas, paginaExtra);
						paginasNecesarias -= (ceil)(
								(double) (tamanio
										- (sizeof(struct HeapMetadata)
												- pconfig->tamanio_pag))
										/ (double) pconfig->tamanio_pag);
					}
				}

				//Se encarga de la metadata y toda la falopa
				struct Pagina *primeraPagina = list_get(
						nuevoSegmento->tablaPaginas, 0);

				void *pos = obtenerPosicionMemoriaPagina(primeraPagina)
						+ tamanio + sizeof(struct HeapMetadata);

				struct HeapMetadata *ultimaMetadata = malloc(
						sizeof(struct HeapMetadata));
				/*
				 if (tamanio < pconfig->tamanio_pag) {
				 ultimaMetadata->isFree = true;
				 } else {
				 ultimaMetadata->isFree = false;
				 }*/
				ultimaMetadata->isFree = true;
				ultimaMetadata->size = pconfig->tamanio_pag
						- ((pconfig->tamanio_pag + tamanio
								+ 2 * sizeof(struct HeapMetadata))
								% pconfig->tamanio_pag);

				struct HeapLista *ultimoHeapLista = malloc(
						sizeof(struct HeapLista));

				int ubicacionHeap = obtenerIndicePagina(
						nuevoSegmento->tablaPaginas, primeraPagina)
						* pconfig->tamanio_pag + sizeof(struct HeapMetadata)
						+ tamanio;
				ubicarMetadataYHeapLista(nuevoSegmento, ubicacionHeap,
						ultimaMetadata->isFree, ultimaMetadata->size,
						sePartioUnaMetadata);

				if ((*sePartioUnaMetadata) == true) {
					paginasNecesarias--;
				}

			}

			if (paginasNecesarias == 1) { //Si es la ultima pagina

				nuevoSegmento = asignarUltimaPaginaSegmento(nuevoSegmento,
						(tam_pagina - (tamanio % tam_pagina)
								- sizeof(struct HeapMetadata))); //tamanio prox metadata?
				tamanioAlocado = 0;
				paginasNecesarias--;

				/*
				 for(int i = 0; i < list_size(primeraPagina->listaMetadata); i++){

				 int metadata = (int)list_get(primeraPagina->listaMetadata, i);
				 printf("una metadata es %i \n",metadata);

				 }*/
			}

		} /*else { //Si es la primera pagina

		 struct Pagina *primeraPagina = list_get(nuevoSegmento->tablaPaginas,0);

		 void *pos = obtenerPosicionMemoriaPagina(primeraPagina) + tamanio + sizeof(struct HeapMetadata);

		 struct HeapMetadata *ultimaMetadata = malloc(sizeof(struct HeapMetadata));
		 ultimaMetadata->isFree = true;
		 //
		 ultimaMetadata->size = pconfig->tamanio_pag - tamanio - (2 * sizeof(struct HeapMetadata));

		 struct HeapLista *ultimoHeapLista = malloc(sizeof(struct HeapLista));

		 int ubicacionHeap = obtenerIndicePagina(nuevoSegmento->tablaPaginas, primeraPagina) * pconfig->tamanio_pag + sizeof(struct HeapMetadata) + tamanio;
		 ubicarMetadataYHeapLista(nuevoSegmento, ubicacionHeap, ultimaMetadata->isFree, ultimaMetadata->size, sePartioUnaMetadata);

		 if((*sePartioUnaMetadata) == true){
		 paginasNecesarias = paginasNecesarias - 2;
		 } else{
		 paginasNecesarias--;
		 }
		 }*/
		else { //Si es una pagina intermedia

			if ((*sePartioUnaMetadata) == true) {
				//Agarra la ultima pagina y la usa
				tamanioAlocado -= pconfig->tamanio_pag;
				(*sePartioUnaMetadata) = false;
				//no se agregaron paginas, no se disminuye pag necesarias
			} else {
				nuevoSegmento = asignarNuevaPagina(nuevoSegmento, tam_pagina);
				tamanioAlocado = tamanioAlocado - pconfig->tamanio_pag;
				paginasNecesarias--;
			}

		}

		/*
		 if((*sePartioUnaMetadata) == true){
		 paginasNecesarias = paginasNecesarias - list_size(nuevoSegmento->tablaPaginas);
		 } else {
		 paginasNecesarias--;
		 }*/

	}

	list_add(listaSegmentosProceso, nuevoSegmento);

	//Actualizo la data en el diccionario
	//dictionary_put(tablasSegmentos, stringIdSocketCliente, listaSegmentosProceso);

	free(stringIdSocketCliente);
	return nuevoSegmento;
}

/*Recibe el segmento donde se crea la metadata y el heap, la ubicacion del heap relativa al segmento, si esta libre y cuanto*/
struct HeapLista *ubicarMetadataYHeapLista(struct Segmento *segmento,
		int ubicacionHeap, bool isFree, uint32_t size,
		bool *sePartioUnaMetadata) {
	struct HeapLista *heap = malloc(sizeof(struct HeapLista));

	heap->direccionHeap = ubicacionHeap;
	heap->isFree = isFree;
	heap->size = size;

	int indicePrimeraPagina = (floor)(ubicacionHeap / pconfig->tamanio_pag);
	if (ubicacionHeap != 0 && ubicacionHeap % pconfig->tamanio_pag == 0) {
		indicePrimeraPagina--;
	}
	struct Pagina *pagina = list_get(segmento->tablaPaginas,
			indicePrimeraPagina);
	int desplazamientoPrimeraPagina = ubicacionHeap % pconfig->tamanio_pag;

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	metadata->size = size;
	metadata->isFree = isFree;
	void *pos = obtenerPosicionMemoriaPagina(pagina)
			+ desplazamientoPrimeraPagina;

	if (pconfig->tamanio_pag - desplazamientoPrimeraPagina
			< sizeof(struct HeapMetadata)) { //Si queda partida
		heap->estaPartido = true;
		heap->bytesPrimeraPagina = pconfig->tamanio_pag
				- desplazamientoPrimeraPagina;
		//heap->size = size - (sizeof(struct HeapMetadata) - heap->bytesPrimeraPagina);

		//Agrego el heaplista al segmento
		list_add(segmento->metadatas, heap);

		//Pego la metadata en los frames (queda partida)
		memcpy(pos, metadata, heap->bytesPrimeraPagina);
		struct Pagina *segundaPagina = malloc(sizeof(struct Pagina));
		segundaPagina->numeroFrame = asignarUnFrame();
		segundaPagina->presencia = 1;
		segundaPagina->indiceSwap = -1;
		list_add(segmento->tablaPaginas, segundaPagina);

		segmento->tamanio += pconfig->tamanio_pag;

		(*sePartioUnaMetadata) = true;

		pos = obtenerPosicionMemoriaPagina(segundaPagina); //escribo lo que queda en el proximo frame
		memcpy(pos, metadata + heap->bytesPrimeraPagina,
				sizeof(struct HeapMetadata) - heap->bytesPrimeraPagina);

		// CODIGO DE MIERDA
		struct HeapMetadata *metadataprueba = malloc(
				sizeof(struct HeapMetadata));
		void *pos = obtenerPosicionMemoriaPagina(pagina)
				+ desplazamientoPrimeraPagina;
		memcpy(metadataprueba, pos, 5);

		//printf("bla");

	} else { //Si no queda partida
		heap->estaPartido = false;
		heap->bytesPrimeraPagina = sizeof(struct HeapMetadata);
		//heap->size = size;

		//Agrego el heaplista al segmento
		list_add(segmento->metadatas, heap);

		//Pego la metadata en el frame
		memcpy(pos, metadata, sizeof(struct HeapMetadata));
	}

	return heap;
}

struct Segmento *buscarSegmentoConTamanioDisponible(t_list *segmentos,
		int tamanio) {
	struct Segmento *segmento;

	for (int i = 0; i < list_size(segmentos); i++) {
		segmento = list_get(segmentos, i);
		struct HeapLista *heapAux;

		for (int i = 0; i < list_size(segmento->metadatas); i++) {
			heapAux = list_get(segmento->metadatas, i);

			if (heapAux->isFree == true && heapAux->size >= tamanio) {
				return segmento;
			}
		}

	}

	return NULL;
}

struct Segmento *extenderSegmento(struct Segmento *segmento, uint32_t tamanio) {

	//busco ultima metadata (que tiene que estar libre) y chequeo si hay que extender
	//o si alcanza, en to do caso se creara la nueva metadata que separe

	struct HeapLista *ultimoHeapLista = list_get(segmento->metadatas,
			list_size(segmento->metadatas) - 1);
	int paginasNecesarias;
	int bytesAAgregar = tamanio + (sizeof(struct HeapMetadata));

	if (ultimoHeapLista->isFree == true) {
		bytesAAgregar -= ultimoHeapLista->size;
	}
	ultimoHeapLista->size = tamanio;
	paginasNecesarias = (int) (ceil(
			(double) bytesAAgregar / (double) tam_pagina));

	while (paginasNecesarias > 0) {

		if (bytesAAgregar > tam_pagina) {

			segmento = asignarNuevaPagina(segmento, tam_pagina);
			bytesAAgregar = bytesAAgregar - tam_pagina;

		} else {

			//asignar ultima pagina ya me crea la ultima metadata necesaria
			segmento = asignarUltimaPaginaSegmento(segmento, bytesAAgregar);
			bytesAAgregar = 0;

		}

		paginasNecesarias--;

	}

	//free(paginaUltimaMetadata);
	//free(frame);
	//free(nuevaUltimaMetadata);

	return segmento;
}

/*
 int indicePaginaQueContieneUltimaMetadata(struct Segmento *segmento){
 t_list *paginas = segmento->tablaPaginas;
 //t_list *metadatas = list_create();
 t_list *metadatas;
 struct Pagina *pagina;
 struct HeapMetadata *ultimaMetadata = malloc(sizeof(struct HeapMetadata));
 int paginaUltimaMetadata = -1;
 ultimaMetadata->size = -1;
 struct HeapMetadata *metadataResultado;

 for(int i = 0; i < list_size(paginas); i++){
 pagina = list_get(paginas, i);
 metadatas = (t_list*)(pagina->listaMetadata);
 int posMeta = (int)list_get(metadatas, (list_size(metadatas) - 1));
 memcpy(ultimaMetadata, obtenerPosicionMemoriaPagina(pagina) + posMeta, sizeof(struct HeapMetadata));

 if(ultimaMetadata->size != -1){
 metadataResultado = ultimaMetadata;
 paginaUltimaMetadata = i;
 }

 }

 return paginaUltimaMetadata;
 }*/

int retornarMetadataTamanioLibre(struct Segmento *segmento, uint32_t tamanio) {
	struct HeapLista *heapAux;

	for (int i = 0; i < list_size(segmento->metadatas); i++) {
		heapAux = list_get(segmento->metadatas, i);

		if (heapAux->isFree == true && heapAux->size >= tamanio) {
			return heapAux->direccionHeap + sizeof(struct HeapMetadata);
		}
	}

	return -1; //nunca llega, porque antes de llamar a esta funcion, chequee que exista metadata libre
}

/*Le asigna la primera pagina que va a contener la metadata al segmento*/
struct Segmento *asignarPrimeraPaginaSegmento(struct Segmento *segmento,
		int tamanioMetadata, bool *sePartioUnaMetadata) {

	struct Pagina *primeraPagina = malloc(sizeof(struct Pagina));
	primeraPagina->numeroFrame = asignarUnFrame();
	primeraPagina->presencia = 1;
	primeraPagina->indiceSwap = -1;

	list_add(segmento->tablaPaginas, primeraPagina);

	int pos = obtenerIndicePagina(segmento->tablaPaginas, primeraPagina)
			* pconfig->tamanio_pag; //como esla primera metadata, no hay desplazamiento (esta en 0 la metadata)

	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	metadata->isFree = false;
	metadata->size = tamanioMetadata;

	ubicarMetadataYHeapLista(segmento, pos, metadata->isFree, metadata->size,
			sePartioUnaMetadata);

	if ((*sePartioUnaMetadata) == true) {
		segmento->tamanio += 2 * pconfig->tamanio_pag;
	} else {
		segmento->tamanio += pconfig->tamanio_pag;
	}

	return segmento;
}

int asignarUnFrame() {

	int frame;
	bool hayQueSwappear;

	if (hayFramesLibres() == true) {
		frame = buscarFrameLibre();
		hayQueSwappear = false;
	} else {
		frame = clockModificado();
		hayQueSwappear = true;

	}

	struct Frame *frameReemplazo = malloc(sizeof(struct Frame));
	frameReemplazo = list_get(bitmapFrames, frame);
	frameReemplazo->modificado = 0;
	frameReemplazo->uso = 1;
	frameReemplazo->estaLibre = false;

	//Swappeo pagina desalojada - busco la pagina que se encuentra en ese frame
	if(hayQueSwappear == true){
		struct Pagina *paginaASwappear = buscarPaginaAsociadaAFrame(frame);
		int indiceSwap;

		if (paginaASwappear != NULL) {
			indiceSwap = llevarASwapUnaPagina(paginaASwappear);
			paginaASwappear->indiceSwap = indiceSwap;
			paginaASwappear->numeroFrame = -1;
			paginaASwappear->presencia = 0;
		}

	}

	//list_replace(bitmapFrames, frame, frameReemplazo);

	return frame;
}

struct Pagina *buscarPaginaAsociadaAFrame(int indiceFrame) {
	t_list *segmentos;
	t_list *paginas;
	struct Segmento *segmento;
	struct Pagina *pagina;

	for (int i = 0; i < dictionary_size(tablasSegmentos); i++) { //Recorro cada proceso
		segmentos = dictionary_get(tablasSegmentos, string_itoa(i));

		for (int j = 0; j < list_size(segmentos); j++) { //Cada lista de segmentos
			segmento = list_get(segmentos, j);
			paginas = segmento->tablaPaginas;

			for (int k = 0; k < list_size(paginas); k++) { //Cada lista de paginas
				pagina = list_get(paginas, k);

				if (pagina->numeroFrame == indiceFrame) {
					return pagina;
				}
			}

		}
	}

	return NULL;
}

/*Le asigna la ultima pagina a un segmento, poniendo la siguiente metadata en el frame*/
struct Segmento *asignarUltimaPaginaSegmento(struct Segmento *segmento,
		int tamanioUltimaMetadata) {

	bool *sePartioUnaMetadata = malloc(sizeof(bool));
	*sePartioUnaMetadata = false;

	struct HeapMetadata *ultimaMetadata = malloc(sizeof(struct HeapMetadata));
	ultimaMetadata->isFree = true;
	if (pconfig->tamanio_pag - tamanioUltimaMetadata == 0) {
		ultimaMetadata->isFree = false;
	}
	ultimaMetadata->size = pconfig->tamanio_pag - tamanioUltimaMetadata;
	//list_add(ultimaPagina->listaMetadata, (void*)(tam_pagina - sizeof(struct HeapMetadata) - tamanioUltimaMetadata));

	int direccionHeap = ((list_size(segmento->tablaPaginas) - 1)
			* pconfig->tamanio_pag)
			+ (pconfig->tamanio_pag - sizeof(struct HeapMetadata)
					+ tamanioUltimaMetadata);
	//int direccionHeap = (int)(obtenerPosicionMemoriaPagina(ultimaPagina) + (pconfig->tamanio_pag - tamanioUltimaMetadata - sizeof(struct HeapMetadata)));
	ubicarMetadataYHeapLista(segmento, direccionHeap, ultimaMetadata->isFree,
			ultimaMetadata->size, sePartioUnaMetadata);

	//TODO me parece que esta hecho en ubicar metadata
	//Pone la metadata en el frame correspondiente
	//memcpy(pos, ultimaMetadata, sizeof(struct HeapMetadata));

	segmento->tamanio += pconfig->tamanio_pag;

	return segmento;
}

/*Le asigna una pagina intermedia a un segmento, no hay metadata inicial ni final*/
struct Segmento *asignarNuevaPagina(struct Segmento *segmento, int tamanio) {

	struct Pagina *nuevaPagina = malloc(sizeof(struct Pagina));
	nuevaPagina->numeroFrame = asignarUnFrame();
	nuevaPagina->presencia = 1;
	nuevaPagina->indiceSwap = -1;

	//Ocupo frame y lo reemplazo - modifico - en el bitmap de frames
	struct Frame *frame = malloc(sizeof(struct Frame));
	frame = list_get(bitmapFrames, nuevaPagina->numeroFrame);
	frame->modificado = 0;
	frame->uso = 1;
	frame->estaLibre = false;
	list_replace(bitmapFrames, nuevaPagina->numeroFrame, frame);

	list_add(segmento->tablaPaginas, nuevaPagina);

	segmento->tamanio += pconfig->tamanio_pag;

	return segmento;

}

/* Recorre el espacio del segmento buscando espacio libre. Debe encontrar un header
 * que isFree = true y size >= tamanio (si se busca espacio tambien para metadata, pasar
 * toda la suma en el parametro)*/
bool poseeTamanioLibreSegmento(struct Segmento *segmento, uint32_t tamanio) {
	struct HeapLista *heapAux;

	for (int i = 0; i < list_size(segmento->metadatas); i++) {
		heapAux = list_get(segmento->metadatas, i);

		if (heapAux->isFree == true && heapAux->size <= tamanio) {
			return true;
		}
	}

	return false; //si sale sin retornar true, no encontro metadata util

}

struct Segmento *asignarTamanioLibreASegmento(struct Segmento *segmento,
		uint32_t tamanio) {

	t_list *paginas = segmento->tablaPaginas;
	struct Pagina *pagina; // = malloc(sizeof(struct Pagina));
	t_list *metadatas; // = list_create();
	void *pos;
	struct HeapMetadata *metadata = malloc(sizeof(struct HeapMetadata));
	struct HeapMetadata *nuevaMetadata = malloc(sizeof(struct HeapMetadata));
	int espacioLibreEnPagina;
	int bytesAMoverme = -1;

	for (int i = 0; i < list_size(paginas); i++) { //Recorro las paginas buscando metadata libre

		pagina = list_get(paginas, i);
		pos = retornarPosicionMemoriaFrame(pagina->numeroFrame);
		//metadatas = pagina->listaMetadata;

		if (bytesAMoverme != -1) {

			if (bytesAMoverme < tam_pagina) {
				pos = pos + bytesAMoverme;

				int menorMetadata = (int) list_get(segmento->metadatas, 0); //estando siempre ordenadas

				//Agregar metadata a la lista del frame y modificar frame
				nuevaMetadata->isFree = true;
				nuevaMetadata->size = menorMetadata - bytesAMoverme
						- sizeof(struct HeapMetadata);
				memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));

			}

		}

		int desplazamiento;

		for (int k = 0; k < list_size(segmento->metadatas); k++) {

			desplazamiento = (int) list_get(segmento->metadatas, k);
			pos = pos + desplazamiento; //me posiciono donde esta la metadata a leer
			memcpy(metadata, pos, sizeof(struct HeapMetadata)); //Leo la metadata

			if (metadata->isFree == true && metadata->size <= tamanio) {

				//Veo si me entra todo en el espacio libre de la pagina o si debo moverme entre paginas
				espacioLibreEnPagina = tam_pagina - desplazamiento /*desplazamiento hasta la metadata*/
						- sizeof(struct HeapMetadata);

				if (espacioLibreEnPagina >= tamanio) { //me entra en esta pagina, lo ubico
					metadata->isFree = false;
					metadata->size = metadata->size - tamanio;

					//Creo la nueva metadata (siguiente metadata) en el frame
					nuevaMetadata->isFree = true;
					nuevaMetadata->size = tam_pagina - desplazamiento
							- metadata->size
							- (2 * sizeof(struct HeapMetadata));
					//AGREGAR METADATA al frame y modificar estructuras

					pos = pos + sizeof(struct HeapMetadata) + metadata->size;
					memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));

					if (metadata->isFree == true && metadata->size <= tamanio) {
						//Veo si me entra t odo en el espacio libre de la pagina o si debo moverme entre paginas
						espacioLibreEnPagina = tam_pagina - desplazamiento /*desplazamiento hasta la metadata*/
								- sizeof(struct HeapMetadata);

						if (espacioLibreEnPagina >= tamanio) { //me entra en esta pagina, lo ubico
							metadata->isFree = false;
							metadata->size = metadata->size - tamanio;

							//Creo la nueva metadata (siguiente metadata) en el frame
							nuevaMetadata->isFree = true;
							nuevaMetadata->size = tam_pagina - desplazamiento
									- metadata->size
									- (2 * sizeof(struct HeapMetadata));
							//AGREGAR METADATA al frame y modificar estructuras

							pos = pos + sizeof(struct HeapMetadata)
									+ metadata->size;
							memcpy(pos, nuevaMetadata,
									sizeof(struct HeapMetadata));

						} else { //si no me entra en esta pagina, me muevo entre paginas hasta donde deba ubicar (primero ocupo la metadata)
							bytesAMoverme = tamanio
									- sizeof(struct HeapMetadata);

							//No me entra en esta pagina, me muevo a las siguientes, descontando lo que me movi en esta pagina
							bytesAMoverme = bytesAMoverme
									- espacioLibreEnPagina;

						}

					}
				}
			}
		}
	}
	//free pagina, frame, metadata

	return NULL; //nunca llegaria aca, porque siempre se llama a esta funcion chequeando antes que posea tamaño libre

}

/*Un segmento es extendible si: es comun y es el ultimo en la lista O tiene x paginas liberadas*/
bool esExtendible(t_list *segmentosProceso, int unIndice, int tamanio) {
	int indiceUltimoSegmento = list_size(segmentosProceso) - 1;

	struct Segmento *ultimoSegmento = list_get(segmentosProceso,
			indiceUltimoSegmento);

	struct Segmento *segmento = list_get(segmentosProceso, unIndice);

	if (segmento->esComun == true && ultimoSegmento->id == segmento->id) {
		return true;
	} else {
		struct HeapLista *heap = malloc(sizeof(struct HeapLista));
		heap = list_get(segmento->metadatas,
				list_size(segmento->metadatas) - 1);
		int paginasNecesarias = (int) ceil(
				(double) (tamanio - heap->size + sizeof(struct HeapMetadata))
						/ (double) tam_pagina);
		if (segmento->esComun == true
				&& segmento->paginasLiberadas >= paginasNecesarias) {
			return true;
		}
	}

	return false;
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
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);
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
			nuevaMetadata->size = sizePrimerFrame - tamanio
					- sizeof(struct HeapMetadata);

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

/*Unificar headers con nueva implementacion, me retorna el segmento modificado*/
struct Segmento *unificarHeaders(int idSegmento, int idSocketCliente) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentos = dictionary_get(tablasSegmentos, stringIdSocketCliente);

	//Segmento a unificar
	struct Segmento *segmento = list_get(segmentos, idSegmento);

	struct HeapLista *heap;
	struct HeapLista *siguienteHeap;

	struct HeapMetadata *nuevoMetadata = malloc(sizeof(struct HeapMetadata));

	if (list_size(segmento->metadatas) >= 2) {

		heap = list_get(segmento->metadatas, 0);

		for (int i = 1; i < list_size(segmento->metadatas) - 1; i++) {
			siguienteHeap = list_get(segmento->metadatas, i);

			if (heap->isFree == true && siguienteHeap->isFree == true) {
				heap->size = heap->size + siguienteHeap->size;

				//Elimino la siguiente metadata (se unifico con la primera)
				list_remove(segmento->metadatas, i);

				//Modifico el heapmetadata en el frame
				nuevoMetadata->isFree = true;
				nuevoMetadata->size = heap->size;

				memcpy((void*) (segmento->baseLogica + heap->direccionHeap),
						nuevoMetadata, sizeof(struct HeapMetadata));
			} else {

				heap = siguienteHeap;

			}
		}
	}

	free(stringIdSocketCliente);
	return segmento;
}

/*
 void unificarHeaders(int idSocketCliente, int idSegmento) {
 char *stringIdSocketCliente = string_itoa(idSocketCliente);
 t_list *segmentosProceso = dictionary_get(tablasSegmentos, stringIdSocketCliente);

 struct Segmento *segmento = malloc(sizeof(struct Segmento));
 segmento = list_get(segmentosProceso, idSegmento);

 t_list *paginasSegmento = segmento->tablaPaginas;
 int numeroPagina = 0;
 int paginasARecorrer = list_size(paginasSegmento);

 struct Pagina *primeraPagina; //= malloc(sizeof(struct Pagina));
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
 }*/

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
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);
	void *resultado = NULL;

	if (listaSegmentos == NULL) {
		//Hacer log
		return NULL;
	}

	int idSegmento = idSegmentoQueContieneDireccion(listaSegmentos,
			(void*) src);
	struct Segmento *segmento = list_get(listaSegmentos, idSegmento);

	if (segmento == NULL) {
		//Hacer log
		return NULL;
	}

	int direccionSeg = (int) src - segmento->baseLogica;
	int primeraPagina = (int) (floor)(
			(double) direccionSeg / (double) pconfig->tamanio_pag);
	int ultimaPagina = (int) (floor)(
			(double) (direccionSeg + n) / (double) (pconfig->tamanio_pag));
	int desplazamiento = direccionSeg % tam_pagina;

	if (segmento->tamanio - direccionSeg >= n
			&& primeraPagina * tam_pagina + desplazamiento + n
					<= segmento->tamanio) {
		int cantidadPaginas = ultimaPagina - primeraPagina + 1;
		resultado = malloc(n);
		void *recorridoPaginas = malloc(cantidadPaginas * tam_pagina);
		//recorridoPaginas = obtenerPosicionMemoriaPagina(list_get(segmento->tablaPaginas,primeraPagina)) ;
		int puntero = 0;

		if (segmento->esComun == false) {

			//me traigo las paginas necesarias a memoria en caso de que no esten (cuando se mmapea no se las trae, podrian no estar ni
			//en mm ppal ni en swap)
			traerPaginasSegmentoMmapAMemoria(direccionSeg, n, segmento);

		}

		for (int i = 0; i < cantidadPaginas; i++, puntero += tam_pagina) {
			struct Pagina *pagina = list_get(segmento->tablaPaginas,
					primeraPagina + i);
			struct Frame *frame = list_get(bitmapFrames, pagina->numeroFrame);
			pagina->presencia = 1;

			void *pos = obtenerPosicionMemoriaPagina(pagina);

			frame->uso = 1;

			memcpy(recorridoPaginas + puntero, pos, tam_pagina);
		}

		memcpy(resultado, recorridoPaginas + desplazamiento, n);
		free(recorridoPaginas);
	}

	//printf("Lo leido es %i \n", *(int*)resultado);

	return resultado;
}

/*Retorna la posicion de memoria del frame en caso de estar en mm ppal y en caso de no estar en mm ppal, la trae de swap*/
void *obtenerPosicionMemoriaPagina(struct Pagina *pagina) {

	if (pagina->presencia == 0) {

		if (pagina->indiceSwap != -1) {

			void* paginaReemplazo = malloc(tam_pagina);
			memcpy(paginaReemplazo, swap + pagina->indiceSwap * tam_pagina,
					tam_pagina);

			bitarray_clean_bit(bitmapSwap, pagina->indiceSwap); //libero el indice en swap
			pagina->indiceSwap = -1;

			int frame = buscarFrameLibre();
			pagina->numeroFrame = frame;
			pagina->presencia = 1;

			//Paso la pagina a memoria
			memcpy(retornarPosicionMemoriaFrame(frame), paginaReemplazo,
					tam_pagina);

		} /*else{

		 //No deberia llegar aca (?)

		 }*/
	}

	struct Frame *frame = list_get(bitmapFrames, pagina->numeroFrame);
	frame->uso = 1;

	list_replace(bitmapFrames, pagina->numeroFrame, frame);

	return memoriaPrincipal + pagina->numeroFrame * pconfig->tamanio_pag;
}

/*Trae las paginas de un segmento de mmap a mm ppal en caso de no estar*/
//Si me pide un length mayor a lo que tiene el segmento mmap, relleno con /0 (???)
void traerPaginasSegmentoMmapAMemoria(int direccion, int tamanio,
		struct Segmento *segmento) {

	t_list *paginas = segmento->tablaPaginas;
	int primeraPagina = direccion / tam_pagina;
	int ultimaPagina = (int) ceil(
			((double) (direccion + tamanio) / tam_pagina) - 1);
	int desplazamiento = direccion % tam_pagina;

	if (paginasNecesariasCargadas(primeraPagina, ultimaPagina, paginas) == false) { //Si hay que traerse alguna pagina del archivo

		int cantidadPaginas = ultimaPagina - primeraPagina + 1;
		int bytesALeer = cantidadPaginas * tam_pagina;
		void *datos = malloc(bytesALeer);

		//Abro el archivo asociado al segmento mmap
		FILE *archivoMmap = fopen(segmento->filePath, "rb");
		//Leo los bytes necesarios
		fread(datos, tam_pagina, cantidadPaginas, archivoMmap);
		//Cierro el archivo
		fclose(archivoMmap);

		//Traigo las paginas necesarias a frames de mm ppal
		int punteroPagina = primeraPagina * tam_pagina;

		for (int i = primeraPagina; i <= ultimaPagina; i++, bytesALeer -=
				tam_pagina, punteroPagina += tam_pagina) {
			struct Pagina *pagina = list_get(paginas, i);

			if (pagina->indiceSwap == -1 && pagina->numeroFrame == -1) { //Si no esta en swap ni en mm ppal, se la trae a mm

				int indicePagina = obtenerIndicePagina(paginas, pagina);
				pagina->numeroFrame = asignarUnFrame();
				pagina->presencia = 1; //paso a mm ppal
				pagina->indiceSwap = -1;

				struct Frame *frame = list_get(bitmapFrames,
						pagina->numeroFrame);
				frame->uso = 1;
				frame->modificado = 0;

				void *pos = obtenerPosicionMemoriaPagina(pagina);

				if (indicePagina == list_size(paginas) - 1) { //Si es la ultima pagina del segmento tengo que rellenar con /0

					int relleno = bytesALeer - desplazamiento - tamanio;
					void *rellenoPagina = obtenerRellenoPagina(relleno);

					memcpy(pos, datos + punteroPagina, tam_pagina - relleno);
					memcpy(pos + (tam_pagina - relleno), rellenoPagina,
							relleno);

				} else { //Sino, copio la pagina entera

					memcpy(pos, datos + punteroPagina, tam_pagina);

				}

			}

		}

	}

}

/*Se fija si TODAS las paginas necesarias del segmento mmap estan cargadas en mm ppal, si alguna
 *NO esta cargada en mm ppal, retorna false*/
bool paginasNecesariasCargadas(int primeraPagina, int ultimaPagina,
		t_list *paginas) {

	for (int i = 0; i < list_size(paginas); i++) {
		struct Pagina *pagina = list_get(paginas, i);
		int indicePagina = obtenerIndicePagina(paginas, list_get(paginas, i));

		//Si es una de las paginas necesarias
		if (indicePagina >= primeraPagina && indicePagina <= ultimaPagina) {

			//Si esta en mm ppal O esta en swap, retorno false
			if (pagina->numeroFrame != -1 || pagina->indiceSwap != -1) {
				return false;
			}

		}

	}

	return true;
}

/*Genera el relleno de /0 para segmento mmap*/
void *obtenerRellenoPagina(int tamanio) {

	void* relleno = malloc(tamanio);
	char* caracterRelleno = malloc(1);
	char caracter = '\0';
	memcpy(caracterRelleno, &caracter, 1);
	int puntero = 0;

	for (int i = 0; i < tamanio; i++, puntero++) {

		memcpy(relleno + puntero, (void*) caracterRelleno, 1);

	}

	free(caracterRelleno);
	return relleno;
}

int idSegmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion) {
	int segmentosARecorrer = list_size(listaSegmentos);
	struct Segmento *unSegmento = malloc(sizeof(struct Segmento));

	for (int i = 0; i < segmentosARecorrer; i++) {
		unSegmento = list_get(listaSegmentos, i);

		if (((int) direccion - unSegmento->baseLogica) < unSegmento->tamanio) {
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
			((((int) direccion) - unSegmento->baseLogica))
					/ pconfig->tamanio_pag);

	struct Pagina *pagina = list_get(listaPaginas, indicePagina);

	return pagina;
}

//MUSE CPY

int musecpy(uint32_t dst, void* src, int n, int idSocketCliente) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	if (listaSegmentos == NULL) {
		return -1;
	}

	struct Segmento *unSegmento = segmentoQueContieneDireccion(listaSegmentos,
			dst);

	if (unSegmento == NULL) {
		return -1;
	}

	int direccion = (int) dst;
	int indicePaginaInicial;
	int desplazamientoPagina;
	struct Pagina *paginaInicial;
	int indicePaginaFinal;
	struct Pagina *paginaFinal;
	struct HeapLista *heap;
	int desplazamientoUltimaPagina;
	int direccionInicial;
	int direccionFinal;
	int bytesAMoverse;

	//Para identificar si no encontro el heap
	struct HeapLista *heapFinal = malloc(sizeof(struct HeapLista));
	heapFinal->size = -1;

	if (unSegmento->esComun == true) { //Recorro los metadatas del segmento buscando la que contenga direccion

		for (int i = 0; i < list_size(unSegmento->metadatas); i++) {
			heap = list_get(unSegmento->metadatas, i);

			indicePaginaInicial = (floor)(
					(double) heap->direccionHeap
							/ (double) pconfig->tamanio_pag);
			paginaInicial = list_get(unSegmento->tablaPaginas,
					indicePaginaInicial);
			desplazamientoPagina = (heap->direccionHeap
					+ sizeof(struct HeapMetadata)) % pconfig->tamanio_pag;
			indicePaginaFinal = (floor)(
					(double) (heap->direccionHeap + sizeof(struct HeapMetadata)
							+ n) / (double) pconfig->tamanio_pag);
			paginaFinal = list_get(unSegmento->tablaPaginas, indicePaginaFinal);
			desplazamientoUltimaPagina = (heap->direccionHeap
					+ sizeof(struct HeapMetadata) + n) % pconfig->tamanio_pag;

			if (heap->estaPartido == true) {
				//voy a la proxima pagina y me muevo hasta donde termine la metadata

				struct Pagina *proximaPagina = list_get(
						unSegmento->tablaPaginas, indicePaginaInicial + 1);

				bytesAMoverse = (int) (sizeof(struct HeapMetadata))
						- (heap->bytesPrimeraPagina);

				direccionInicial = (int) (obtenerPosicionMemoriaPagina(
						proximaPagina) + bytesAMoverse);

			} else {
				direccionInicial = (int) (obtenerPosicionMemoriaPagina(
						paginaInicial) + desplazamientoPagina);
			}

			//tengo que ver si se parte la metadata, en ese caso obtengo la prox pag y el frame sig
			direccionFinal = (int) (obtenerPosicionMemoriaPagina(paginaFinal)
					+ desplazamientoUltimaPagina);

			if (direccion >= direccionInicial && direccion <= direccionFinal) {
				heapFinal = heap;
				break;
			}
		}

		//pos - donde tengo que escribir

		if (heapFinal->size == -1) { //No se encontro el heap de destino
			return -1;
		}

		if (heapFinal->size < n) {
			return -1;
		}

		//Empiezo a copiar los datos en las paginas
		int bytesPrimeraPagina;
		//int bytesAMover;
		struct Pagina *proxPagina;
		struct Frame *frameInicial;
		struct Pagina *paginaInicialPosta;
		int indicePaginaInicialPosta;

		if (heapFinal->estaPartido == true) {
			//voy a la proxima pagina y me muevo hasta donde termine la metadata

			proxPagina = list_get(unSegmento->tablaPaginas,
					indicePaginaInicial + 1);
			paginaInicialPosta = proxPagina;
			bytesAMoverse = (int) (sizeof(struct HeapMetadata))
					- heap->bytesPrimeraPagina;
			frameInicial = list_get(bitmapFrames, proxPagina->numeroFrame);
			bytesPrimeraPagina = pconfig->tamanio_pag - bytesAMoverse;
			indicePaginaInicialPosta = indicePaginaInicial + 1;

		} else {

			bytesPrimeraPagina = pconfig->tamanio_pag
					- ((direccion - unSegmento->baseLogica)
							% pconfig->tamanio_pag);
			frameInicial = list_get(bitmapFrames, paginaInicial->numeroFrame);
			paginaInicialPosta = paginaInicial;
			indicePaginaInicialPosta = indicePaginaInicial;
		}

		int bytesACopiar = n;
		void *pos;

		//Copio bytes primera pagina
		if (bytesACopiar < bytesPrimeraPagina) { //

			memcpy(
					obtenerPosicionMemoriaPagina(paginaInicialPosta)
							+ ((direccion - unSegmento->baseLogica)
									% pconfig->tamanio_pag), src, bytesACopiar);
			bytesACopiar -= bytesACopiar;

		} else {

			pos = obtenerPosicionMemoriaPagina(paginaInicialPosta)
					+ ((direccion - unSegmento->baseLogica)
							% pconfig->tamanio_pag);
			memcpy(pos, src, bytesPrimeraPagina);
			bytesACopiar -= bytesPrimeraPagina;

		}

		frameInicial->uso = 1;
		frameInicial->modificado = 1;

		int indiceProximaPagina = indicePaginaInicialPosta + 1;
		struct Pagina *proximaPagina;
		struct Frame *proximoFrame;

		while (bytesACopiar > 0) {
			proximaPagina = list_get(unSegmento->tablaPaginas,
					indiceProximaPagina);
			proximoFrame = list_get(bitmapFrames, proximaPagina->numeroFrame);

			if (bytesACopiar >= tam_pagina) {

				memcpy(obtenerPosicionMemoriaPagina(proximaPagina),
						src + (n - bytesACopiar), pconfig->tamanio_pag);
				bytesACopiar -= pconfig->tamanio_pag;

			} else {

				memcpy(obtenerPosicionMemoriaPagina(proximaPagina),
						src + (n - bytesACopiar), bytesACopiar);
				bytesACopiar -= bytesACopiar;

			}

			proximoFrame->uso = 1;
			proximoFrame->modificado = 1;
			proximaPagina++;
		}

		//Test

		void* deDondeLeer;
		deDondeLeer = obtenerPosicionMemoriaPagina(
				list_get(unSegmento->tablaPaginas, 0))
				+ sizeof(struct HeapMetadata);
		printf("El numero copiado es %i \n", *((int*) deDondeLeer));

		//printf("El resultado es EL CASTEADO %i \n", (int)deDondeLeer);

		//printf("La frase copiada es %i \n", resultado);

		return 0; //Copio correctamente

	} else { //Es un segmento mmap

		traerPaginasSegmentoMmapAMemoria((direccion - unSegmento->baseLogica),
				n, unSegmento);

		//Empiezo a copiar los datos en las paginas
		int indicePrimeraPagina = (direccion - unSegmento->baseLogica)
				/ pconfig->tamanio_pag;
		struct Pagina *pagina = list_get(unSegmento->tablaPaginas,
				indicePrimeraPagina);
		int bytesPrimeraPagina = pconfig->tamanio_pag
				- ((direccion - unSegmento->baseLogica) % pconfig->tamanio_pag);
		int bytesACopiar = n;
		struct Frame *framePrimeraPagina = list_get(bitmapFrames,
				pagina->numeroFrame);
		framePrimeraPagina->uso = 1;
		framePrimeraPagina->modificado = 1;

		if (indicePrimeraPagina * pconfig->tamanio_pag
				+ ((direccion - unSegmento->baseLogica) % pconfig->tamanio_pag)
				+ n > unSegmento->tamanio) {
			return -1;
		}

		//Copio bytes primera pagina
		if (bytesACopiar < bytesPrimeraPagina) {
			memcpy(
					obtenerPosicionMemoriaPagina(pagina)
							+ ((direccion - unSegmento->baseLogica)
									% pconfig->tamanio_pag), src, bytesACopiar);
			bytesACopiar -= bytesACopiar;
		} else {
			memcpy(
					obtenerPosicionMemoriaPagina(pagina)
							+ ((direccion - unSegmento->baseLogica)
									% pconfig->tamanio_pag), src,
					bytesPrimeraPagina);
			bytesACopiar -= bytesPrimeraPagina;
		}

		int proximaPag = indicePrimeraPagina + 1;
		struct Pagina *proximaPagina;
		struct Frame *proximoFrame;

		while (bytesACopiar > 0) {
			proximaPagina = list_get(unSegmento->tablaPaginas, proximaPag);
			proximoFrame = list_get(bitmapFrames, proximaPagina->numeroFrame);

			if (bytesACopiar >= tam_pagina) {

				memcpy(obtenerPosicionMemoriaPagina(proximaPagina),
						src + (n - bytesACopiar), pconfig->tamanio_pag);
				bytesACopiar -= pconfig->tamanio_pag;

			} else {

				memcpy(obtenerPosicionMemoriaPagina(proximaPagina),
						src + (n - bytesACopiar), bytesACopiar);
				bytesACopiar -= bytesACopiar;

			}

			proximoFrame->uso = 1;
			proximoFrame->modificado = 1;
			proximaPagina++;
		}

		void* deDondeLeer;
		deDondeLeer = obtenerPosicionMemoriaPagina(
				list_get(unSegmento->tablaPaginas, 0))
				+ sizeof(struct HeapMetadata);
		printf("El resultado es %i \n", (int) deDondeLeer);

		return 0;
	}

}

//MUSE FREE

int musefree(int idSocketCliente, uint32_t dir) {

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	if (segmentosProceso == NULL) {
		return -1;
	}

	//busco el segmento especifico que contiene la direccion
	struct Segmento *segmento;
	segmento = segmentoQueContieneDireccion(segmentosProceso, dir);

	if (segmento == NULL) { //No existe el segmento buscado
		return -1;
	}

	if (segmento->esComun == false) { //Existe el segmento pero es un segmento mmap, no se libera con free
		return -1;
	}

	//voy a la pagina de ese segmento donde esta la direccion y recorro metadatas
	struct Pagina *pagina; //= malloc(sizeof(struct Pagina));
	pagina = paginaQueContieneDireccion(segmento, (void*) dir);
	int indicePrimeraPagina = obtenerIndicePagina(segmento->tablaPaginas,
			pagina);

	if (pagina == NULL) {
		return -1;
	}

	int direccionSeg = dir - segmento->baseLogica;

	//retorno la posicion memoria dentro del frame
	void *pos = obtenerPosicionMemoriaPagina(pagina);
	//
	//int desplazamientoPagina = direccionSeg % pconfig->tamanio_pag;

	struct HeapMetadata *metadataBuscada = malloc(sizeof(struct HeapMetadata));
	metadataBuscada->size = -1;

	struct HeapMetadata *nuevaMetadata = malloc(sizeof(struct HeapMetadata));
	nuevaMetadata->size = -1;

	struct HeapLista *heapLista = malloc(sizeof(struct HeapLista));
	struct HeapLista *heapBuscada = malloc(sizeof(struct HeapLista)); //malloc innecesario?

	int desplazamientoMetadata;
	int indicePrimeraPaginaMetadata;
	int indicePagina;
	//struct Pagina *paginaActual;
	struct Pagina *segundaPagina;

	for (int i = 0; i < list_size(segmento->metadatas); i++) {
		heapLista = list_get(segmento->metadatas, i);
		desplazamientoMetadata = heapLista->direccionHeap
				% pconfig->tamanio_pag;
		indicePagina = (floor)(
				(double) heapLista->direccionHeap
						/ (double) pconfig->tamanio_pag);

		if ((indicePagina * pconfig->tamanio_pag) + desplazamientoMetadata
				+ sizeof(struct HeapMetadata) == direccionSeg) { //Es la metadata que busco liberar

			heapBuscada = heapLista;

			//la modifico (libero) y la reemplazo
			//Modifico el heap buscado asociado a la metadata
			heapBuscada->isFree = true;

			nuevaMetadata->isFree = true;
			nuevaMetadata->size = heapBuscada->size;

			//Me posiciono donde arranca la metadata y la copio modificada
			indicePrimeraPaginaMetadata = indicePrimeraPagina - 1; //indiceprimerapagina es donde arranca el malloc y no la metadata
			pagina = list_get(segmento->tablaPaginas,
					indicePrimeraPaginaMetadata);
			int posPagina = (int) (obtenerPosicionMemoriaPagina(pagina));
			pos = obtenerPosicionMemoriaPagina(pagina) + desplazamientoMetadata;

			if (heapBuscada->estaPartido == false) {
				memcpy(pos, nuevaMetadata, sizeof(struct HeapMetadata));
			} else {
				segundaPagina = list_get(segmento->tablaPaginas,
						indicePrimeraPaginaMetadata + 1);
				memcpy(pos, nuevaMetadata, heapBuscada->bytesPrimeraPagina);
				//Me paro en la siguiente pagina - frame
				pos = obtenerPosicionMemoriaPagina(segundaPagina);
				//memcpy(pos, nuevaMetadata + (sizeof(struct HeapMetadata) - heapBuscada->bytesPrimeraPagina), (sizeof(struct HeapMetadata) - heapBuscada->bytesPrimeraPagina));

				int dire = (int) (nuevaMetadata)
						+ heapBuscada->bytesPrimeraPagina;

				memcpy(pos, nuevaMetadata + heapBuscada->bytesPrimeraPagina,
						(sizeof(struct HeapMetadata)
								- heapBuscada->bytesPrimeraPagina));
			}

			break;

		}

	}

	if (heapBuscada->size == -1) { //No se encontro la metadata requerida
		return -1;
	}

	//una vez que realizo el free unifico headers
	//segmento = unificarHeaders(segmento->id,idSocketCliente);
	//segmento = eliminarPaginasLibresSegmento(idSocketCliente, segmento->id);

	//Prueba free
	void *posPagina = obtenerPosicionMemoriaPagina(pagina);
	void *posMetadata = posPagina + desplazamientoMetadata;
	void * buffer = malloc(sizeof(struct HeapMetadata));
	memcpy(buffer, posMetadata, sizeof(struct HeapMetadata));

	struct HeapMetadata *meta = malloc(sizeof(struct HeapMetadata));
	meta = (struct HeapMetadata *) buffer;

	//free(stringIdSocketCliente);
	//free(segmento);
	//free(pagina);
	return 0;
}

struct Segmento *segmentoQueContieneDireccion(t_list* listaSegmentos,
		uint32_t direccion) {

	struct Segmento *unSegmento;

	for (int i = 0; i < list_size(listaSegmentos); i++) {
		unSegmento = list_get(listaSegmentos, i);

		if ((direccion - unSegmento->baseLogica) < unSegmento->tamanio) {
			return unSegmento;
		}

		//return unSegmento;
	}

	//Si sale del for sin retorno, no hay ningun segmento que contenga esa direc
	return NULL; //error
}

/*Modifica el segmento luego de unificar headers, para que, en el caso de haber quedado paginas
 *enteras vacias (con t odo free), eliminarlas y modificar el tamaño del segmento*/

struct Segmento *eliminarPaginasLibresSegmento(int idSocketCliente,
		int idSegmento) {
	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	if (segmentosProceso == NULL) { //No deberia pasar porque ya se chequeo en la funcion de donde viene
		return NULL;
	}

	struct Segmento *segmento = list_get(segmentosProceso, idSegmento);

	if (segmento == NULL) { //No deberia pasar
		return NULL;
	}

	int paginasEliminadas = 0;
	int pag = list_size(segmento->tablaPaginas) - 1;
	struct Pagina *pagina;
	t_list *metadatas = list_create();
	struct Frame *frame; //= malloc(sizeof(struct Frame));
	int indiceFrame;

	while (pag >= 0) { //Recorre las paginas de la mas grande a la mas chica

		pagina = list_get(segmento->tablaPaginas, pag);

		if (hayAlgunaMetadataOcupada(segmento, pag) == true) {
			//Si hay alguna metadata ocupada, no se elimina y se corta
			break;
		} else {
			paginasEliminadas++; //Incremento lo que tengo que borrar de atras para adelante
			pag--;
		}

	}

	segmento->paginasLiberadas += paginasEliminadas;
	segmento->tamanio -= paginasEliminadas * pconfig->tamanio_pag;

	//Desde atras, tengo que eliminar las paginas
	int paginaAEliminar = list_size(segmento->tablaPaginas) - 1;

	while (paginaAEliminar > 0) {
		list_remove(segmento->tablaPaginas, paginaAEliminar);

		paginaAEliminar--;
	}

	return segmento;
}

bool hayAlgunaMetadataOcupada(struct Segmento *segmento, int indicePagina) {

	struct HeapLista *heap;
	int indicePaginaHeap;

	for (int i = 0; i < list_size(segmento->metadatas); i++) {
		heap = list_get(segmento->metadatas, i);
		indicePaginaHeap = (floor)(
				(double) heap->direccionHeap / (double) pconfig->tamanio_pag);

		if (indicePaginaHeap == indicePagina && heap->isFree == false) {
			return true;
		}
	}

	return false;
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
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

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
	//dictionary_put(tablasSegmentos, stringIdSocketCliente, segmentosProceso);

	return 0;
}

/*Trae a memoria principal una pagina de un segmento de un proceso en particular
 *y retorna el numero de frame en el que la ubico*/
int traerAMemoriaPrincipal(int indicePagina, int indiceSegmento,
		int idSocketCliente) {
	//Obtengo pagina swapeada
	struct Pagina *paginaSwapeada = malloc(sizeof(struct Pagina));
	struct Segmento *segmentoQueContienePagina = malloc(
			sizeof(struct Segmento));

	char *stringIdSocketCliente = string_itoa(idSocketCliente);
	t_list *segmentosProceso = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	char *datosEnSwap = malloc(sizeof(struct Pagina));

	segmentoQueContienePagina = list_get(segmentosProceso, indiceSegmento);
	paginaSwapeada = list_get(segmentoQueContienePagina->tablaPaginas,
			indicePagina);

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

	struct Frame *nuevoFrame; //= malloc(sizeof(struct Frame));

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

	//Modifico las estructuras ----> XXX: No hace falta, son punteros, modificas 1 y modificas los demas
	/*list_replace(bitmapFrames, frameReemplazo, nuevoFrame);
	 list_replace(segmentoQueContienePagina->tablaPaginas, indicePagina,
	 paginaSwapeada);
	 list_replace(segmentosProceso, indiceSegmento, segmentoQueContienePagina);
	 dictionary_put(tablasSegmentos, stringIdSocketCliente, segmentosProceso);*/

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
int llevarASwapUnaPagina(struct Pagina *paginaASwappear) {

	char *datosASwappear = malloc(sizeof(struct Pagina));

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

	struct Frame *frameModificado; //= malloc(sizeof(struct Frame));
	frameModificado = list_get(bitmapFrames, frameQueContieneData);
	frameModificado->modificado = 0;
	frameModificado->uso = 0;
	frameModificado->estaLibre = true;

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
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

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
	t_list *listaSegmentos = dictionary_get(tablasSegmentos,
			stringIdSocketCliente);

	//Obtencion segmento
	int idSegmento;

	//Obtencion idSegmento y segmento
	struct Segmento *unSegmento; //= malloc(sizeof(struct Segmento));
	idSegmento = idSegmentoQueContieneDireccion(listaSegmentos, (void*) dir);
	unSegmento = list_get(listaSegmentos, idSegmento);

	if (unSegmento->esComun == true) { //Error

		return -1;

	} else {

		//Borro el segmento entero y actualizo el diccionario
		list_remove(listaSegmentos, idSegmento); //XXX aca haria falta un remove and destroy
		/*dictionary_put(tablasSegmentos, (char*) idSocketCliente,
		 listaSegmentos);*/
		return 0;

	}

	return -1;
}
