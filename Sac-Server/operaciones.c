#include "operaciones.h"
#include "estructuras.h"
#include "unistd.h"
#include <stdio.h>
#include <libgen.h>


int o_create(char* path){

	if (determinar_nodo(path) != -1){
		return -EEXIST;
	}
	log_info(logger, "Create: Path: %s", path);

	int nodo_padre, i, res = 0;
	int new_free_node;
	struct gfile *node;
	char *nombre = malloc(strlen(path) + 1), *nom_to_free = nombre;
	char *dir_padre = malloc(strlen(path) + 1), *dir_to_free = dir_padre;
	char *data_block;

	split_path(path, &dir_padre, &nombre);

	// Ubica el nodo correspondiente. Si es el raiz, lo marca como 0,
	// si es menor a 0, lo crea (mismos permisos).
	if (strcmp(dir_padre, "/") == 0) {
		nodo_padre = 0;
	} else if ((nodo_padre = determinar_nodo(dir_padre)) < 0){
		return -ENOENT;
	}

	node = node_table_start;

	// Toma un lock de escritura.
	//log_lock_trace(logger, "Mknod: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
	//log_lock_trace(logger, "Mknod: Recibe lock escritura.");

	// Busca el primer nodo libre (state 0) y cuando lo encuentra, lo crea:
	for (i = 0; (node->estado != 0) & (i <= NODE_TABLE_SIZE); i++) {
		node = &(node_table_start[i]);
	}
	// Si no hay un nodo libre, devuelve un error.
	if (i > NODE_TABLE_SIZE){
		res = -EDQUOT;
		goto finalizar;
	}

	// Escribe datos del archivo
	node->estado = OCUPADO;
	strcpy((char*) &(node->nombre_archivo[0]), nombre);
	node->tamanio_archivo = 0; // El tamanio se ira sumando a medida que se escriba en el archivo.
	node->bloque_padre = nodo_padre;
	// (Abajo) se utiliza esta marca para avisar que es un archivo nuevo. De esta manera, la funcion add_node conoce que esta recien creado.
	node->bloques_indirectos[0] = 0;
	node->fecha_creacion = node->fecha_modificacion = time(NULL);
	res = 0;

	// Obtiene un bloque libre para escribir.
	new_free_node = get_node();

	// Actualiza la informacion del archivo.
	add_node(node, new_free_node);

	// Lo relativiza al data block.
	new_free_node -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
	data_block = (char*) &(data_block_start[new_free_node]);

	// Escribe en ese bloque de datos.
	memset(data_block, '\0', BLOCKSIZE);

	finalizar:
	free(nom_to_free);
	free(dir_to_free);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
	//log_lock_trace(logger, "Mknod: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);

	return res;

	/*
	 * FUNCIONAMIENTO ANTERIOR
	 *
	 * int ok;

	char* path = string_new();
	string_append(&path, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&path, pathC);

	if( access( path, F_OK ) != -1 ) {
	    // file exists
		ok = 0;
	} else {
	    // file doesn't exist
		ok = 1;
		FILE *fp1;

		fp1 = fopen (path, "w");
		fclose(fp1);
	}
	return ok;
	*/
}



int o_open(char* path){

	/*
	 * FUNCIONAMIENTO ANTERIOR:
	 *
	 * char* pathAppend = string_new();
	string_append(&pathAppend, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&pathAppend, path);

	int respuesta;
	if( access( pathAppend, F_OK ) != -1 ) {
	    // file exists
		respuesta = 1;
	} else {
	    // file doesn't exist
		respuesta = 0;
	}
	return respuesta;
	*/

	return 0;
}

int o_read(char* path, int size, int offset, char* texto){

	log_info(logger, "Reading: Path: %s - Size: %d - Offset %d", path, size, offset);
	unsigned int nodo = determinar_nodo(path), bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar; // Estructura auxiliar para no dejar choclos
	struct gfile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tam = size;
	int res;

	if (nodo == -1){
		return -ENOENT;
	}

	node = node_table_start;

	// Ubica el nodo correspondiente al archivo
	node = &(node[nodo-1]);

	pthread_rwlock_rdlock(&rwlock); //Toma un lock de lectura.
	//log_lock_trace(logger, "Read: Toma lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	if(node->tamanio_archivo <= offset){
	log_error(logger, "Fuse intenta leer un offset mayor o igual que el tamanio de archivo. Se retorna size 0. File: %s, Size: %d", path, node->tamanio_archivo);
	res = 0;
	goto finalizar;
	} else if (node->tamanio_archivo <= (offset+size)){
	tam = size = ((node->tamanio_archivo)-(offset));
	log_error(logger, "Fuse intenta leer una posicion mayor o igual que el tamanio de archivo. Se retornaran %d bytes. File: %s, Size: %d", size, path, node->tamanio_archivo);
	}
	// Recorre todos los punteros en el bloque de la tabla de nodos
	for (bloque_punteros = 0; bloque_punteros < BLKINDIRECT; bloque_punteros++){

	// Chequea el offset y lo acomoda para leer lo que realmente necesita
	if (offset > BLOCKSIZE * 1024){
		offset -= (BLOCKSIZE * 1024);
		continue;
	}

	bloque_a_buscar = (node->bloques_indirectos)[bloque_punteros];	// Ubica el nodo de punteros a nodos de datos, es relativo al nodo 0: Header.
	bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo de punteros a nodos de datos, es relativo al bloque de datos.
	pointer_block =(ptrGBloque *) &(data_block_start[bloque_a_buscar]);		// Apunta al nodo antes ubicado. Lo utiliza para saber de donde leer los datos.

	// Recorre el bloque de punteros correspondiente.
	for (num_bloque_datos = 0; num_bloque_datos < 1024; num_bloque_datos++){

		// Chequea el offset y lo acomoda para leer lo que realmente necesita
		if (offset >= BLOCKSIZE){
			offset -= BLOCKSIZE;
			continue;
		}

		bloque_a_buscar = pointer_block[num_bloque_datos]; 	// Ubica el nodo de datos correspondiente. Relativo al nodo 0: Header.
		bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo, haciendolo relativo al bloque de datos.
		data_block = (char *) &(data_block_start[bloque_a_buscar]);

		// Corre el offset hasta donde sea necesario para poder leer lo que quiere.
		if (offset > 0){
			data_block += offset;
			offset = 0;
		}

		if (tam < BLOCKSIZE){
			memcpy(buf, data_block, tam);
			buf = &(buf[tam]);
			tam = 0;
			break;
		} else {
			memcpy(buf, data_block, BLOCKSIZE);
			tam -= BLOCKSIZE;
			buf = &(buf[BLOCKSIZE]);
			if (tam == 0) break;
		}

	}

	if (tam == 0) break;
	}
	res = size;

	finalizar:
	pthread_rwlock_unlock(&rwlock); //Devuelve el lock de lectura.
	log_lock_trace(logger, "Read: Libera lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	log_trace(logger, "Terminada lectura.");

	return res;

	/*
	 * FUNCIONAMIENTO ANTERIOR
	 *
	FILE *f;

	//open the file for write operation
	if( (f=fopen(path,"rb")) == NULL){
		//if the file does not exist print the string
		printf("No se pudo abrir el archivo");
	}
	fseek(f, offset, SEEK_SET);
	if( fread(texto, size, 1, f) == 0){
		printf("No leyo una verga");
	}

	fclose(f);
*/
}

void o_readDir(char* path, int cliente){

   struct dirent *dp;

   char* pathNuevo = string_new();
   string_append(&pathNuevo, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
   string_append(&pathNuevo, path);

   DIR *dir = opendir(pathNuevo);

   if (!dir){
	  return;
   }

   // concateno todos los directorios
   char* directoriosPegoteados = string_new();

   while ((dp = readdir(dir)) != NULL) {
	   string_append(&directoriosPegoteados, &dp->d_name);
	   string_append(&directoriosPegoteados, ";");
   }

   // serializo directoriosPegoteados y se los envio a saccli
   char* buffer = malloc(sizeof(int) + strlen(directoriosPegoteados));

   int tamanioDirectoriosPegoteados = strlen(directoriosPegoteados);
   memcpy(buffer, &tamanioDirectoriosPegoteados, sizeof(int));
   memcpy(buffer + sizeof(int), directoriosPegoteados, strlen(directoriosPegoteados));

   send(cliente, buffer, sizeof(int) + strlen(directoriosPegoteados), 0);

   closedir(dir);

}

void o_getAttr(char* nombre, int cliente){

	char* path = string_new();
	string_append(&path, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&path, nombre);

	if( access( path, F_OK ) != -1 ) {
		// file exists
		struct stat file_info;

		//Serializo ok = 1, file_info (mode y nlink)
		void* buffer = malloc( 7 * sizeof(int) + sizeof(file_info.st_mode));

		int ok = 1;
		int tamanioOk = sizeof(int);
		memcpy(buffer, &tamanioOk, sizeof(int));
		memcpy(buffer + sizeof(int), &ok, sizeof(int));

		lstat(path, &file_info);

		int tamanioStmode = sizeof(file_info.st_mode);
		memcpy(buffer + 2 * sizeof(int), &tamanioStmode, sizeof(int));
		memcpy(buffer + 3 * sizeof(int), &file_info.st_mode, sizeof(file_info.st_mode));

		int tamanioStnlink = sizeof(int);
		memcpy(buffer + 3 * sizeof(int) + sizeof(file_info.st_mode), &tamanioStnlink, sizeof(int));
		memcpy(buffer + 4 * sizeof(int) + sizeof(file_info.st_mode), &file_info.st_nlink, sizeof(int));

		int tamanioEscrito = sizeof(int);
		memcpy(buffer + 5 * sizeof(int) + sizeof(file_info.st_size), &tamanioEscrito, sizeof(int));
		memcpy(buffer + 6 * sizeof(int) + sizeof(file_info.st_size), &file_info.st_size, sizeof(int));

		send(cliente, buffer, 7 * sizeof(int) + sizeof(file_info.st_mode), 0);

	} else {
	    // file doesn't exist

		//Serializo ok = 0
		void* buffer = malloc( 2 * sizeof(int) );

		int ok = 0;
		int tamanioOk = sizeof(int);
		memcpy(buffer, &tamanioOk, sizeof(int));
		memcpy(buffer + sizeof(int), &ok, sizeof(int));

		send(cliente, buffer, 2 * sizeof(int), 0);
	}
}

int o_mkdir(char* path){
	int ok;
	struct stat sb;

	char* folder = string_new();
	string_append(&folder, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&folder, path);

	if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) {
	     // folder exists
		ok = 0;
	} else {
		// folder doesn't exist
		int check;
		check = mkdir(folder, 0700);

		// check if directory is created or not
		if (!check){
		    printf("Directory created\n");
			ok = 1;
		}
		else {
		   printf("Unable to create directory\n");
		   ok = 0;
		}
	}

	return ok;
}

int o_unlink(char* pathC){

	struct grasa_file_t* file_data;
	int node = determinar_nodo(pathC);

	//ENABLE_DELETE_MODE;

	file_data = &(node_table_start[node - 1]);

	// Toma un lock de escritura.
	//log_lock_trace(logger, "Ulink: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
	//log_lock_trace(logger, "Ulink: Recibe lock escritura.");

	delete_nodes_upto(file_data, 0, 0);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
	//Log_lock_trace(logger, "Ulink: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);

	//DISABLE_DELETE_MODE;

	return grasa_rmdir(pathC);

	/*
	 * FUNCIONAMIENTO ANTERIOR
	 *
	 * int ok;

	char* path = string_new();
	string_append(&path, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&path, pathC);

	if( access( path, F_OK ) != -1 ) {
	    // file exists
		// entonces lo elimino

		FILE *fp1;

		int status = remove(path);

		if (status == 0){
		    printf("%s file deleted successfully.\n", path);
			ok = 1;
		}
		else{
		    printf("Unable to delete the file\n");
		    perror("Following error occurred");
		    ok = 0;
		 }
	} else {
	    // file doesn't exist
		ok = 0;
	}

	return ok;
	*/
}

int o_rmdir(char* pathC){

	char* path = string_new();
	string_append(&path, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
	string_append(&path, pathC);

	int retorno = o_rmdir_2(path);

	return retorno;
}

int o_rmdir_2(char* path){

	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d) {
		struct dirent *p;

		r = 0;

		while (!r && (p = readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;

			// Skip the names "." and ".." as we don't want to recurse on them.
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
				continue;
			}

			len = path_len + strlen(p->d_name) + 2;
			buf = malloc(len);

			if (buf) {
				struct stat statbuf;

				snprintf(buf, len, "%s/%s", path, p->d_name);

				if (!stat(buf, &statbuf)) {
					if (S_ISDIR(statbuf.st_mode)) {
						r2 = o_rmdir_2(buf);
					} else {
						r2 = unlink(buf);
					}
				}
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	if (!r) {
		r = rmdir(path);
	}

	return r;
}


void o_write(char* path, int size, int offset, char* buffer){

	FILE *f;
	f = fopen(path,"wb");

	if(f){
		fseek(f, offset, SEEK_SET);
		fwrite(buffer, 1, size, f);
	}
	else{
		printf("write : no se pudo abrir el archivo");
	}

	fclose(f);
}
