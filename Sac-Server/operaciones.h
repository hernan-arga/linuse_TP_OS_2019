#ifndef OPERACIONES_H_
#define OPERACIONES_H_
#include "estructuras.h"


int o_create(char*);
int o_open(char*);
int o_read(char*, int, int, char*);
void o_readDir(char*, int);
void o_getAttr(char*, int);
int o_mkdir(char*);
int o_unlink(char*);
int o_rmdir(char*);
int o_rmdir2(char*);
int o_write(char*, int, int, char*);
ptrGBloque buscarPadre(char*);
void eliminarRecursivamente(int);
int o_truncate(char*, int);
int o_rename(char* , char* );


#endif /* OPERACIONES_H_ */
