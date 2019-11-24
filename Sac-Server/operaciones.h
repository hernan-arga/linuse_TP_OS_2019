#ifndef OPERACIONES_H_
#define OPERACIONES_H_
#include "estructuras.h"


int o_create(char*);
int o_open(char*);
int o_read(char*, int, int, char*);
int o_readDir(char*, int);
void o_getAttr(char*, int);
int o_mkdir(char*);
int o_unlink(char*);
int o_rmdir(char*);
int o_rmdir_2(char*);
void o_write(char*, int, int, char*);
ptrGBloque buscarPadre(char*);

#endif /* OPERACIONES_H_ */
