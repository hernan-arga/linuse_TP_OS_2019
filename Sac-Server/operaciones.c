#include "operaciones.h"
#include "unistd.h"
#include <stdio.h>

int o_create(char* path){

	FILE *fp1;
	fp1= fopen (path, "r");

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
   DIR *dir = opendir(path);

   if (!dir){
	  return;
   }

   // concateno todos los directorios
   char* directoriosPegoteados = string_new();
   while ((dp = readdir(dir)) != NULL) {
	   string_append(&directoriosPegoteados, dp->d_name);
	   string_append(directoriosPegoteados, ";");
   }

   // serializo directoriosPegoteados y se los envio a saccli
   char* buffer = malloc(sizeof(int) + strlen(directoriosPegoteados));

   int tamanioDirectoriosPegoteados = strlen(directoriosPegoteados);
   memcpy(buffer, &tamanioDirectoriosPegoteados, sizeof(int));
   memcpy(buffer + sizeof(int), directoriosPegoteados, strlen(directoriosPegoteados));

   send(cliente, buffer, sizeof(int) + strlen(directoriosPegoteados), 0);

   closedir(dir);
}


