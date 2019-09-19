#ifndef MUSE_H_
#define MUSE_H_
#include <stdint.h>
#include<commons/collections/list.h>

char* memoriaPrincipal;

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
	char* data;
} pagina;

#endif /* MUSE_H_ */
