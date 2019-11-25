#ifndef MUSE_H_
#define MUSE_H_

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

void* memoriaPrincipal;
t_dictionary* tablasSegmentos; /*Diccionario que contiene las tablas de segmentos por
								*proceso, la key es el pid de cada proceso*/
int tam_mem;
int tam_pagina;
int tam_swap;

FILE *swap;

int cantidadFrames;
//t_bitarray *bitmapFrames;
t_list *bitmapFrames; //Va a ser una t_list de struct Frame, el INDICE es el numero de frame

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
	//bool modificado;
	//bool uso;
	//int presencia;
	int numeroFrame;
	//struct Frame frame; ???
};

struct Frame{
	bool modificado;
	bool uso;
	int presencia;
};


///////////////Funciones//////////////////
void reservarMemoriaPrincipal(int tamanio);
void crearTablaSegmentos();
void *crearHeaderInicial(uint32_t tamanio);
void *buscarEspacioLibre(uint32_t tamanio);
void unificarHeaders(int idSocketCliente, int idSegmento);
void *buscarPosicionSegmento(int idSocketCliente, int idSegmento);
int calcularTamanioSegmento(char *idProceso, int idSegmento);
int espacioPaginas(int idProceso, int idSegmento);
void crearTablaSegmentosProceso(int idProceso);
void *posicionMemoriaUnSegmento(struct Segmento *unSegmento);
uint32_t obtenerTamanioSegmento(int idSegmento, int idSocketCliente);

//Funciones bitmap de frames
void crearBitmapFrames();
void inicializarBitmapFrames();
void ocuparFrame(int indiceFrame);
void liberarFrame(int indiceFrame);
int buscarFrameLibre();
void *retornarPosicionMemoriaFrame(int unFrame);
bool estaOcupadoCompleto(int indiceFrame);
bool frameEstaLibre(int indice);

//Funciones init
int museinit(int idSocketCliente);
bool hayMemoriaDisponible();

//Funciones malloc
void *musemalloc(uint32_t tamanio, int idSocketCliente);
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente);
int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio);
void *asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio);
bool esExtendible(t_list *segmentosProceso, int unIndice);
void asignarNuevaPagina(struct Segmento *unSegmento, uint32_t tamanio);
uint32_t obtenerTamanioSegmento(int idSegmento, int idSocketCliente);
void *buscarEspacioLibreProceso(int idSocketCliente, uint32_t tamanio);
void asignarTamanioADireccion(uint32_t tamanio, void* src, int idSocketCliente); //A TERMINAR

//Funciones cpy
int musecpy(uint32_t dst, void* src, int n, int idSocket);
int idSegmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion);
struct Pagina *paginaQueContieneDireccion(struct Segmento *unSegmento, void *direccion);
struct Segmento *segmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion);

//Funciones get
int museget(void* dst, uint32_t src, size_t n, int idSocket);

//Funciones free
int musefree(int idSocketCliente, uint32_t dir);
struct Segmento *segmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion);

//Memoria virtual
int clockModificado();

#endif /* MUSE_H_ */

