#include "sacCli.h"

#include <stdio.h>
#include <fuse.h>
#include <string.h>
#include "conexionCli.h"

 #define DEFAULT_FILE_CONTENT "Hello World!\n"
/*
 * Este es el nombre del archivo que se va a encontrar dentro de nuestro FS
 */
// #define DEFAULT_FILE_NAME "hello"
/*
 * Este es el path de nuestro, relativo al punto de montaje, archivo dentro del FS
 */
 #define DEFAULT_FILE_PATH "/"
//DEFAULT_FILE_NAME
// gcc -DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF "sacCli.d" -MT "sacCli.d" -o "sacCli.o" "sacCli.c"



static int hello_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 1;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));

	send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

	// Deserializo respuesta
	int* tamanioRespuesta = malloc(sizeof(int));
	read(sacServer, tamanioRespuesta, sizeof(int));
	int* ok = malloc(*tamanioRespuesta);
	read(sacServer, ok, *tamanioRespuesta);
	if (*ok == 1) {
		return 0;
	}
	else{
		return errno;
	}
}


static int hello_open(const char *path, struct fuse_file_info *fi) {

	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 2;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));

	send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	//Serializo peticion, path, size y offset
	char* buffer = malloc(7 * sizeof(int) + strlen(path));

	int peticion = 3;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));

	int sizeInt = (int) size;
	int tamanioSize = sizeof(int);
	memcpy(buffer + 3 * sizeof(int) + strlen(path), &tamanioSize, sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + strlen(path), &sizeInt, sizeof(int));

	int offsetInt = (int) offset;
	int tamanioOffset = sizeof(int);
	memcpy(buffer + 5 * sizeof(int) + strlen(path), &tamanioOffset, sizeof(int));
	memcpy(buffer + 6 * sizeof(int) + strlen(path), &offsetInt, sizeof(int));

	send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

	//Deserializo el texto del archivo que te manda SacCli y lo guardo en buf
	int *tamanioTexto = malloc(sizeof(int));
	read(sacServer, tamanioTexto, sizeof(int));
	char *texto = malloc(*tamanioTexto);
	read(sacServer, texto, *tamanioTexto);

	memcpy(buf, texto, strlen(texto));

	return strlen(texto);
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ){
	if(strcmp(path, "/")== 0){
		//Serializo peticion y path
		char* buffer = malloc(3 * sizeof(int) + strlen(path));

		int peticion = 4;
		int tamanioPeticion = sizeof(int);
		memcpy(buffer, &tamanioPeticion, sizeof(int));
		memcpy(buffer + sizeof(int), &peticion, sizeof(int));

		int tamanioPath = strlen(path);
		memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
		memcpy(buffer + 3 * sizeof(int), path, strlen(path));

		send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

		//Deserializo los directorios concatenados que te manda SacCli
		int *tamanioTexto = malloc(sizeof(int));
		read(sacServer, tamanioTexto, sizeof(int));
		char *texto = malloc(*tamanioTexto);
		read(sacServer, texto, *tamanioTexto);
		char *textoCortado = string_substring_until(texto, *tamanioTexto);

		//Hago un filler de cada uno de esos nombres, serparandolos por ;
		char** arrayNombres = string_split(textoCortado, ";");
		int i = 0;
		while(arrayNombres[i] != NULL){
			filler(buf, arrayNombres[i], NULL, 0);
			i++;
		}
	}
	return 0;
}

static int hello_getattr(const char *path, struct stat *stbuf) {

	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 5;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));

	send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

	// Deserializo respuesta
	int* tamanioRespuesta = malloc(sizeof(int));
	read(sacServer, tamanioRespuesta, sizeof(int));
	int* ok = malloc(*tamanioRespuesta);
	read(sacServer, ok, *tamanioRespuesta);
	if (*ok == 1) {
		//Deserializo mode y nlink
		int* tamanioMode = malloc(sizeof(int));
		read(sacServer, tamanioMode, sizeof(int));
		void* mode = malloc(*tamanioMode);
		read(sacServer, mode, *tamanioMode);

		int* tamanioNlink = malloc(sizeof(int));
		read(sacServer, tamanioNlink, sizeof(int));
		int* nlink = malloc(*tamanioNlink);
		read(sacServer, nlink, *tamanioNlink);

		memcpy(&stbuf->st_mode, mode, *tamanioMode);
		memcpy(&stbuf->st_nlink, nlink, *tamanioNlink);

		return 0;
	}
	if (*ok == 0) {
		return -ENOENT;
	}


	/*
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	 return -ENOENT;
	 */

}


static int hello_write(const char *path, const char *buf, size_t size, off_t offset,  struct fuse_file_info *fi)
{
    int fd;
    int res;

    fd = open(path, fi->flags);
    if(fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if(res == -1)
        res = -errno;

    close(fd);
    return res;
}


static struct fuse_operations hello_oper = {
		.create = hello_create,
		.open = hello_open,
		.read = hello_read,
		.readdir = hello_readdir,
		.getattr = hello_getattr
};


struct t_runtime_options {
	char* welcome_msg;
} runtime_options;


enum {
	KEY_VERSION,
	KEY_HELP,
};


static struct fuse_opt fuse_options[] = {

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

//pthread_t threadConexion;

int main(int argc, char *argv[]){

	conectarseASacServer();

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1) {
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --welcome-msg
	// el campo welcome_msg deberia tener el
	// valor pasado
	if (runtime_options.welcome_msg != NULL) {
		printf("%s\n", runtime_options.welcome_msg);
	}

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &hello_oper, NULL);

	// Levanta archivo de configuracion
	config* pconfig = malloc(2 * sizeof(int));
	levantarConfigFile(pconfig);

	return 0;
}
