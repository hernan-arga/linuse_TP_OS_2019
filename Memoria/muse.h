#ifndef MUSE_H_
#define MUSE_H_

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

void* memoriaPrincipal;
t_dictionary* tablasSegmentos; /*Diccionario que contiene las tablas de segmentos por
								*proceso, la key es el pid de cada proceso*/
int tam_mem;
int tam_pagina;
int tam_swap;

FILE *swap;

int cantidadFrames;
size_t cantidadPaginasSwap;
int punteroClock;

//t_bitarray *bitmapFrames;
t_list *bitmapFrames; //Va a ser una t_list de struct Frame, el INDICE es el numero de frame

t_bitarray *bitmapSwap;

struct HeapMetadata {

	uint32_t size;
	bool isFree;

};

struct Segmento{

	bool esComun;
	char *filePath; //NULL si es un segmento normal
	int id;
	uint32_t baseLogica;
	uint32_t tamanio;
	t_list* tablaPaginas;

};

struct Pagina{

	int numeroFrame;
	int indiceSwap;

};

struct Frame{

	int listaMetadata[50]; //ver de hacer lista int
	int modificado;
	int uso;
	int presencia;

};


///////////////Funciones//////////////////
void reservarMemoriaPrincipal(int tamanio);
void crearTablaSegmentos();
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
bool hayFramesLibres();

//Funciones init
int museinit(int idSocketCliente);
bool hayMemoriaDisponible();

//Funciones malloc
void *musemalloc(uint32_t tamanio, int idSocketCliente);
struct Segmento *crearSegmento(uint32_t tamanio, int idSocketCliente);
int poseeTamanioLibre(struct Segmento *unSegmento, uint32_t tamanio);
void *asignarEspacioLibre(struct Segmento *unSegmento, uint32_t tamanio);
bool esExtendible(t_list *segmentosProceso, int unIndice);
struct Segmento *asignarPrimeraPaginaSegmento(struct Segmento *unSegmento, int tamanio);
struct Segmento *asignarUltimaPaginaSegmento(struct Segmento *unSegmento, int tamanio);
struct Segmento *asignarNuevaPagina(struct Segmento *unSegmento, int tamanio);
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
int min(int num1, int num2);

//Funciones free
int musefree(int idSocketCliente, uint32_t dir);
struct Segmento *segmentoQueContieneDireccion(t_list* listaSegmentos, void *direccion);

//Memoria virtual
int clockModificado();
void incrementarPunteroClockModificado();
void inicializarBitmapSwap();
uint32_t musemap(char *path, size_t length/*, int flags*/);
int traerAMemoriaPrincipal(int indicePagina, int indiceSegmento, int idSocketCliente);
void cargarDatosEnFrame(int indiceFrame, char *datos);
int llevarASwapUnaPagina(int indicePagina, int indiceSegmento, int idSocketCliente);
int buscarIndiceSwapLibre();
int musesync(uint32_t addr, size_t len, int idSocketCliente);
int obtenerIndicePagina(t_list *listaPaginas, struct Pagina *pagina);
void *obtenerDatosActualizados(int frame, int desplazamiento, size_t len, struct Segmento *unSegmento, struct Pagina *unaPagina);
int muse_unmap(uint32_t dir, int idSocketCliente);
int asignarUnFrame();

#endif /* MUSE_H_ */

