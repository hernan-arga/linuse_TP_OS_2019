#ifndef UTILS_H_
#define UTILS_H_

#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<netdb.h>
#include <unistd.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/log.h>

t_log* logger;

int iniciar_servidor(char *, char *);
int esperar_cliente(int);
t_config* leer_config(void);
t_log * crear_log();

#endif /* UTILS_H_ */
