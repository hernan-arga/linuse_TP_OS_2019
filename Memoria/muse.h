#ifndef MUSE_H_
#define MUSE_H_
#include <stdint.h>
#include<commons/collections/list.h>

void* memoriaPrincipal;
t_list* tablaSegmentos;
int tam_mem;
int tam_pagina;

struct HeapMetadata { //Heapmetada es por MALLOC y no por segmento
	uint32_t size;
	bool isFree;
};

struct Segmento{
	int id;
	t_list* paginas;
};

struct Pagina{
	//bool modificado;
	//bool uso;
	//bool presencia;
	//int numeroFrame;
	int data;
};


///////////////Funciones//////////////////
void reservarMemoriaPrincipal(int tamanio);
void crearTablaSegmentos();
void *musemalloc(uint32_t tamanio);
int atenderMuseInit(int sd);
int atenderMuseClose(int sd); //Verificar retorno
uint32_t atenderMuseAlloc(int sd);
int atenderMuseFree(int sd);
int atenderMuseGet(int sd);
int atenderMuseCopy(int sd);
uint32_t atenderMuseMap(int sd);
int atenderMuseSync(int sd);
int atenderMuseUnmap(int sd);
void *crearSegmentoInicial(uint32_t tamanio);
void crearTablaPaginas(struct Segmento segmento);
void *buscarEspacioLibre(uint32_t tamanio);
void unificarHeaders(int id);
void *buscarPosicionSegmento(int idSegmento);
int calcularTamanioSegmento(int id);
int espacioPaginas(int idSegmento);

#endif /* MUSE_H_ */
