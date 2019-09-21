#ifndef MUSE_H_
#define MUSE_H_
#include <stdint.h>
#include<commons/collections/list.h>

char* memoriaPrincipal;
t_list* tablaSegmentos;

typedef struct {
	bool uso;
	uint32_t size;
	t_list* paginas;
	//char* data; Ver - segun el mail esta en segmento, yo creo que esta en cada pagina.
} segmento;

typedef struct {
	int numeroPagina;
	bool modificado;
	bool uso;
	bool presencia;
	int numeroFrame;
	char* data;
} pagina;



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


#endif /* MUSE_H_ */
