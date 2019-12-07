#include "sac-server.h"

int path_size_in_bytes(const char* path){
	FILE *fd;
	int size;

	fd=fopen(path, "r"); // printf("Error al abrir el archivo calculando el tamanio");

	fseek(fd, 0L, SEEK_END);
	size = ftell(fd);

	fclose(fd);

	return size;
}



int main(){
	// Levanta archivo de configuracion
	config* pconfig = malloc(sizeof(config));
	levantarConfigFile(pconfig);


	// Obiene el tamanio del disco
	fuse_disc_size = path_size_in_bytes(DISC_PATH);

	// Asigna el size del bitarray de 64 bits
	_bitarray_64 = get_size() / 64;
	_bitarray_64_leak = get_size() - (_bitarray_64 * 64);

	// Abrir conexion y traer directorios, guarda el bloque de inicio para luego liberar memoria
	if ((discDescriptor = fd = open(DISC_PATH, O_RDWR, 0)) == -1){
		printf("ERROR");
	}

	struct stat mystat;
	if (fstat(fd, &mystat) < 0) {
			close(fd);
		}

	// Abrir conexion y traer directorios, guarda el bloque de inicio para luego liberar memoria
	header_start = (struct sac_header_t*) mmap(NULL, mystat.st_size , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
	Header_Data = *header_start;
	bitmap_start = (struct sac_file_t*) &header_start[HEADER];
	inicioTablaDeNodos = (struct sac_file_t*) &header_start[HEADER + TAMANIO_BITMAP];
	inicioBloquesDeDatos = (struct sac_file_t*) &header_start[HEADER + TAMANIO_BITMAP + TAMANIO_TABLA_DE_NODOS];
	/* Obliga a que se mantenga la tabla de nodos y el bitmap en memoria */
	mlock(bitmap_start, TAMANIO_BITMAP*TAMANIO_BLOQUE);
	mlock(inicioTablaDeNodos, TAMANIO_TABLA_DE_NODOS*TAMANIO_BLOQUE);
	/* El codigo es tan, pero tan egocentrico, que le dice al SO como tratar la memoria */
	madvise(header_start, ACTUAL_DISC_SIZE_B ,MADV_RANDOM);
	obtenerBloquesLibres();
	//iniciarMmap();
	//bitArray = bitarray_create(mmapDeBitmap, tamanioEnBytesDelBitarray());



	// Levanta conexion por socket
	iniciar_conexion(pconfig->ip, pconfig->puerto);

	// Logueo
	loguearInfo(" + Levantado Sac-Server correctamente");

	pthread_join(hiloLevantarConexion, NULL);
	return 0;
}

