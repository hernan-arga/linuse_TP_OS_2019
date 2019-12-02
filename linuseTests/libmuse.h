#ifndef LIBMUSE_H_
#define LIBMUSE_H_

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct sockaddr_in serverAddress;
struct sockaddr_in serverAddressMUSE;
struct sockaddr_in direccionServer;
int32_t server;
int32_t serverMUSE;
uint32_t tamanoDireccion;



    /**
     * Inicializa la biblioteca de MUSE.
     * @param id El Process (o Thread) ID para identificar el caller en MUSE.
     * @param ip IP en dot-notation de MUSE.
     * @param puerto Puerto de conexión a MUSE.
     * @return Si pasa un error, retorna -1. Si se inicializó correctamente, retorna 0.
     * @see Para obtener el id de un proceso pueden usar getpid() de la lib POSIX (unistd.h)
     * @note Debido a la naturaleza centralizada de MUSE, esta función deberá definir
     *  el ID del proceso/hilo según "IP-ID".
     */
    int muse_init(int id, char* ip, int puerto);

    /**
     * Cierra la biblioteca de MUSE.
     */
    void muse_close();

    /**
     * Reserva una porción de memoria contígua de tamaño `tam`.
     * @param tam La cantidad de bytes a reservar.
     * @return La dirección de la memoria reservada.
     */
    uint32_t muse_alloc(uint32_t tam);

    /**
     * Libera una porción de memoria reservada.
     * @param dir La dirección de la memoria a reservar.
     */
    void muse_free(uint32_t dir);

    /**
     * Copia una cantidad `n` de bytes desde una posición de memoria de MUSE a una `dst` local.
     * @param dst Posición de memoria local con tamaño suficiente para almacenar `n` bytes.
     * @param src Posición de memoria de MUSE de donde leer los `n` bytes.
     * @param n Cantidad de bytes a copiar.
     * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
     */
    int muse_get(void* dst, uint32_t src, size_t n);

    /**
     * Copia una cantidad `n` de bytes desde una posición de memoria local a una `dst` en MUSE.
     * @param dst Posición de memoria de MUSE con tamaño suficiente para almacenar `n` bytes.
     * @param src Posición de memoria local de donde leer los `n` bytes.
     * @param n Cantidad de bytes a copiar.
     * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
     */
    int muse_cpy(uint32_t dst, void* src, int n);


    /**
     * Devuelve un puntero a una posición mappeada de páginas por una cantidad `length` de bytes el archivo del `path` dado.
     * @param path Path a un archivo en el FileSystem de MUSE a mappear.
     * @param length Cantidad de bytes de memoria a usar para mappear el archivo.
     * @param flags
     *          MAP_PRIVATE     Solo un proceso/hilo puede mappear el archivo.
     *          MAP_SHARED      El segmento asociado al archivo es compartido.
     * @return Retorna la posición de memoria de MUSE mappeada.
     * @note: Si `length` sobrepasa el tamaño del archivo, toda extensión deberá estar llena de "\0".
     * @note: muse_free no libera la memoria mappeada. @see muse_unmap
     */
    uint32_t muse_map(char *path, size_t length, int flags);

    /**
     * Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
     * @param addr Dirección a memoria mappeada.
     * @param len Cantidad de bytes a escribir.
     * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
     * @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
     */
    int muse_sync(uint32_t addr, size_t len);

    /**
     * Borra el mappeo a un archivo hecho por muse_map.
     * @param dir Dirección a memoria mappeada.
     * @param
     * @note Esto implicará que todas las futuras utilizaciones de direcciones basadas en `dir` serán accesos inválidos.
     * @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan liberado la misma cantidad de muse_unmap que muse_map.
     * @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
     */
    int muse_unmap(uint32_t dir);

int muse_init(int id, char* ip, int puerto) { //Case 1

	serverMUSE = socket(AF_INET, SOCK_STREAM, 0);
	serverAddressMUSE.sin_family = AF_INET;
	serverAddressMUSE.sin_port = htons(puerto);
	//serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_addr.s_addr = inet_addr(ip);

	int resultado = connect(serverMUSE, (struct sockaddr *) &serverAddressMUSE,
			sizeof(serverAddressMUSE));

	if(resultado==-1){
		perror("Hubo un error en la conexion");
		exit(-1);
	}

	char *buffer = malloc(2*sizeof(int));
	int peticion = 1;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));
	send(serverMUSE, buffer, 2 * sizeof(int), 0);
	free(buffer);
	//printf("init\n");
	return resultado;
}

void muse_close() {
	char *buffer = malloc(2*sizeof(int));
	int peticion = 2;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));
	send(serverMUSE, buffer, 2 * sizeof(int), 0);
	free(buffer);
} //Case 2

uint32_t muse_alloc(uint32_t tam) { //Case 3
	//printf("alloc\n");
	//Serializo peticion (3) y uint32_t tam tamaño a reservar

	//2 size de tamanio, 1 size de peticion, 1 size de tam (parametro)
	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 3;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioTam = sizeof(tam);
	memcpy(buffer + 2 * sizeof(int), &tamanioTam, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &tam, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer, 3 * sizeof(int) + sizeof(uint32_t), 0);

	int *tamanio = malloc(sizeof(int));
	read(serverMUSE, tamanio, sizeof(int));
	uint32_t *direccionMemoriaReservada = malloc(*tamanio);
	read(serverMUSE, direccionMemoriaReservada, *tamanio);

	uint32_t direccionMemoriaReservadaFinal = direccionMemoriaReservada;
	free(buffer);
	free(tamanio);
	free(direccionMemoriaReservada);
	return direccionMemoriaReservadaFinal;

}

void muse_free(uint32_t dir) { //Case 4
	//free((void*) dir);
	//printf("free\n");
	//Serializo peticion (4) y uint32_t dir

	//2 size de tamanio, 1 size de peticion, 1 size de dir (parametro)
	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 4;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDir = sizeof(dir);
	memcpy(buffer + 2 * sizeof(int), &tamanioDir, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dir, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer, 3 * sizeof(int) + sizeof(uint32_t), 0);

	free(buffer);
}

int muse_get(void* dst, uint32_t src, size_t n) { //Case 5
	//printf("get\n");
	//Serializo peticion (5), void* dst ,uint32_t src y size_t n

	//4 size de tamanio, 1 size de peticion, 1 size de dst, 1 size de src y 1 size de n
	char *buffer = malloc(5 * sizeof(int) + sizeof(dst) + sizeof(uint32_t) + sizeof(size_t));

	int peticion = 5;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDst = sizeof(dst);
	memcpy(buffer + 2 * sizeof(int), &tamanioDst, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), dst, sizeof(dst));

	int tamanioSrc = sizeof(uint32_t);
	memcpy(buffer + 3 * sizeof(int) + sizeof(dst), &tamanioSrc, sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + sizeof(dst), &src, sizeof(uint32_t));

	int tamanioN = sizeof(size_t);
	memcpy(buffer + 4 * sizeof(int) + sizeof(dst) + sizeof(uint32_t), &tamanioN, sizeof(int));
	memcpy(buffer + 5 * sizeof(int) + sizeof(dst) + sizeof(uint32_t), &n, sizeof(size_t));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer,
			5 * sizeof(int) + sizeof(dst) + sizeof(uint32_t) + sizeof(size_t),
			0);

	//ahora leo la respuesta de muse si salio bien o error y la retorno

	int *tamanioResp = malloc(sizeof(int));
	read(serverMUSE, tamanioResp, sizeof(int));
	int *resultado = malloc(*tamanioResp);
	read(serverMUSE, resultado, *tamanioResp);

	int resultadoFinal = *resultado;

	free(buffer);
	free(tamanioResp);
	free(resultado);
	return resultadoFinal;
}

int muse_cpy(uint32_t dst, void* src, int n) { //Case 6
	//Serializo peticion (6) y parametros (uint32_t dst, void* src, int n)
	//printf("cpy\n");
	char *buffer = malloc(6 * sizeof(int) + sizeof(uint32_t) + sizeof(src));

	int peticion = 6;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDst = sizeof(dst);
	memcpy(buffer + 2 * sizeof(int), &tamanioDst, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dst, sizeof(uint32_t));

	int tamanioSrc = sizeof(src);
	memcpy(buffer + 3 * sizeof(int) + sizeof(uint32_t), &tamanioSrc,
			sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + sizeof(uint32_t), src, sizeof(src));

	int tamanioN = sizeof(n);
	memcpy(buffer + 4 * sizeof(int) + sizeof(uint32_t) + sizeof(src), &tamanioN,
			sizeof(int));
	memcpy(buffer + 5 * sizeof(int) + sizeof(uint32_t) + sizeof(src), &n,
			sizeof(int));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer, 6 * sizeof(int) + sizeof(uint32_t) + sizeof(src),
			0);

	//ahora leo la respuesta de muse si salio bien o error y la retorno

	int *tamanioResp = malloc(sizeof(int));
	read(serverMUSE, tamanioResp, sizeof(int));
	int *resultado = malloc(*tamanioResp);
	read(serverMUSE, resultado, *tamanioResp);

	int resultadoFinal = *resultado;

	free(buffer);
	free(tamanioResp);
	free(resultado);
	return resultadoFinal;

}

uint32_t muse_map(char *path, size_t length, int flags) { //Case 7
	//Serializo peticion (7) y parametros (char* path, size_t length, int flags)

	char *buffer = malloc(6 * sizeof(int) + strlen(path) + sizeof(size_t));

	int peticion = 7;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioPath = strlen(path);
	memcpy(buffer + 2 * sizeof(int), &tamanioPath, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), path, strlen(path));


	int tamanioLength = sizeof(size_t);
	memcpy(buffer + 3 * sizeof(int) + strlen(path), &tamanioLength,
			sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + strlen(path), &length, sizeof(size_t));

	int tamanioFlags = sizeof(int);
	memcpy(buffer + 4 * sizeof(int) + strlen(path) + sizeof(size_t),
			&tamanioFlags, sizeof(int));
	memcpy(buffer + 5 * sizeof(int) + strlen(path) + sizeof(size_t), &flags,
			sizeof(int));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer, 6 * sizeof(int) + strlen(path) + sizeof(size_t),
			0);

	//leo la posicion de memoria mapeada de me manda muse

	int *tamanio = malloc(sizeof(int));
	read(serverMUSE, tamanio, sizeof(int));
	uint32_t *posicionMemoriaMapeada = malloc(*tamanio);
	read(serverMUSE, posicionMemoriaMapeada, *tamanio);

	uint32_t posicionMemoriaMapeadaFinal = *posicionMemoriaMapeada;

	free(buffer);
	free(tamanio);
	free(posicionMemoriaMapeada);
	return posicionMemoriaMapeadaFinal;
}

int muse_sync(uint32_t addr, size_t len) { //Case 8
	//Serializo peticion (8)

	char *buffer = malloc(4 * sizeof(int) + sizeof(uint32_t) + sizeof(size_t));

	int peticion = 8;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioAddr = sizeof(addr);
	memcpy(buffer + 2 * sizeof(int), &tamanioAddr, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &addr, sizeof(uint32_t));

	int tamanioLen = sizeof(len);
	memcpy(buffer + 3 * sizeof(int) + sizeof(uint32_t), &tamanioLen,
			sizeof(int));
	memcpy(buffer + 4 * sizeof(int) + sizeof(uint32_t), &len, sizeof(size_t));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer,
			4 * sizeof(int) + sizeof(uint32_t) + sizeof(size_t), 0);

	int *tamanioResp = malloc(sizeof(int));
	read(serverMUSE, tamanioResp, sizeof(int));
	int *resultado = malloc(*tamanioResp);
	read(serverMUSE, resultado, *tamanioResp);

	int resultadoFinal = *resultado;

	free(buffer);
	free(tamanioResp);
	free(resultado);
	return resultadoFinal;
}

int muse_unmap(uint32_t dir) { //Case 9
	//Serializo peticion (9) y uint32_t dir

	char *buffer = malloc(3 * sizeof(int) + sizeof(uint32_t));

	int peticion = 9;
	int tamanioPeticion = sizeof(int);
	memcpy(buffer, &tamanioPeticion, sizeof(int));
	memcpy(buffer + sizeof(int), &peticion, sizeof(int));

	int tamanioDir = sizeof(dir);
	memcpy(buffer + 2 * sizeof(int), &tamanioDir, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), &dir, sizeof(uint32_t));

	//Falta conexion y se hace envio a MUSE
	send(serverMUSE, buffer, 3 * sizeof(int) + sizeof(uint32_t), 0);

	int *tamanioResp = malloc(sizeof(int));
	read(serverMUSE, tamanioResp, sizeof(int));
	int *resultado = malloc(*tamanioResp);
	read(serverMUSE, resultado, *tamanioResp);

	int resultadoFinal = *resultado;

	free(buffer);
	free(tamanioResp);
	free(resultado);
	return resultadoFinal;
}

#endif
