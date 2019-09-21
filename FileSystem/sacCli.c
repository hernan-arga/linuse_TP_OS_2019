#include "sacCli.h"

#include <stdio.h>
#include "utils.h"
#include <fuse.h>

#define DEFAULT_FILE_CONTENT "Hello World!\n"

/*
 * Este es el nombre del archivo que se va a encontrar dentro de nuestro FS
 */
#define DEFAULT_FILE_NAME "hello"

/*
 * Este es el path de nuestro, relativo al punto de montaje, archivo dentro del FS
 */
#define DEFAULT_FILE_PATH "/" DEFAULT_FILE_NAME

// gcc -DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF "sacCli.d" -MT "sacCli.d" -o "sacCli.o" "sacCli.c"

static int hello_open(const char *path, struct fuse_file_info *fi) {
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int hello_getattr(const char *path, struct stat *stbuf) {
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, DEFAULT_FILE_PATH) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(DEFAULT_FILE_CONTENT);
	} else {
		res = -ENOENT;
	}
	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, DEFAULT_FILE_NAME, NULL, 0);

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	size_t len;
	(void) fi;
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;

	len = strlen(DEFAULT_FILE_CONTENT);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, DEFAULT_FILE_CONTENT + offset, size);
	} else
		size = 0;

	return size;
}

static int hello_write(const char *path, const char *buf, size_t size, off_t offset)
{
    int fd;
    int res;

    fd = open(path, O_WRONLY);
    if(fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if(res == -1)
        res = -errno;

    close(fd);
    return res;
}

static struct fuse_operations hello_oper = {
		.open = hello_open,
		.readdir = hello_readdir,
		.getattr = hello_getattr,
		.read = hello_read,
		.write = hello_write
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



int main(int argc, char *argv[]){

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

		// Limpio la estructura que va a contener los parametros
		memset(&runtime_options, 0, sizeof(struct t_runtime_options));

		// Esta funcion de FUSE lee los parametros recibidos y los intepreta
		if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
			/** error parsing options */
			perror("Invalid arguments!");
			return EXIT_FAILURE;
		}

		// Si se paso el parametro --welcome-msg
		// el campo welcome_msg deberia tener el
		// valor pasado
		if( runtime_options.welcome_msg != NULL ){
			printf("%s\n", runtime_options.welcome_msg);
		}

		// Esta es la funcion principal de FUSE, es la que se encarga
		// de realizar el montaje, comuniscarse con el kernel, delegar todo
		// en varios threads
		return fuse_main(args.argc, args.argv, &hello_oper, NULL);



	/*
	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(2 * sizeof(int));
	levantarConfigFile(pconfig);

	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "FUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);

	return 0;
	*/
}
