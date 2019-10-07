#include "operaciones.h"
#include "unistd.h"
#include <stdio.h>

int o_create(char* pathC){
	int ok;

	if( access( pathC, F_OK ) != -1 ) {
	    // file exists
		ok = 0;
	} else {
	    // file doesn't exist
		ok = 1;
		FILE *fp1;

		char* path = string_new();
		string_append(&path, "/home/utnso/tp-2019-2c-Cbados/Sac-Server/miFS");
		string_append(&path, pathC);

		fp1 = fopen (path, "w");
		fclose(fp1);
	}
	return ok;
}

int o_open(char* path){

	char* pathAppend = string_new();
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
}

void o_read(char* path, int size, int offset, char* texto){

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
