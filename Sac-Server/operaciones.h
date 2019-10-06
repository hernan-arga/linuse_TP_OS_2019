#ifndef OPERACIONES_H_
#define OPERACIONES_H_
#include <dirent.h>
#include <sys/stat.h>

int o_create(char*);
int o_open(char*);
void o_read(char*, int, int, char*);
void o_readDir(char*, int);
void o_getAttr(char*, int);

#endif /* OPERACIONES_H_ */
