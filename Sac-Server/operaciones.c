#include "operaciones.h"
#include "unistd.h"
#include <stdio.h>

int o_create(char* path){

	FILE *fp1;
	fp1 = fopen (path, "w");

	fclose(fp1);
	int respuesta = 1;
	return respuesta;
}

int o_open(char* path){

	int respuesta;
	if( access( path, F_OK ) != -1 ) {
	    // file exists
		respuesta = 1;
	} else {
	    // file doesn't exist
		respuesta = 0;
	}
	return respuesta;
}

void o_read(char* path, int size, int offset, char* texto){

	FILE *f;

	//open the file for write operation
	if( (f=fopen(path,"w")) == NULL){
		//if the file does not exist print the string
		printf("No se pudo abrir el archivo");
	}
	if( fread(texto, size, offset, path) == 0){
		printf("No leyo una verga");
	}

	fclose(f);
}

void o_readDir(char* path, int cliente){

	struct dirent *dp;
   DIR *dir = opendir("/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");

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
		void* buffer = malloc( 5 * sizeof(int) + sizeof(file_info.st_mode));

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

		send(cliente, buffer, 5 * sizeof(int) + sizeof(file_info.st_mode), 0);

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

