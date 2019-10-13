#include "suse_utils.h"

int iniciar_conexion(int puerto){

	fd_set master;   // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in myaddr;     // dirección del servidor
	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax;        // número máximo de descriptores de fichero
	int listener;     // descriptor de socket a la escucha
	int newfd;        // descriptor de socket de nueva conexión aceptada
	    // buffer para datos del cliente
	int nbytes;
	int yes=1;        // para setsockopt() SO_REUSEADDR, más abajo
	int addrlen;
	int i;
	FD_ZERO(&master);    // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// obviar el mensaje "address already in use" (la dirección ya se está usando)
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
														sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(puerto);
	memset(&(myaddr.sin_zero), '\0', 8);
	if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(1);
	}

	// añadir listener al conjunto maestro
	FD_SET(listener, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = listener; // por ahora es éste

	// bucle principal
	for(;;) {
		read_fds = master; // cópialo
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == listener)
				{
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr,&addrlen)) == -1)
					{
						perror("accept");
					} else
					{
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) {    // actualizar el máximo
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				} else {

					void *buf=malloc(24); //doble del buffer maximo para evitar inconvenientes con la recepción

					// gestionar datos de un cliente
					if ((nbytes = recv(i, buf, 24, 0)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0) {
							// conexión cerrada
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {

						int* operacion=malloc(sizeof(int));
						memcpy (operacion, buf, 4);

						printf("\ncantidad de bytes %d\n", nbytes); //Debug only
						printf("\noperacion: %d\n", *operacion);		//Debug only

						switch(*operacion)
						{
							case INIT: suse_init(buf);
								break;
							case CREATE: suse_create(buf);
								break;
							case 6: exit(0); //debug
							default:;
						}

						free(operacion);
					}

					free(buf);
				}
			}
		}
	}

	limpiarListasyColas();
	return 0;
}
