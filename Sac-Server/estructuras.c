#include "unistd.h"
#include <stdio.h>
#include "operaciones.h"
#include "estructuras.h"
#include "sac-server.h"

void iniciarMmap() {
	char *pathBitmap = string_new();
	string_append(&pathBitmap, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/");
	string_append(&pathBitmap, "bitmap.bin");
	//Open es igual a fopen pero el FD me lo devuelve como un int (que es lo que necesito para fstat)
	int bitmap = open(pathBitmap, O_RDWR);
	struct stat mystat;

	//fstat rellena un struct con informacion del FD dado
	if (fstat(bitmap, &mystat) < 0) {
		close(bitmap);
	}

	/*	mmap mapea un archivo en memoria y devuelve la direccion de memoria donde esta ese mapeo
	 *  MAP_SHARED Comparte este área con todos los otros  objetos  que  señalan  a  este  objeto.
	 Almacenar  en  la  región  es equivalente a escribir en el fichero.
	 */
	mmapDeBitmap = mmap(NULL, mystat.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, bitmap, 0);
	close(bitmap);
}


t_bitarray* crearBitmap(){

	// revisar cambio de PATH para archivo
	int bitmap = open("/home/utnso/workspace/tp-2019-2c-Cbados/disk.bin", O_RDWR);

	struct stat mystat;

	if (fstat(bitmap, &mystat) < 0) {
	    printf("Error al establecer fstat\n");
	    close(bitmap);
	}

	void * bmap = mmap(NULL, mystat.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, bitmap, 0);

    int tamanioBitmap = mystat.st_size / BLOCK_SIZE / 8;
    memset(bmap,0,tamanioBitmap);

    printf("El tamaño del archivo es %li \n",mystat.st_size);

	if (bmap == MAP_FAILED) {
			printf("Error al mapear a memoria: %s\n", strerror(errno));

	}

	t_bitarray * bitarray =  bitarray_create_with_mode((char *)bmap, tamanioBitmap, LSB_FIRST);

	int tope = (int)bitarray_get_max_bit(bitarray);

	for(int i=0; i<tope; i++){
		bitarray_clean_bit(bitarray,i);
	}

	int tope2 = tamanioBitmap + GFILEBYTABLE + 1;

		for(int i=0; i<tope2; i++){
				bitarray_set_bit(bitarray,i);
			}

	printf("El tamano del bitarray creado es de: %i\n\n\n",(int)bitarray_get_max_bit(bitarray));
	printf("Bloques ocupados %i\n",tope2);
	printf("Bloques libres %li\n",(mystat.st_size / BLOCK_SIZE) - tope2);
	munmap(bmap,mystat.st_size);
	close(bitmap);

	return bitarray;
}


unsigned long long getMicrotime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return ((unsigned long long)( (tv.tv_sec)*1000 + (tv.tv_usec)/1000 ));
}


int asignarBloqueLibre(){

	int encontroUnBloque = 0;
	int bloqueEncontrado = 0;

	for (int i = 0; i < bitarray_get_max_bit(bitArray); i++) {
		if (bitarray_test_bit(bitArray, i) == 0) {
			bitarray_set_bit(bitArray, i);
			encontroUnBloque = 1;
			bloqueEncontrado = i;
			break;
		}
	}

	if (encontroUnBloque) {
		loguearBloqueQueCambio(bloqueEncontrado);
		return bloqueEncontrado;
	}

	printf("No se encontro bloque disponible\n");
	exit(-1);
}


void loguearBloqueQueCambio(int bloqueEncontrado){

	char* mensajeALogear = malloc( strlen("Se modifico el bloque: .bin") + strlen(string_itoa(bloqueEncontrado)));
	strcpy(mensajeALogear, "Se modifico el bloque: ");
	strcat(mensajeALogear, string_itoa(bloqueEncontrado));
	strcat(mensajeALogear, ".bin");
	t_log* g_logger;
	g_logger = log_create( "./bitarray.log", "LFS", 0, LOG_LEVEL_INFO);
	log_info(g_logger, mensajeALogear);
	log_destroy(g_logger);
	free(mensajeALogear);
}


int tamanioEnBytesDelBitarray(){
	return 1024;
}
