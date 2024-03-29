#include "utils.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/socket.h>
#include <stdint.h>
#include <sys/select.h>
#include <inttypes.h>
#include <stdio.h>

int iniciar_conexion(char *ip, int32_t puerto) {
	printf("IP %s - PUERTO %i\n", ip, puerto);
	int opt = 1;
	int master_socket, addrlen, new_socket, max_clients = 30,
			activity, i, sd, valread;
	int max_sd;
	struct sockaddr_in address;

	//set of socket descriptors
	fd_set readfds;

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++) {
		client_socket[i] = 0;
	}

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	//address.sin_addr.s_addr = INADDR_ANY;
	//address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(puerto);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *) &address, sizeof(address))
			< 0) {
		perror("Bind fallo en el FS");
		return 1;
	}

	listen(master_socket, 100);

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Esperando conexiones ...");

	while (1) {
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for (i = 0; i < max_clients; i++) {
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			//highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("select error");
		}

		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds)) {
			new_socket = accept(master_socket, (struct sockaddr *) &address,
					(socklen_t*) &addrlen);
			if (new_socket < 0) {

				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			//printf("Nueva Conexion , socket fd: %d , ip: %s , puerto: %d 	\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++) {
				//if position is empty
				if (client_socket[i] == 0) {
					client_socket[i] = new_socket;
					printf("Agregado a la lista de sockets como: %d\n", i);

					break;
				}
			}
		}

		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds)) {
				//Check if it was for closing , and also read the
				//incoming message
				int *tamanioOperacion = malloc(sizeof(int));

				if ((valread = read(sd, tamanioOperacion, sizeof(int))) == 0) {
					//printf("tamanio: %d", *tamanio);
					getpeername(sd, (struct sockaddr *) &address,
							(socklen_t *) &addrlen);
					printf("Host disconected, ip: %s, port: %d\n",
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;

				} else {

					int *operacion = malloc(sizeof(*tamanioOperacion));
					read(sd, operacion, sizeof(int));

					switch (*operacion) {
					case 1: //init
						printf("Llego un init del cliente: %d \n",sd);
						atenderMuseInit(sd);
						break;
					case 2: //close
						printf("Llego un close del cliente: %d \n",sd);
						atenderMuseClose(sd);
						break;
					case 3: //alloc
						printf("Llego un alloc del cliente: %d \n",sd);
						atenderMuseAlloc(sd);
						break;
					case 4: //free
						printf("Llego un free del cliente: %d\n",sd);
						atenderMuseFree(sd);
						break;
					case 5: //get
						printf("Llego un get del cliente: %d\n",sd);
						atenderMuseGet(sd);
						break;
					case 6: //copy
						printf("Llego un cpy del cliente: %d\n",sd);
						atenderMuseCopy(sd);
						break;
					case 7:	//map
						printf("Llego un mmap del cliente: %d\n",sd);
						atenderMuseMap(sd);
						break;
					case 8: //sync
						printf("Llego un sync del cliente: %d\n",sd);
						atenderMuseSync(sd);
						break;
					case 9: //unmap
						printf("Llego un unmap del cliente: %d\n",sd);
						atenderMuseUnmap(sd);
						break;
					default:
						break;
					}
					free(operacion);
				}

				free(tamanioOperacion);
			}
		}
	}
}

void levantarConfigFile(config* pconfig) {
	t_config* configuracion = config_create("muse_config");

	pconfig->ip = malloc(
			strlen(config_get_string_value(configuracion, "IP")) + 1);
	strcpy(pconfig->ip, config_get_string_value(configuracion, "IP"));

	//pconfig->ip = config_get_int_value(configuracion, "IP");
	pconfig->puerto = config_get_int_value(configuracion, "LISTEN_PORT");
	pconfig->tamanio_memoria = config_get_int_value(configuracion,
			"MEMORY_SIZE");
	pconfig->tamanio_pag = config_get_int_value(configuracion, "PAGE_SIZE");
	pconfig->tamanio_swap = config_get_int_value(configuracion, "SWAP_SIZE");

	config_destroy(configuracion);
}

t_config* leer_config() {
	return config_create("muse_config");
}

t_log * crear_log() {
	return log_create("muse.log", "muse", 1, LOG_LEVEL_DEBUG);
}

void loguearInfo(char* texto) {

}

void atenderMuseClose(int cliente) {
	museclose(cliente);
}

void atenderMuseInit(int cliente) {
	museinit(cliente);
}

void atenderMuseAlloc(int cliente) {

	//deserializo lo que me manda el cliente

	int *tamanio = malloc(sizeof(int));
	read(cliente, tamanio, sizeof(int));
	uint32_t *bytesAReservar = malloc(*tamanio);
	read(cliente, bytesAReservar, *tamanio);

	//loguearInfo(" + Se hizo un muse alloc\n");

	//serializo esa direccion y se la mando al cliente

	char* buffer = malloc(sizeof(int) + sizeof(uint32_t));

	int tamanioDireccion = sizeof(int);

	uint32_t direccion = musemalloc(*bytesAReservar, cliente); //aca hacemos el malloc en muse y devolves la direccion de memoria

	printf("La direccion del malloc es %i \n", direccion);

	memcpy(buffer, &tamanioDireccion, sizeof(int));
	memcpy(buffer + sizeof(int), &direccion, sizeof(uint32_t));

	send(cliente, buffer, sizeof(int) + sizeof(uint32_t), 0);

	free(tamanio);
	free(bytesAReservar);
	free(buffer);

}

void atenderMuseFree(int cliente) {
	//deserializo lo que me manda el cliente

	//Esto es al pedo pero no tengo ganas de modificar la serializacion
	int *tamanioDireccion = malloc(sizeof(int));
	read(cliente, tamanioDireccion, sizeof(int));

	uint32_t *direccionDeMemoria = malloc(sizeof(uint32_t));
	read(cliente, direccionDeMemoria, sizeof(uint32_t));

	//printf("free direccionDeMemoria %lu\n", *direccionDeMemoria);

	int resultado = musefree(cliente, *direccionDeMemoria);

	if (resultado == 0) {
		loguearInfo("Memoria liberada exitosamente");
	} else {
		loguearInfo("Error liberando memoria");
	}

	free(tamanioDireccion);
	free(direccionDeMemoria);
}

void atenderMuseGet(int cliente) {
	//deserializo lo que me manda el cliente

	int *tamanioDst = malloc(sizeof(int));
	read(cliente, tamanioDst, sizeof(int));
	void *dst = malloc(*tamanioDst);
	read(cliente, dst, *tamanioDst);

	int *tamanioSrc = malloc(sizeof(int));
	read(cliente, tamanioSrc, sizeof(int));
	uint32_t *src = malloc(*tamanioSrc);
	read(cliente, src, *tamanioSrc);

	int *tamanioN = malloc(sizeof(int));
	read(cliente, tamanioN, sizeof(int));
	size_t *N = malloc(*tamanioN);
	read(cliente, N, *tamanioN);


	//hacer en muse un get y mandar -1 error o 0 ok
	void *respuesta = museget(dst, *src, *N, cliente);

	int resultado;

	if (respuesta != NULL) {
		loguearInfo("Get realizado correctamente");
		resultado = 0;

	} else {
		loguearInfo("Error realizando get");
		resultado = -1;
	}

	//char* buffer = malloc(2 * sizeof(int));
	char* buffer = malloc(3 * sizeof(int) + *N);

	int tamanioResult = sizeof(int);
	memcpy(buffer, &tamanioResult, sizeof(int));
	memcpy(buffer + sizeof(int), &resultado, sizeof(int));

	//
	int tamanioRespuesta = sizeof(int);
	memcpy(buffer + 2 * sizeof(int), &tamanioRespuesta, sizeof(int));
	memcpy(buffer + 3 * sizeof(int), respuesta, *N);

	//send(cliente, buffer, 2 * sizeof(int), 0);
	send(cliente, buffer, 3 * sizeof(int) + *N, 0);

	free(tamanioDst);
	free(dst);
	free(tamanioSrc);
	free(src);
	free(tamanioN);
	free(N);
	free(buffer);
}

void atenderMuseCopy(int cliente) {
	//deserializo lo que me manda el cliente

	int *tamanioDst = malloc(sizeof(int));
	read(cliente, tamanioDst, sizeof(int));
	uint32_t *dst = malloc(*tamanioDst);
	read(cliente, dst, *tamanioDst);

	int *tamanioSrc = malloc(sizeof(int));
	read(cliente, tamanioSrc, sizeof(int));
	void *src = malloc(*tamanioSrc);
	read(cliente, src, *tamanioSrc);

	int *tamanioN = malloc(sizeof(int));
	read(cliente, tamanioN, sizeof(int));
	int *N = malloc(*tamanioN);
	read(cliente, N, *tamanioN);

	int z = *((int *) src);

	//printf("%" PRIu32 "\n", *dst);
	//printf("%d \n", z); //ok
	//printf("%d \n", *N);

	//hacer en muse un copy y mandar -1 error o 0 ok
	//int resultado = 0;
	int resultado = musecpy(*dst, src, *N, cliente);

	if (resultado == 0) {
		loguearInfo("Cpy realizado exitosamente");
	} else {
		loguearInfo("Error realizando cpy");
	}

	char* buffer = malloc(2 * sizeof(int));

	int tamanioResult = sizeof(int);
	memcpy(buffer, &tamanioResult, sizeof(int));
	memcpy(buffer + sizeof(int), &resultado, sizeof(int));

	send(cliente, buffer, 2 * sizeof(int), 0);

	free(buffer);
	free(tamanioDst);
	free(tamanioSrc);
	free(dst);
	free(src);
	free(tamanioN);
	free(N);

}

void atenderMuseMap(int cliente) {
	//deserializo lo que me manda el cliente

	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *dst = malloc(*tamanioPath);
	read(cliente, dst, *tamanioPath);
	char *dstCortado = string_substring_until(dst, *tamanioPath);

	int *tamanioLength = malloc(sizeof(int));
	read(cliente, tamanioLength, sizeof(int));
	size_t *length = malloc(*tamanioLength);
	read(cliente, length, *tamanioLength);

	int *tamanioFlags = malloc(sizeof(int));
	read(cliente, tamanioFlags, sizeof(int));
	int *flags = malloc(*tamanioFlags);
	read(cliente, flags, *tamanioFlags);

	printf("%s", dstCortado); //ok
	printf("%zu\n", *length); //ok
	printf("%d\n", *flags); //ok

	//hacer en muse un map y mandar la posicion de memoria mapeada a libmuse

	char* buffer = malloc(sizeof(int) + sizeof(uint32_t));
	int tamanioPosicionMemoria = sizeof(int);
	uint32_t posicionMemoria = 4294967295; //pongo esto para mandar algo
	memcpy(buffer, &tamanioPosicionMemoria, sizeof(int));
	memcpy(buffer + sizeof(int), &posicionMemoria, sizeof(uint32_t));

	send(cliente, buffer, sizeof(int) + sizeof(uint32_t), 0);

	free(tamanioPath);
	free(dst);
	free(tamanioLength);
	free(length);
	free(tamanioFlags);
	free(flags);
	free(buffer);

}

void atenderMuseSync(int cliente) {
	//deserializo lo que manda el cliente

	int *tamanioAddr = malloc(sizeof(int));
	read(cliente, tamanioAddr, sizeof(int));
	uint32_t *addr = malloc(*tamanioAddr);
	read(cliente, addr, *tamanioAddr);

	int *tamanioLen = malloc(sizeof(int));
	read(cliente, tamanioLen, sizeof(int));
	size_t *len = malloc(*tamanioLen);
	read(cliente, len, *tamanioLen);

	//hacer en muse un sync y mandar -1 error o 0 ok

	int resultado = 0;

	if (resultado == 0) {
		loguearInfo("sync realizado correctamente");
	} else {
		loguearInfo("Error realizando sync");
	}

	char* buffer = malloc(2 * sizeof(int));

	int tamanioResult = sizeof(int);
	memcpy(buffer, &tamanioResult, sizeof(int));
	memcpy(buffer + sizeof(int), &resultado, sizeof(int));

	send(cliente, buffer, 2 * sizeof(int), 0);

	free(tamanioAddr);
	free(addr);
	free(tamanioLen);
	free(len);
	free(buffer);
}

void atenderMuseUnmap(int cliente) {
	//deserializo lo que manda el cliente

	int *tamanioDir = malloc(sizeof(int));
	read(cliente, tamanioDir, sizeof(int));
	uint32_t *dir = malloc(*tamanioDir);
	read(cliente, dir, *tamanioDir);

	printf("%" PRIu32 "\n", *dir);

	//hacer en muse un unmap y mandar -1 error o 0 ok

	int resultado = 0;

	if (resultado == 0) {
		loguearInfo("unmap realizado correctamente");
	} else {
		loguearInfo("Error realizando unamap");
	}

	char* buffer = malloc(2 * sizeof(int));

	int tamanioResult = sizeof(int);
	memcpy(buffer, &tamanioResult, sizeof(int));
	memcpy(buffer + sizeof(int), &resultado, sizeof(int));

	send(cliente, buffer, 2 * sizeof(int), 0);

	free(tamanioDir);
	free(dir);
	free(buffer);

}

