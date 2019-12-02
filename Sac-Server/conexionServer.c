#include "conexionServer.h"


void iniciar_conexion(int ip, int puerto){
	  int serverSocket, newSocket;
	  struct sockaddr_in serverAddr;
	  struct sockaddr_storage serverStorage;
	  socklen_t addr_size;

	  //Create the socket.
	  serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	  serverAddr.sin_family = AF_INET;
	  serverAddr.sin_port = htons(puerto);
	  serverAddr.sin_addr.s_addr = INADDR_ANY; //ip

	  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

	  if(listen(serverSocket,50) == 0)
	    printf("Listening\n");
	  else
	    printf("Error\n");
	    pthread_t tid[60];
	    int i = 0;
	    while(1){
	    	//Accept call creates a new socket for the incoming connection
	    	 addr_size = sizeof serverStorage;
	    	 newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

	    	 //for each client request creates a thread and assign the client request to it to process
	    	 //so the main thread can entertain next request
	    	 if( pthread_create(&tid[i], NULL, socketThread, (void*)newSocket) != 0 )
	    	        printf("Failed to create thread\n");
	    	        if( i >= 50) {
	    	        	i = 0;
	    	        	while(i < 50) {
	    	                pthread_join(tid[i++],NULL);
	    	            }
	    	            i = 0;
	    	        }
	     }

}


void* socketThread(void* newSocket)
{
	  int cliente =  (int) newSocket;
	  int valread;

	  while(1){
	  // Verifica por tamaño de la operacion, que no se haya desconectado el socket
	  int *tamanioOperacion = malloc(sizeof(int));
	  if ((valread = read(cliente, tamanioOperacion, sizeof(int))) == 0)
	  {
		  printf("Host disconected \n");
		  close(cliente);
		  pthread_exit(NULL);

	  }  else  {
		  // lee el siguiente dato que le pasa el cliente, con estos datos, hace la operacion correspondiente
		  int *operacion = malloc(4);
		  read(cliente, operacion, sizeof(int));

		  switch (*operacion) {
		      case 1:
			  	  	// Operacion CREATE
			  	  	tomarPeticionCreate(cliente);
			  	  	break;
			  case 2:
				  	// Operacion OPEN
				    tomarPeticionOpen(cliente);
				    break;
			  case 3:
			  		// Operacion READ
			  		tomarPeticionRead(cliente);
			  		break;
			  case 4:
			  		// Operacion READDIR
			  		tomarPeticionReadDir(cliente);
			  		break;
			  case 5:
			  		// Operacion GETATTR
			  		tomarPeticionGetAttr(cliente);
			  		break;
			  case 6:
			  		// Operacion MKNOD
			  		tomarPeticionMkdir(cliente);
			  		break;
			  case 7:
			  		// Operacion UNLINK
			  		tomarPeticionUnlink(cliente);
			  		break;
			  case 8:
				     // Operacion RMDIR
				  	 tomarPeticionRmdir(cliente);
			  		break;
			  case 9:
			  		// Operacion WRITE
			  		tomarPeticionWrite(cliente);
			  		break;
			  case 10:
			  		// Operacion TRUNCATE
			  		tomarPeticionTruncate(cliente);
			  		break;
			  case 11:
			  		// Operacion RENAME
			  		tomarPeticionRename(cliente);
			  		break;
			  default:
				  	  ;
			  }
			free(operacion);
		}
	free(tamanioOperacion);
	 }
	return NULL;
}

	  /* Send message to the client socket
	  pthread_mutex_lock(&lock);
	  char *message = malloc(sizeof(client_message)+20);
	  strcpy(message,"Hello Client : ");
	  strcat(message,client_message);
	  strcat(message,"\n");
	  strcpy(buffer,message);
	  free(message);
	  pthread_mutex_unlock(&lock);
	  sleep(1);
	  send(newSocket,buffer,13,0);
	  printf("Exit socketThread \n");
	  close(newSocket);
	  pthread_exit(NULL);
	  */


/*
	int opt = 1;
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30, activity, i, cliente, valread;
	int max_cliente;
	struct sockaddr_in address;

	//set of socket descriptors
	fd_set readfds;

	//a message
	//char *message = "Este es el mensaje de FUSE \r\n";

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
	//address.sin_addr.s_addr = ip;
	address.sin_addr.s_addr = INADDR_ANY;
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
		max_cliente = master_socket;

		//add child sockets to set
		for (i = 0; i < max_clients; i++) {
			//socket descriptor
			cliente = client_socket[i];

			//if valid socket descriptor then add to read list
			if (cliente > 0)
				FD_SET(cliente, &readfds);

			//highest file descriptor number, need it for the select function
			if (cliente > max_cliente)
				max_cliente = cliente;
		}

		//wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
		activity = select(max_cliente + 1, &readfds, NULL, NULL, NULL);

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

			if( send(new_socket, message, strlen(message), 0) != strlen(message) )
			{
				perror("error al enviar mensaje al cliente");
			}
			puts("Welcome message sent successfully");
			*/
			//add new socket to array of sockets
/*
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
			cliente = client_socket[i];

			if (FD_ISSET(cliente, &readfds)) {

				// Verifica por tamaño de la operacion, que no se haya desconectado el socket
				int *tamanioOperacion = malloc(sizeof(int));
				if ((valread = read(cliente, tamanioOperacion, sizeof(int))) == 0)
				{
					getpeername(cliente, (struct sockaddr *) &address, (socklen_t *) &addrlen);
					printf("Host disconected, ip: %s, port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					close(cliente);
					client_socket[i] = 0;
				}
				else
				{
					// lee el siguiente dato que le pasa el cliente,
					// con estos datos, hace la operacion correspondiente
					int *operacion = malloc(4);
					read(cliente, operacion, sizeof(int));

					switch (*operacion) {
					case 1:
						// Operacion CREATE
						tomarPeticionCreate(cliente);
						break;
					case 2:
						// Operacion OPEN
						tomarPeticionOpen(cliente);
						break;
					case 3:
						// Operacion READ
						tomarPeticionRead(cliente);
						break;
					case 4:
						// Operacion READDIR
						tomarPeticionReadDir(cliente);
						break;
					case 5:
						// Operacion GETATTR
						tomarPeticionGetAttr(cliente);
						break;
					case 6:
						// Operacion MKNOD
						tomarPeticionMkdir(cliente);
						break;
					case 7:
						// Operacion UNLINK
						tomarPeticionUnlink(cliente);
						break;
					case 8:
						// Operacion RMDIR
						tomarPeticionRmdir(cliente);
						break;
					case 9:
						// Operacion WRITE
						tomarPeticionWrite(cliente);
						break;
					case 10:
						// Operacion TRUNCATE
						tomarPeticionTruncate(cliente);
						break;
					case 11:
						// Operacion RENAME
						tomarPeticionRename(cliente);
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
*/



void levantarConfigFile(config* pconfig){
	t_config* configuracion = leer_config();

	pconfig->ip = config_get_int_value(configuracion, "IP");
	pconfig->puerto = config_get_int_value(configuracion, "LISTEN_PORT");

}

t_config* leer_config() {
	return config_create("sacServer_config");
}

void loguearInfo(char* texto) {
	char* mensajeALogear = malloc( strlen(texto) + 1);
	strcpy(mensajeALogear, texto);
	t_log* g_logger;
	g_logger = log_create("./sacServer.log", "SacServer", 1, LOG_LEVEL_INFO);
	log_info(g_logger, mensajeALogear);
	log_destroy(g_logger);
	free(mensajeALogear);
}

void loguearError(char* texto) {
	char* mensajeALogear = malloc( strlen(texto) + 1);
	strcpy(mensajeALogear, texto);
	t_log* g_logger;
	g_logger = log_create("./sacServer.log", "SacServer", 1, LOG_LEVEL_ERROR);
	log_error(g_logger, mensajeALogear);
	log_destroy(g_logger);
	free(mensajeALogear);
}


void borrarBitmap(t_bitarray* bitArray){

	bitarray_destroy(bitArray);
	printf("Se ha borrado el Bitmap\n");
}



void tomarPeticionCreate(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int respuesta = o_create(pathCortado);

	char* buffer = malloc(2 * sizeof(int));

	//logueo respuesta create
	if (respuesta == 0){
		loguearInfo(" + Se hizo un create en SacServer\n");
	}
	else {
		loguearError(" - NO se pudo hacer el create en SacServer\n");
	}

	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &respuesta, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}


void tomarPeticionOpen(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);

	int ok = o_open(path);

	//logueo respuesta open
	if (ok == 1){
		loguearInfo(" + Se hizo un open en SacServer\n");
	}
	if (ok == 0) {
		loguearError(" - NO se pudo hacer el open en SacServer\n");
	}

	char* buffer = malloc(2 * sizeof(int));
	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &ok, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}


void tomarPeticionRead(int cliente){

	//Deserializo path, size y offset
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);

	char *pathCortado = string_substring_until(path, *tamanioPath);

	int* tamanioSize = malloc(sizeof(int));
	read(cliente, tamanioSize, sizeof(int));
	int* size = malloc(*tamanioSize);
	read(cliente, size, *tamanioSize);

	int* tamanioOffset = malloc(sizeof(int));
	read(cliente, tamanioOffset, sizeof(int));
	int* offset = malloc(*tamanioOffset);
	read(cliente, offset, *tamanioOffset);

	char* texto = malloc(4096); //TODO revisar esto!!
	int respuesta = o_read(pathCortado, *size, *offset, texto);

	if(respuesta == 0) // tamaño es 0
	{
		loguearInfo(" + Se hizo un read vacio en SacServer\n");

		char* buffer = malloc(sizeof(int));
		memcpy(buffer, &respuesta, sizeof(int));

		send(cliente, buffer, sizeof(int), 0);
		free(buffer);
		free(texto);
	} else {

		char* buffer = malloc(sizeof(int) + respuesta);

		memcpy(buffer , &respuesta, sizeof(int));
		memcpy(buffer + sizeof(int), texto,  respuesta);

		send(cliente, buffer, sizeof(int) + respuesta, 0);
		free(texto);
	}

	if (respuesta == 1){


	}
	if(respuesta == -1) {
		loguearError(" - NO se pudo hacer el read en SacServer\n");

		char* buffer = malloc(2 * sizeof(int));
		int tamanioRespuesta = sizeof(int);
		memcpy(buffer, &tamanioRespuesta, sizeof(int));
		memcpy(buffer + sizeof(int), &respuesta, sizeof(int));

		send(cliente, buffer, 2*sizeof(int), 0);
		free(texto);

	}
}

void tomarPeticionReadDir(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	o_readDir(pathCortado, cliente);

}

void tomarPeticionGetAttr(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	o_getAttr(pathCortado, cliente);

}

void tomarPeticionMkdir(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int res = o_mkdir(pathCortado);

	char* buffer = malloc(2 * sizeof(int));
	//logueo respuesta mkdir
	if (res == 0){
		loguearInfo(" + Se hizo un mkdir en SacServer\n");
	}
	if (res == 1) {
		loguearError(" - NO se pudo hacer el mkdir en SacServer\n");
	}

	int tamanioRes = sizeof(int);
	memcpy(buffer, &tamanioRes, sizeof(int));
	memcpy(buffer + sizeof(int), &res, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}


void tomarPeticionUnlink(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int res = o_unlink(pathCortado);

	char* buffer = malloc(2 * sizeof(int));
	//logueo respuesta unlink
	if (res == 0){
		loguearInfo(" + Se hizo un unlink en SacServer\n");
	}
	else {
		loguearError(" - NO se pudo hacer el unlink en SacServer\n");
	}

	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &res, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}

void tomarPeticionRmdir(int cliente){

	//Deserializo path
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int respuesta = o_rmdir(pathCortado);

	char* buffer = malloc(2 * sizeof(int));
	//logueo respuesta unlink
	if (respuesta == 0){
		loguearInfo(" + Se hizo un rmdir en SacServer\n");
	}
	else {
		loguearError(" - NO se pudo hacer el rmdir en SacServer\n");
	}

	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &respuesta, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}

void tomarPeticionWrite(int cliente){

	//Deserializo path, size, offset y buf
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int* tamanioSize = malloc(sizeof(int));
	read(cliente, tamanioSize, sizeof(int));
	int* size = malloc(*tamanioSize);
	read(cliente, size, *tamanioSize);

	int* tamanioOffset = malloc(sizeof(int));
	read(cliente, tamanioOffset, sizeof(int));
	int* offset = malloc(*tamanioOffset);
	read(cliente, offset, *tamanioOffset);

	int *tamanioBuf = malloc(sizeof(int));
	read(cliente, tamanioBuf, sizeof(int));
	char *buf = malloc(*size);
	read(cliente, buf, *size);

	int bytes = o_write(pathCortado, *size, *offset, buf);

	//Serializo bytes
	char* buffer = malloc(2 * sizeof(int));

	int tamanioBytes = sizeof(int);
	memcpy(buffer, &tamanioBytes, sizeof(int));
	memcpy(buffer + sizeof(int), &bytes, sizeof(int));

	send(cliente, buffer, 2*sizeof(int), 0);
	free(buf);
	free(offset);
	free(buffer);
	free(tamanioBuf);
	free(path);
	free(tamanioPath);

}


void tomarPeticionTruncate(int cliente){

	//Deserializo path y offset
	int *tamanioPath = malloc(sizeof(int));
	read(cliente, tamanioPath, sizeof(int));
	char *path = malloc(*tamanioPath);
	read(cliente, path, *tamanioPath);
	char *pathCortado = string_substring_until(path, *tamanioPath);

	int* tamanioOffset = malloc(sizeof(int));
	read(cliente, tamanioOffset, sizeof(int));
	int* offset = malloc(*tamanioOffset);
	read(cliente, offset, *tamanioOffset);

	int respuesta = o_truncate(pathCortado, *offset);

	char* buffer = malloc(2 * sizeof(int));
	//logueo respuesta unlink
	if (respuesta == 0){
		loguearInfo(" + Se hizo un truncate en SacServer\n");
	}
	else {
		loguearError(" - NO se pudo hacer el truncate en SacServer\n");
	}

	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &respuesta, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}



void tomarPeticionRename(int cliente){

	//Deserializo path viejo y path nuevo
	int *tamanioPath1 = malloc(sizeof(int));
	read(cliente, tamanioPath1, sizeof(int));
	char *path1 = malloc(*tamanioPath1);
	read(cliente, path1, *tamanioPath1);
	char *pathCortado1 = string_substring_until(path1, *tamanioPath1);

	int *tamanioPath2 = malloc(sizeof(int));
	read(cliente, tamanioPath2, sizeof(int));
	char *path2 = malloc(*tamanioPath2);
	read(cliente, path2, *tamanioPath2);
	char *pathCortado2 = string_substring_until(path2, *tamanioPath2);

	int respuesta = o_rename(pathCortado1, pathCortado2);

	char* buffer = malloc(2 * sizeof(int));
	//logueo respuesta unlink
	if (respuesta == 0){
		loguearInfo(" + Se hizo un rename en SacServer\n");
	}
	else {
		loguearError(" - NO se pudo hacer el rename en SacServer\n");
	}

	int tamanioOk = sizeof(int);
	memcpy(buffer, &tamanioOk, sizeof(int));
	memcpy(buffer + sizeof(int), &respuesta, sizeof(int));

	send(cliente, buffer, 2* sizeof(int), 0);
}


