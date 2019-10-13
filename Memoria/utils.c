#include "utils.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/socket.h>
#include <stdint.h>
#include <sys/select.h>
#include <inttypes.h>

int iniciar_conexion(int ip, int puerto) {
	int opt = 1;
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30,
			activity, i, sd, valread;
	int max_sd;
	struct sockaddr_in address;

	//set of socket descriptors
	fd_set readfds;

	//a message
	char *message = "Este es el mensaje de MUSE \r\n";

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
	//address.sin_addr.s_addr = ip;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(puerto);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *) &address, sizeof(address))
			< 0) {
		perror("Bind fallo en MUSE");
		return 1;
	}

	printf("Escuchando en el puerto: %d \n", puerto);
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

		//wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("Error en conexion por select");
		}

		//If something happened on the master socket, then its an incoming connection
		if (FD_ISSET(master_socket, &readfds)) {
			new_socket = accept(master_socket, (struct sockaddr *) &address,
					(socklen_t*) &addrlen);
			if (new_socket < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("Nueva Conexion , socket fd: %d , ip: %s , puerto: %d 	\n",
					new_socket, inet_ntoa(address.sin_addr),
					ntohs(address.sin_port));

			// send new connection greeting message
			// ACA SE LE ENVIA EL PRIMER MENSAJE AL SOCKET (si no necesita nada importante, mensaje de bienvenida)
			/*
			 if (send(new_socket, message, strlen(message), 0)
			 != strlen(message)) {
			 perror("error al enviar mensaje al cliente");
			 }
			 puts("Welcome message sent successfully");
			 */
			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++) {
				//if position is empty
				if (client_socket[i] == 0) {
					client_socket[i] = new_socket;
					printf("Agregado a la lista de sockets como: %d\n", i);
					break;
				}
			}
		} // cierro el if

		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds)) {

				// Verifica por tamaño de la operacion, que no se haya desconectado el socket
				int *tamanioOperacion = malloc(sizeof(int));
				if ((valread = read(sd, tamanioOperacion, sizeof(int))) == 0) {
					getpeername(sd, (struct sockaddr *) &address,
							(socklen_t *) &addrlen);
					printf("Host disconected, ip: %s, port: %d\n",
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;
				} else {
					// lee el siguiente dato que le pasa el cliente
					// con estos datos, hace la operacion correspondiente
					int *operacion = malloc(4);
					read(sd, operacion, sizeof(int));

					switch (*operacion) {
					case 1: //init
						//atenderMuseInit(sd);
						break;
					case 2: //close
						//atenderMuseClose(sd);
						break;
					case 3: //alloc
						printf("llego alloc");
						atenderMuseAlloc(sd);
						break;
					case 4: //free
						printf("llego free");
						atenderMuseFree(sd);
						break;
					case 5: //get
						printf("llego get");
						atenderMuseGet(sd);
						break;
					case 6: //copy
						printf("llego copy");
						atenderMuseCopy(sd);
						break;
					case 7:	//map
						printf("llego map");
						atenderMuseMap(sd);
						break;
					case 8: //sync
						printf("llego sync");
						atenderMuseSync(sd);
						break;
					case 9: //unmap
						printf("llego unmap");
						atenderMuseUnmap(sd);
						break;
					default:
						;
					}
					free(operacion);
				}
				free(tamanioOperacion);
			}
		}
	}
}

void levantarConfigFile(config* pconfig) {
	t_config* configuracion = leer_config();

	pconfig->ip = config_get_int_value(configuracion, "IP");
	pconfig->puerto = config_get_int_value(configuracion, "LISTEN_PORT");
	pconfig->tamanio_memoria = config_get_int_value(configuracion,
			"MEMORY_SIZE");
	pconfig->tamanio_pag = config_get_int_value(configuracion, "PAGE_SIZE");
	pconfig->tamanio_swap = config_get_int_value(configuracion, "SWAP_SIZE");
}

t_config* leer_config() {
	return config_create("muse_config");
}

t_log * crear_log() {
	return log_create("muse.log", "muse", 1, LOG_LEVEL_DEBUG);
}

void loguearInfo(char* texto) {
	char* mensajeALogear = malloc(strlen(texto) + 1);
	strcpy(mensajeALogear, texto);
	t_log* g_logger;
	g_logger = log_create("./Muse.log", "Muse", 1, LOG_LEVEL_INFO);
	log_info(g_logger, mensajeALogear);
	log_destroy(g_logger);
	free(mensajeALogear);
}

void atenderMuseAlloc(int cliente) {

	//deserializo lo que me manda el cliente

	int *tamanio = malloc(sizeof(int));
	read(cliente, tamanio, sizeof(int));
	uint32_t *bytesAReservar = malloc(*tamanio);
	read(cliente, bytesAReservar, *tamanio);

	//loguearInfo(" + Se hizo un muse alloc\n");

	//musemalloc(10);

	//aca tendriamos que hacer el malloc en muse y devolver la direccion de memoria
	//serializo esa direccion y se la mando al cliente

	char* buffer = malloc(sizeof(int) + sizeof(uint32_t));

	int tamanioDireccion = sizeof(int);
	uint32_t direccion = 4294967295; //pongo esto para mandar algo
	memcpy(buffer, &tamanioDireccion, sizeof(int));
	memcpy(buffer + sizeof(int), &direccion, sizeof(uint32_t));

	send(cliente, buffer, sizeof(int) + sizeof(uint32_t), 0);

}

void atenderMuseFree(int cliente) {
	//deserializo lo que me manda el cliente

	int *tamanioDireccion = malloc(sizeof(int));
	read(cliente, tamanioDireccion, sizeof(int));
	uint32_t *direccionDeMemoria = malloc(*tamanioDireccion);
	read(cliente, direccionDeMemoria, *tamanioDireccion);

	printf("%" PRIu32 "\n", *direccionDeMemoria);

	//int resultado = museFree(direccionDeMemoria);
	int resultado = 1; //pa probar

	if (resultado == 1) {
		loguearInfo("Memoria liberada exitosamente");
	} else {
		loguearInfo("Error liberando memoria");
	}

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

	int y = *((int *) dst);

	printf("%d \n", y); //ok
	printf("%" PRIu32 "\n", *src);
	printf("%zu\n", *N);

	//hacer en muse un get y mandar -1 error o 0 ok
	int resultado = 0;

	if (resultado == 0) {
		loguearInfo("Get realizado correctamente");
	} else {
		loguearInfo("Error realizando get");
	}

	char* buffer = malloc(2 * sizeof(int));

	int tamanioResult = sizeof(int);
	memcpy(buffer, &tamanioResult, sizeof(int));
	memcpy(buffer + sizeof(int), &resultado, sizeof(int));

	send(cliente, buffer, 2 * sizeof(int), 0);

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

	//hacer en muse un copy y mandar -1 error o 0 ok

	int resultado = 0;

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

}

void atenderMuseMap(int cliente) {
	//deserializo lo que me manda el cliente

	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *dst = malloc(*tamanioPath);
	read(cliente, dst, *tamanioPath);
	char *dstCortado = string_substring_until(dst,
				*tamanioPath);

	int *tamanioLength = malloc(sizeof(int));
	read(cliente, tamanioLength, sizeof(int));
	size_t *length = malloc(*tamanioLength);
	read(cliente, length, *tamanioLength);

	int *tamanioFlags = malloc(sizeof(int));
	read(cliente, tamanioFlags, sizeof(int));
	int *flags = malloc(*tamanioFlags);
	read(cliente, flags, *tamanioFlags);

	printf("%s",dstCortado); //ok
	printf("%zu\n", *length); //ok
	printf("%d\n", *flags); //ok

	//hacer en muse un map y mandar la posicion de memoria mapeada a libmuse

	char* buffer = malloc(sizeof(int) + sizeof(uint32_t));
	int tamanioPosicionMemoria = sizeof(int);
	uint32_t posicionMemoria = 4294967295; //pongo esto para mandar algo
	memcpy(buffer, &tamanioPosicionMemoria, sizeof(int));
	memcpy(buffer + sizeof(int), &posicionMemoria, sizeof(uint32_t));

	send(cliente, buffer, sizeof(int) + sizeof(uint32_t), 0);

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

}

