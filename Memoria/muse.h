#ifndef MUSE_H_
#define MUSE_H_
#include <stdint.h>
#include<commons/collections/list.h>

void* memoriaPrincipal;
t_list* tablaSegmentos;
int tam_mem;

struct HeapMetadata { //Heapmetada es por MALLOC y no por segmento
	uint32_t size;
	bool isFree;
};

struct Segmento{
	//t_list* paginas;
	char* data;

	//char* data; Ver si esta en segmento o en las paginas (...)
};

struct Pagina{
	int numeroPagina;
	bool modificado;
	bool uso;
	bool presencia;
	int numeroFrame;
	char* data;
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
void unificarHeaders(struct HeapMetadata *header1, struct HeapMetadata *header2);

#endif /* MUSE_H_ */
