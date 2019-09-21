#include "utils.h"

int iniciar_conexion(int ip, int puerto){
	int opt = 1;
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30, activity, i, sd, valread;
	int max_sd;
	struct sockaddr_in address;

	//set of socket descriptors
	fd_set readfds;

	//a message
	char *message = "Este es el mensaje de FUSE \r\n";

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
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ip;
	address.sin_port = htons(puerto);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("Bind fallo en FUSE");
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
			new_socket = accept(master_socket, (struct sockaddr *) &address, (socklen_t*) &addrlen);
			if (new_socket < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("Nueva Conexion , socket fd: %d , ip: %s , puerto: %d 	\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			// send new connection greeting message
			// ACA SE LE ENVIA EL PRIMER MENSAJE AL SOCKET (si no necesita nada importante, mensaje de bienvenida)
			if( send(new_socket, message, strlen(message), 0) != strlen(message) )
			{
				perror("error al enviar mensaje al cliente");
			}
			puts("Welcome message sent successfully");

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
				if ((valread = read(sd, tamanioOperacion, sizeof(int))) == 0)
				{
					getpeername(sd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
					printf("Host disconected, ip: %s, port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;
				}
				else
				{
					// lee el siguiente dato que le pasa el cliente,
					// con estos datos, hace la operacion correspondiente
					int *operacion = malloc(4);
					read(sd, operacion, sizeof(int));

					switch (*operacion) {
					case 1:
						// Operacion

						break;
					case 2:
						// Operacion

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

void levantarConfigFile(config* pconfig){
	t_config* configuracion = leer_config();

	pconfig->ip = config_get_int_value(configuracion, "IP");
	pconfig->puerto = config_get_int_value(configuracion, "LISTEN_PORT");

}

t_config* leer_config() {
	return config_create("sacCli_config");
}


t_log * crear_log() {
	return log_create("fuse.log", "fuse", 1, LOG_LEVEL_DEBUG);
}

t_bitarray * crearBitmap(){

	// revisar cambio de PATH para archivo
	int bitmap = open("/home/utnso/workspace/tp-2019-2c-Cbados/FileSystem/disk.bin", O_RDWR);

	struct stat mystat;

	if (fstat(bitmap, &mystat) < 0) {
	    printf("Error al establecer fstat\n");
	    close(bitmap);
	}

	void * bmap = mmap(NULL, mystat.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, bitmap, 0);

    int tamanioBitmap = mystat.st_size / BLOCK_SIZE / 8;
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
	munmap(bmap,mystat.st_size);
	close(bitmap);
	return bitarray;

}

