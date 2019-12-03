#include "estructuras.h"

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

    int tamanioBitmap = mystat.st_size / TAMANIO_BLOQUE / 8;
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
	printf("Bloques libres %li\n",(mystat.st_size / TAMANIO_BLOQUE) - tope2);
	munmap(bmap,mystat.st_size);
	close(bitmap);

	return bitarray;
}


unsigned long long getMicrotime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
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


/*
 * split_path
 *
 * 	DESC
 * 		Divide el path con formato de [RUTA] en: [RUTA_SUPERIOR] y [NOMBRE].
 * 		Ejemplo:
 * 			path: /home/utnso/algo.txt == /home/utnso - algo.txt
 * 			path: /home/utnso/ == /home - utnso
 *
 * 	PARAM
 * 		path - Ruta a dividir
 * 		super_path - Puntero sobre el cual se guardara la ruta superior.
 * 		name - Puntero al nombre del archivo
 *
 * 	RET
 * 		0... SIEMPRE!
 *
 */
int dividirRuta(const char* path, char** super_path, char** name){

	int aux;
	strcpy(*super_path, path);
	strcpy(*name, path);
	// Obtiene y acomoda el nombre del archivo.
	if (lastchar(path, '/')) {
		(*name)[strlen(*name)-1] = '\0';
	}
	*name = strrchr(*name, '/');
	*name = *name + 1; // Acomoda el nombre, ya que el primer digito siempre es '/'

	// Acomoda el super_path
	if (lastchar(*super_path, '/')) {
		(*super_path)[strlen(*super_path)-1] = '\0';
	}
	aux = strlen(*super_path) - strlen(*name);
	(*super_path)[aux] = '\0';

	return 0;
}



/* @DESC
 * 		Determina cual es el nodo sobre el cual se encuentra un path.
 *
 * 	@PARAM
 * 		path - Direccion del directorio o archivo a buscar.
 *
 * 	@RETURN
 * 		Devuelve el numero de bloque en el que se encuentra el nombre.
 * 		Si el nombre no se encuentra, devuelve -1.
 *
 */
ptrGBloque dameNodoDe(const char* path){

	// Si es el directorio raiz, devuelve 0:
	if(!strcmp(path,"/")){
		return 0;
	}

	int i, nodo_anterior, err = 0;
	// Super_path usado para obtener la parte superior del path, sin el nombre.
	char *super_path = (char*) malloc(strlen(path) +1), *nombre = (char*) malloc(strlen(path)+1);
	char *start = nombre, *start_super_path = super_path; //Estos liberaran memoria.
	struct sac_file_t *node;
	unsigned char *node_name;

	dividirRuta(path, &super_path, &nombre);

	nodo_anterior = dameNodoDe(super_path);

	pthread_rwlock_rdlock(&superLockeador); //Toma un lock de lectura.
	//log_lock_trace(logger, "Determinar_nodo: Toma lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	node = inicioTablaDeNodos;

	// Busca el nodo sobre el cual se encuentre el nombre.
	node_name = &(node->nombre_archivo[0]);
	for (i = 0; ( (node->bloque_padre != nodo_anterior) | (strcmp(nombre, (char*) node_name) != 0) | (node->estado == 0)) & (i < GFILEBYTABLE) ; i++ ){
		 node = &(node[1]);
		 node_name = &(node->nombre_archivo[0]);
	}

	// Cierra conexiones y libera memoria. Contempla casos de error.
	pthread_rwlock_unlock(&superLockeador);
	//log_lock_trace(logger, "Determinar_nodo: Libera lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	free(start);
	free(start_super_path);
	if (err != 0) return err;
	if (i >= GFILEBYTABLE) return -1;

	return (i+1);
}


/*
 *  @DESC
 *  	Obtiene un bloque libre, actualiza el bitmap.
 *
 *  @PARAM
*		(void)
 *
 *  @RETURN
 *  	Devuelve el numero de un bloque listo para escribir. Si hay error, un numero negativo, correspondiente al error.
 */
int obtenerBloqueLibre(void){
	t_bitarray *bitarray;
	int res;

	bitarray = bitarray_create_with_mode((char *)bitmap_start, BITMAP_SIZE_BITS, LSB_FIRST);

	int encontrado = 0;

	for (int i =HEADER+TAMANIO_BITMAP+GFILEBYTABLE; i < bitarray_get_max_bit(bitarray); i++) {
		if(bitarray_test_bit(bitarray, i) == 0) {
			bitarray_set_bit(bitarray, i);
			encontrado = 1;
			res = i;
			break;

		}
	}

	//res = bitarray_test_and_set(bitarray, GHEADERBLOCKS+BITMAP_BLOCK_SIZE+GFILEBYTABLE);

	// Cierra el bitmap
	bitarray_destroy(bitarray);
	return res;
}


/*
 *  @DESC
 *  	Actualiza la informacion del archivo.
 *
 *  @PARAM
 *		file_data - El puntero al nodo en el que se encuentra el archivo.
 *		node_number - El numero de nodo que se le debe agregar.
 *
 *  @RET
 *  	Devuelve 0 si salio bien, negativo si hubo problemas.
 */
int agregarBloqueLibre(struct sac_file_t *file_data, int node_number){
	int node_pointer_number, position;
	size_t tam = file_data->tamanio_archivo;
	int new_pointer_block;
	ptrGBloque *nodo_punteros;

	// Ubica el ultimo nodo escrito y se posiciona en el mismo.
	set_position(&node_pointer_number, &position, 0, tam);

	if((node_pointer_number == BLOQUESINDIRECTOS-1) & (position == PTRGBLOQUE_SIZE-1)){
		return -ENOSPC;
	}

	// Si es el primer nodo del archivo y esta escrito, debe escribir el segundo.
		// Se sabe que el primer nodo del archivo esta escrito siempre que el primer
	    //puntero a bloque punteros del nodo sea distinto de 0 (file_data->blk_indirect[0] != 0)
		// ya que se le otorga esa marca (=0) al escribir el archivo, para indicar que es un archivo nuevo
	if ((file_data->bloques_indirectos[node_pointer_number] != 0)){
		if (position == 1024) {
			position = 0;
			node_pointer_number++;
		}
	}
	// Si es el ultimo nodo en el bloque de punteros, pasa al siguiente
	if (position == 0){
		new_pointer_block = obtenerBloqueLibre();
		if(new_pointer_block < 0) {
			return new_pointer_block;
			/* Si sucede que sea menor a 0, contendra el codigo de error */
		}
		memset((char*)&(header_start[new_pointer_block]), 0, TAMANIO_BLOQUE);
		file_data->bloques_indirectos[node_pointer_number] = new_pointer_block;
		// Cuando crea un bloque, settea al siguente como 0, dejando una marca.
		file_data->bloques_indirectos[node_pointer_number +1] = 0;
	} else {
		new_pointer_block = file_data->bloques_indirectos[node_pointer_number];
		//Se usa como auxiliar para encontrar el numero del bloque de punteros
	}

	// Ubica el nodo de punteros, relativo al bloque de datos.
	nodo_punteros = (ptrGBloque*) &inicioBloquesDeDatos[new_pointer_block - (HEADER + TAMANIO_TABLA_DE_NODOS + TAMANIO_BITMAP)];

	// Hace que dicho puntero, en la posicion ya obtenida, apunte al nodo indicado.
	nodo_punteros[position] = node_number;

	return 0;
}


/*
 *
 *
 * OPERACIONES DE MIERDA
 *
 *
 *
 */

int get_size(void){
	return ((int) (ACTUAL_DISC_SIZE_B / TAMANIO_BLOQUE));
}

int lastchar(const char* str, char chr){
	if ( ( str[strlen(str)-1]  == chr) ){
		return 1;
	}
	return 0;
}

int set_position (int *pointer_block, int *data_block, size_t size, off_t offset){
	div_t divi;
	divi = div(offset, (TAMANIO_BLOQUE*PTRGBLOQUE_SIZE));
	*pointer_block = divi.quot;
	*data_block = divi.rem / TAMANIO_BLOQUE;
	return 0;
}

/*
 *	@DESC
 *		Borra los nodos hasta la estructura correspondiente (upto:hasta) especificadas. EXCLUSIVE.
 *
 *	@PARAM
 *
 *
 *	@RET
 *
 */
int delete_nodes_upto (struct sac_file_t *file_data, int pointer_upto, int data_upto){
	t_bitarray *bitarray;
	size_t file_size = file_data->tamanio_archivo;
	int node_to_delete, node_pointer_to_delete, delete_upto;
	ptrGBloque *aux; // Auxiliar utilizado para saber que nodo redireccionar
	int data_pos, pointer_pos;

	// Ubica cual es el ultimo nodo del archivo
	set_position(&pointer_pos, &data_pos, 0, file_size);
	if (file_size%(TAMANIO_BLOQUE*PTRGBLOQUE_SIZE) == 0) {
		pointer_pos--;
		data_pos = PTRGBLOQUE_SIZE-1;
	}
	else if (file_size%TAMANIO_BLOQUE == 0) data_pos--;

	// Crea el bitmap
	//bitarray = bitarray_create((char*) bitmap_start, BITMAP_SIZE_B);
	bitarray = bitarray_create_with_mode((char *)bitmap_start, BITMAP_SIZE_BITS, LSB_FIRST);

	// Activa el DELETE_MODE. Este modo NO debe activarse cuando se hacen operaciones que
	// dejen al archivo con un solo nodo. Por ejemplo, truncate -s 0.
	// Deberia estar activo en otro momento.
	if((pointer_upto != 0) | (data_upto != 0)){
		DISABLE_DELETE_MODE;
	}

	// Borra hasta que los nodos de posicion coincidan con los nodos especificados.
	while( (data_pos != data_upto) | (pointer_pos != pointer_upto) | (DELETE_MODE == 1) ){  // | ((data_pos == 0) & (pointer_pos == 0)) ){
		if ((data_pos < 0) | (pointer_pos < 0)) break;

		// localiza el puntero de datos a borrar.
		node_pointer_to_delete = file_data->bloques_indirectos[pointer_pos];
		aux = (ptrGBloque*) &(header_start[node_pointer_to_delete]);

		// Indica hasta que nodo debe borrar.
		if (pointer_pos == pointer_upto){
			delete_upto = data_upto;
		} else {
			delete_upto = 0;
		}

		// Borra los nodos de datos que sean necesarios.
		while ((data_pos + DELETE_MODE) != delete_upto){
			node_to_delete = aux[data_pos];
			bitarray_clean_bit(bitarray, node_to_delete);
			bitmap_free_blocks++;
			data_pos--;
		}

		// Si es necesario, borra el nodo de punteros.
		if ((pointer_pos + DELETE_MODE) != pointer_upto){
			bitarray_clean_bit(bitarray, node_pointer_to_delete);
			file_data->bloques_indirectos[pointer_pos] = 0;
			pointer_pos--;
			data_pos = 1023;
		}

	}

	// Cierra el bitmap
	bitarray_destroy(bitarray);
	return 0;
}



/*
 *	@DESC
 *		Obtiene espacio nuevo para un archivo, agregandole los nodos que sean necesarios.
 *		Actualiza el FileSize al tamanio correspondiente.
 *
 *	@PARAM
 *		file_data - El puntero al nodo donde se encuentra el archivo.
 *		size - El tamanio que se le debe agregar.
 *
 *	@RET
 *		0 - Se consiguio el espacio requerido
 *		negativo - Error.
 */
int get_new_space (struct sac_file_t *file_data, int size){
	size_t file_size = file_data->tamanio_archivo, space_in_block = file_size % TAMANIO_BLOQUE;
	int new_node;
	space_in_block = TAMANIO_BLOQUE - space_in_block; // Calcula cuanto tamanio le queda para ocupar en el bloque

	// Si no hay suficiente espacio, retorna error.
	if ((bitmap_free_blocks*TAMANIO_BLOQUE) < (size - space_in_block)) return -ENOSPC;

	// Actualiza el file size al tamanio que le corresponde:
	if (space_in_block >= size){
		file_data->tamanio_archivo += size;
		return 0;
	} else {
		file_data->tamanio_archivo += space_in_block;
	}

	while ( (space_in_block <= size) ){ // Siempre que lo que haya que escribir sea mas grande que el espacio que quedaba en el bloque
		new_node = obtenerBloqueLibre();
		agregarBloqueLibre(file_data, new_node);
		size -= TAMANIO_BLOQUE;
		file_data->tamanio_archivo += TAMANIO_BLOQUE;
	}

	file_data->tamanio_archivo += size;

	return 0;
}

int obtain_free_blocks(void){
	t_bitarray *bitarray;
	int free_nodes=0, i;
	int bitmap_size_in_bits = BITMAP_SIZE_BITS;

	bitarray = bitarray_create_with_mode((char *)bitmap_start, BITMAP_SIZE_B, LSB_FIRST);

	for (i = 0; i < bitmap_size_in_bits; i++){
		if (bitarray_test_bit(bitarray, i) == 0) free_nodes++;
	}

	bitarray_destroy(bitarray);

	bitmap_free_blocks = free_nodes;

	return free_nodes;
}
