#ifndef MUSE_H_
#define MUSE_H_

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

void* memoriaPrincipal;
t_dictionary* tablasSegmentos; //Diccionario que contiene las tablas de segmentos por proceso, la key es el pid de cada proceso
int tam_mem;
int tam_pagina;
int cantidadFrames;
t_bitarray *bitmapFrames;

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
	int presencia;
	int numeroFrame;
};


///////////////Funciones//////////////////
void reservarMemoriaPrincipal(int tamanio);
void crearTablaSegmentos();
void *crearHeaderInicial(uint32_t tamanio);
void *buscarEspacioLibre(uint32_t tamanio);
void unificarHeaders(char *idProceso, int idSegmento);
void *buscarPosicionSegmento(char *idProceso, int idSegmento);
int calcularTamanioSegmento(char *idProceso, int idSegmento);
uint32_t espacioPaginas(char *idProceso, int idSegmento);
void crearTablaSegmentosProceso(char *idProceso);
void *posicionMemoriaUnSegmento(struct Segmento *unSegmento);

//Funciones bitmap de frames
void crearBitmapFrames();
void inicializarBitmapFrames();
int ocuparFrame(int indiceFrame, uint32_t tamanio);
void liberarFrame(int indiceFrame);
int buscarFrameLibre();
void *retornarPosicionMemoriaFrame(int unFrame);
bool estaOcupadoCompleto(int indiceFrame);
bool frameEstaLibre(int indice);
int espacioLibreFrame(int indice);

//Funciones init
int museinit(int idSocket);

//Funciones subyacentes malloc
void *musemalloc(uint32_t tamanio, int idSocketCliente);
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente);
int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio);
void *asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio);
int esExtendible(t_list *segmentosProceso,int unIndice);
void asignarNuevaPagina(struct Segmento *unSegmento, uint32_t tamanio);

#endif /* MUSE_H_ */
