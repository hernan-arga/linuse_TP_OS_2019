/*
 * utils.h
 *
 *  Created on: 8 sep. 2019
 *      Author: utnso
 */



#ifndef SUSE_UTILS_H_
#define SUSE_UTILS_H_


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


#endif /* SUSE_UTILS_H_ */
