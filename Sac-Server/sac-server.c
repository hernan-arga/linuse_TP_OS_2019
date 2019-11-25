#include "sac-server.h"


int main(){
	// Levanta archivo de configuracion
	config* pconfig = malloc(2 * sizeof(int));
	levantarConfigFile(pconfig);




	// Obiene el tamanio del disco
	//fuse_disc_size = path_size_in_bytes(DISC_PATH);

	// Asigna el size del bitarray de 64 bits
	_bitarray_64 = get_size() / 64;
	_bitarray_64_leak = get_size() - (_bitarray_64 * 64);

	// Abrir conexion y traer directorios, guarda el bloque de inicio para luego liberar memoria
	if ((discDescriptor = fd = open(DISC_PATH, O_RDWR, 0)) == -1) printf("ERROR");
	// Abrir conexion y traer directorios, guarda el bloque de inicio para luego liberar memoria
	header_start = (struct gheader*) mmap(NULL, ACTUAL_DISC_SIZE_B , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
	//Header_Data = *header_start;
	bitmap_start = (struct gfile*) &header_start[GHEADERBLOCKS];
	node_table_start = (struct gfile*) &header_start[GHEADERBLOCKS + BITMAP_BLOCK_SIZE];
	data_block_start = (struct gfile*) &header_start[GHEADERBLOCKS + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE];
	/* Obliga a que se mantenga la tabla de nodos y el bitmap en memoria */
	mlock(bitmap_start, BITMAP_BLOCK_SIZE*BLOCKSIZE);
	mlock(node_table_start, NODE_TABLE_SIZE*BLOCKSIZE);
	/* El codigo es tan, pero tan egocentrico, que le dice al SO como tratar la memoria */
	madvise(header_start, ACTUAL_DISC_SIZE_B ,MADV_RANDOM);




	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);

	// Logueo
	loguearInfo(" + Levantado Sac-Server correctamente");

	iniciarMmap();
	bitArray = bitarray_create(mmapDeBitmap, tamanioEnBytesDelBitarray());

	pthread_join(hiloLevantarConexion, NULL);
	return 0;
}
