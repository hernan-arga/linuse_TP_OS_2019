#ifndef SUSE_UTILS_H_
#define SUSE_UTILS_H_
#define EXIT_FAILURE 1

#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<netdb.h>
#include<unistd.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/log.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "suse_commons.h"

int iniciar_conexion(int);

#endif /* SUSE_UTILS_H_ */




