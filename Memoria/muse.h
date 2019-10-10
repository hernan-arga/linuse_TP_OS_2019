#ifndef MUSE_H_
#define MUSE_H_

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <stdint.h>

void* memoriaPrincipal;
t_dictionary* tablasSegmentos; //Diccionario que contiene las tablas de segmentos por proceso, la key es el pid de cada proceso
int tam_mem;
int tam_pagina;
int cantidadFrames;
int bitmapFrames [/*cantidadFrames*/];

struct HeapMetadata { //Heapmetada es por MALLOC y no por segmento
	uint32_t size;
	bool isFree;
};

struct Segmento{
	int id;
	uint32_t baseLogica;
	uint32_t tamanio;
	t_list* tablaPaginas;
};

struct Pagina{
	bool modificado;
	bool uso;
	bool presencia;
	int numeroFrame;
};


///////////////Funciones//////////////////
void reservarMemoriaPrincipal(int tamanio);
void crearTablaSegmentos();
/* UTILS
int atenderMuseInit(int sd);
int atenderMuseClose(int sd); //Verificar retorno
//uint32_t atenderMuseAlloc(int sd);
//int atenderMuseFree(int sd);
int atenderMuseGet(int sd);
int atenderMuseCopy(int sd);
uint32_t atenderMuseMap(int sd);
int atenderMuseSync(int sd);
int atenderMuseUnmap(int sd);
*/
void *crearHeaderInicial(uint32_t tamanio);
void *buscarEspacioLibre(uint32_t tamanio);
void unificarHeaders(int id);
void *buscarPosicionSegmento(int idSegmento);
int calcularTamanioSegmento(int id);
int espacioPaginas(int idSegmento);
void crearTablaSegmentosProceso(char *idProceso);

//Funciones bitmap de frames
void inicializarBitmapFrames();
int ocuparFrame(int indiceFrame);
void liberarFrame(int indiceFrame);

//Funciones init
int museinit(int id, char* ip/*, int puerto*/);

//Funciones subyacentes malloc
void *musemalloc(uint32_t tamanio, int idSocketCliente);
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente);
int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio);
void asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio);
int esExtendible(t_list *segmentosProceso,int unIndice);
void asignarNuevaPagina(t_list *listaSegmentos, int idSegmento);
void *posicionMemoriaFrame(int unFrame);

#endif /* MUSE_H_ */
