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

void loguearInfo(char* texto) {
	char* mensajeALogear = malloc( strlen(texto) + 1);
	strcpy(mensajeALogear, texto);
	t_log* g_logger;
	g_logger = log_create("./sacCli.log", "SacCli", 1, LOG_LEVEL_INFO);
	log_info(g_logger, mensajeALogear);
	log_destroy(g_logger);
	free(mensajeALogear);
}


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
	if (*ok == 0) {
		return 0;
	}
	else{
		return errno;
	}
}


static int hello_open(const char *path, struct fuse_file_info *fi) {

	return 0;

	/*
 * FUNCIONAMIENTO ANTERIOR
 *
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
*/
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

	send(sacServer, buffer, 7 * sizeof(int) + strlen(path), 0);


	int *tamanioTexto = malloc(sizeof(int));
	read(sacServer, tamanioTexto, sizeof(int));
	if (*tamanioTexto == 0) {
		return 0;
	} else {
		char *texto = malloc(size);
		read(sacServer, texto, size);

		memcpy(buf, texto, size);
		free(texto);
		return size;
	}
	free(buffer);
}


	// antes hacia return *tamanioTexto;



static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ){
	//if(strcmp(path, "/")== 0){
		//Serializo peticion y path
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		char* buffer = malloc(3 * sizeof(int) + strlen(path));

		int peticion = 4;
		int tamanioPeticion = sizeof(int);
		memcpy(buffer, &tamanioPeticion, sizeof(int));
		memcpy(buffer + sizeof(int), &peticion, sizeof(int));

		int tamanioPath = strlen(path);
		memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
		memcpy(buffer + 3 * sizeof(int), path, strlen(path));

		send(sacServer, buffer, 3 * sizeof(int) + strlen(path), 0);

		// Deserializo 0 ok, ó -1 error
		int* tamanioRespuesta = malloc(sizeof(int));
		read(sacServer, tamanioRespuesta, sizeof(int));
		int* res = malloc(*tamanioRespuesta);
		read(sacServer, res, *tamanioRespuesta);
		if (*res == 0) {
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
				return 0;
		}
		else {
			return -ENOENT;
		}
}

static int hello_getattr(const char *path, struct stat *stbuf) {

	if (strcmp(path, "/") == 0){
			stbuf->st_mode = S_IFDIR | 0777;
			stbuf->st_nlink = 2;
			return 0;
	}

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
	int* res = malloc(*tamanioRespuesta);
	read(sacServer, res, *tamanioRespuesta);
	if (*res == 1) {
		//Deserializo mode, nlink y size
		int* tamanioMode = malloc(sizeof(int));
		read(sacServer, tamanioMode, sizeof(int));
		void* mode = malloc(*tamanioMode);
		read(sacServer, mode, *tamanioMode);

		int* tamanioNlink = malloc(sizeof(int));
		read(sacServer, tamanioNlink, sizeof(int));
		int* nlink = malloc(*tamanioNlink);
		read(sacServer, nlink, *tamanioNlink);

		int* tamanioEscrito = malloc(sizeof(int));
		read(sacServer, tamanioEscrito, sizeof(int));
		int* bytesEscritos = malloc(*tamanioEscrito);
		read(sacServer, bytesEscritos, *tamanioEscrito);

		memcpy(&stbuf->st_mode, mode, *tamanioMode);
		memcpy(&stbuf->st_nlink, nlink, *tamanioNlink);
		memcpy(&stbuf->st_size, bytesEscritos, *tamanioEscrito);

		return 0;
	}
	else {
		if(*res == 0){
			//Deserializo mode, nlink y size
			int* tamanioMode = malloc(sizeof(int));
			read(sacServer, tamanioMode, sizeof(int));
			void* mode = malloc(*tamanioMode);
			read(sacServer, mode, *tamanioMode);

			int* tamanioNlink = malloc(sizeof(int));
			read(sacServer, tamanioNlink, sizeof(int));
			int* nlink = malloc(*tamanioNlink);
			read(sacServer, nlink, *tamanioNlink);

			int* tamanioEscrito = malloc(sizeof(int));
			read(sacServer, tamanioEscrito, sizeof(int));
			int* bytesEscritos = malloc(*tamanioEscrito);
			read(sacServer, bytesEscritos, *tamanioEscrito);

			int* tamanioModif = malloc(sizeof(int));
			read(sacServer, tamanioModif, sizeof(int));
			void* modificacion = malloc(*tamanioModif);
			read(sacServer, modificacion, *tamanioModif);

			int* tamanioCreacion = malloc(sizeof(int));
			read(sacServer, tamanioCreacion, sizeof(int));
			void* creacion = malloc(*tamanioCreacion);
			read(sacServer, creacion, *tamanioCreacion);

			int* tamanioAcceso = malloc(sizeof(int));
			read(sacServer, tamanioAcceso, sizeof(int));
			void* acceso = malloc(*tamanioAcceso);
			read(sacServer, acceso, *tamanioAcceso);

			memcpy(&stbuf->st_mode, mode, *tamanioMode);
			memcpy(&stbuf->st_nlink, nlink, *tamanioNlink);
			memcpy(&stbuf->st_size, bytesEscritos, *tamanioEscrito);
			memcpy(&stbuf->st_mtime, modificacion, *tamanioModif);
			memcpy(&stbuf->st_ctime, creacion, *tamanioCreacion);
			memcpy(&stbuf->st_atime, acceso, *tamanioAcceso);

			return 0;
		}
		else{
			return -ENOENT;
		}
	}
}

static int hello_mkdir(const char *path, mode_t mode)
{
	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 6;
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
	int* res = malloc(*tamanioRespuesta);
	read(sacServer, res, *tamanioRespuesta);
	if (*res == 0) {
		return 0;
	}
	else{
		return errno;
	}
}

static int hello_unlink(const char *path)
{
	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 7;
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
	if (*ok == 0) {
		return 0;
	} else {
		return errno;
	}
}

static int hello_rmdir(const char *path)
{
	//Serializo peticion y path
	char* buffer = malloc(3 * sizeof(int) + strlen(path));

	int peticion = 8;
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
	int* resp = malloc(*tamanioRespuesta);
	read(sacServer, resp, *tamanioRespuesta);
	if (*resp == 0) {
		return 0;
	} else {
		return errno;
	}
}

/*
 * 	@DESC
 * 		Funcion que escribe archivos en fuse. Tiene la posta.
 *
 * 	@PARAM
 * 		path - Dir del archivo
 * 		buf - Buffer que indica que datos copiar.
 * 		size - Tam de los datos a copiar
 * 		offset - Situa una posicion sobre la cual empezar a copiar datos
 * 		fi - File Info. Contiene flags y otras cosas locas que no hay que usar
 *
 * 	@RET
 * 		Devuelve la cantidad de bytes escritos, siempre y cuando este OK. Caso contrario, numero negativo tipo -ENOENT.
 */
static int hello_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	//Serializo peticion, path, size, offset y buf
	char* buffer = malloc(8 * sizeof(int) + strlen(path) + size);

	int peticion = 9;
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

	int tamanioBuf = strlen(buf);
	memcpy(buffer + 7 * sizeof(int) + strlen(path), &tamanioBuf, sizeof(int));
	memcpy(buffer + 8 * sizeof(int) + strlen(path), buf, size);

	send(sacServer, buffer, 8 * sizeof(int) + strlen(path) + size, 0);

	// Deserializo respuesta
	int* tamanioRespuesta = malloc(sizeof(int));
	read(sacServer, tamanioRespuesta, sizeof(int));
	int* resp = malloc(*tamanioRespuesta);
	read(sacServer, resp, *tamanioRespuesta);
	if (*resp == -1) {
		return -ENOSPC;
	} else {
		return *resp;
	}


	return 0;
}


static int hello_getxattr(const char *path, const char *name, char *value, size_t size){

	return 0;
}


static int hello_truncate(const char *path, off_t offset){
	//Serializo peticion, path y offset
	char* buffer = malloc(5 * sizeof(int) + strlen(path));

	int peticion = 10;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));

	int offsetInt = (int) offset;
	int tamanioOffset = sizeof(int);
	memcpy(buffer + 3 * sizeof(int) + strlen(path), &tamanioOffset, sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + strlen(path), &offsetInt, sizeof(int));

	send(sacServer, buffer, 5 * sizeof(int) + strlen(path), 0);

	// Deserializo respuesta
	int* tamanioRespuesta = malloc(sizeof(int));
	read(sacServer, tamanioRespuesta, sizeof(int));
	int* resp = malloc(*tamanioRespuesta);
	read(sacServer, resp, *tamanioRespuesta);
	if (*resp == 0) {
		return 0;
	} else {
		return errno;
	}
}



int hello_rename (const char* oldpath, const char* newpath){

	char* buffer = malloc(4 * sizeof(int) + strlen(oldpath) + strlen(newpath));

	int peticion = 11;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath1 = strlen(oldpath);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath1, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), oldpath, strlen(oldpath));

	int tamanioPath2 = strlen(newpath);
	memcpy(buffer + 3 * sizeof(int) + strlen(oldpath), &tamanioPath2, sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + strlen(oldpath), newpath, strlen(newpath));

	send(sacServer, buffer, 4 * sizeof(int) + strlen(oldpath) + strlen(newpath), 0);

	// Deserializo respuesta
	int* tamanioRespuesta = malloc(sizeof(int));
	read(sacServer, tamanioRespuesta, sizeof(int));
	int* resp = malloc(*tamanioRespuesta);
	read(sacServer, resp, *tamanioRespuesta);
	if (*resp == 0) {
		return 0;
	} else {
		return errno;
	}
}


static struct fuse_operations hello_oper = {
		.create = hello_create,
		.open = hello_open,
		.read = hello_read,
		.readdir = hello_readdir,
		.getattr = hello_getattr,
		.mkdir = hello_mkdir,
		.unlink = hello_unlink,
		.rmdir = hello_rmdir,
		.write = hello_write,
		.getxattr = hello_getxattr,
		// chmod, chown, truncate y utime para escribir archivos
		.truncate = hello_truncate,
		.rename = hello_rename
		/*
		.flush = remote_flush,
		.release = remote_release,
		 pueden llegar a faltar
		*/
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
