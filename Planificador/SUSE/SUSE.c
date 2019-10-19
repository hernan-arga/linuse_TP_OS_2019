#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <ctype.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>

#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/string.h>

typedef struct {
	int32_t PUERTO;
	int METRICAS_TIMER;
	int GRADO_DE_MULTIPROGRAMACION;
	t_dictionary *SEMAFOROS;
	float ALPHA_SJF;
} archivoConfiguracion;

typedef struct {
	//char * IDENTIFICADOR_SEMAFORO;	Creo que no me hace falta
	int VALOR_ACTUAL_SEMAFORO;
	int VALOR_MAXIMO_SEMAFORO;
} semaforo;

typedef struct {
	int pid;
	int tid;
	float estimacion;
	float rafaga;
} hilo;

typedef struct {
	int pid;
	hilo *exec;
} programa;

void iniciarSUSE();
int32_t iniciarConexion();
void planificarReady();
void sem_suse_signal(char*);
void pasarAReady(hilo*);
void actualizarEstimacion(hilo*);
void pasarAExec(hilo*);
void crearHilo(int);
int siguienteAEjecutar(int);
hilo* siguienteDeReadyAExec(int);
void planificarExecParaUnPrograma(char *, programa *);
void planificarExec();


pthread_t hiloLevantarConexion;
pthread_t hiloPlanificadorReady;
pthread_t hiloPlanficadorExec;
archivoConfiguracion configuracion;
t_config *config;
t_dictionary *diccionarioDeListasDeReady;
t_dictionary *diccionarioDeExec;
t_dictionary *diccionarioDeProgramas;
sem_t MAXIMOPROCESAMIENTO;
sem_t sem_diccionario_ready;
sem_t sem_programas;
sem_t hayNuevos;

t_queue *new;
t_queue *exitCola;


int main(int argc, char *argv[]){

	config = config_create(argv[1]);

	iniciarSUSE();

	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciarConexion, NULL);
	pthread_create(&hiloPlanificadorReady, NULL, (void*) planificarReady, NULL);
	pthread_create(&hiloPlanficadorExec, NULL, (void*) planificarExec, NULL);
	pthread_join(hiloLevantarConexion, NULL);
	pthread_join(hiloPlanificadorReady, NULL);
	pthread_join(hiloPlanficadorExec, NULL);
	return 0;
}

void iniciarSUSE(){
	configuracion.SEMAFOROS = dictionary_create();

	configuracion.ALPHA_SJF = config_get_double_value(config, "ALPHA_SJF");
	configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	configuracion.METRICAS_TIMER = config_get_int_value(config, "METRICS_TIMER");
	//todo: El puerto no lo tengo que levantar aca sino cuando se levanta suse
	configuracion.PUERTO = config_get_int_value(config, "LISTEN_PORT");

	//printf("%f\n", configuracion.ALPHA_SJF);

	//todo: Preguntar por la perdida de memoria en get_array_value
	int i = 0;
	while((config_get_array_value(config, "SEM_IDS"))[i] != NULL){
		semaforo *unSemaforo = malloc(sizeof(semaforo));

		unSemaforo->VALOR_ACTUAL_SEMAFORO = atoi((config_get_array_value(config, "SEM_INIT"))[i]);
		unSemaforo->VALOR_MAXIMO_SEMAFORO = atoi((config_get_array_value(config, "SEM_MAX"))[i]);
		dictionary_put(configuracion.SEMAFOROS, (config_get_array_value(config, "SEM_IDS"))[i], unSemaforo);
		i++;
	}

	new = queue_create();
	exitCola = queue_create();
	diccionarioDeListasDeReady = dictionary_create();
	diccionarioDeExec = dictionary_create();
	diccionarioDeProgramas = dictionary_create();
	sem_init(&MAXIMOPROCESAMIENTO, 0, configuracion.GRADO_DE_MULTIPROGRAMACION);
	sem_init(&sem_diccionario_ready,0,1);
	sem_init(&sem_programas,0,1);
	sem_init(&hayNuevos, 0, 0);
}

void planificarReady(){
	while(1){
		//if(!queue_is_empty(new)){
			sem_wait(&hayNuevos);
			hilo *unHilo = queue_pop(new);
			sem_wait(&MAXIMOPROCESAMIENTO);
			pasarAReady(unHilo);
		//}
	}
}

void planificarExec(){
	while(1){
		//Problema en los semaforos?
		sem_wait(&sem_programas);
		dictionary_iterator(diccionarioDeProgramas, (void*)planificarExecParaUnPrograma);
		sem_post(&sem_programas);
	}
}

void planificarExecParaUnPrograma(char *pid, programa *unPrograma){
	if(unPrograma->exec==NULL){
		//Me fijo en la lista de ready del diccionario el que sigue para ejecutar
		hilo *unHilo = siguienteDeReadyAExec(atoi(pid));
		unPrograma->exec = unHilo;
		printf("tid en exec %i\n", ((programa*)dictionary_get(diccionarioDeProgramas, pid))->exec->tid );
	}
}

hilo* siguienteDeReadyAExec(int pid){
	sem_wait(&sem_diccionario_ready);
	bool tieneMenorEstimacion(hilo *unHilo, hilo *otroHilo){
		return unHilo->estimacion < otroHilo->estimacion;
	}
	t_list *listaDeReady = list_create();

	list_add_all(listaDeReady, ((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid)))) );

	//printf("diccionario antes :%i\n", (list_size((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid)))) ) );

	list_sort(listaDeReady, (void*)tieneMenorEstimacion);

	//Lo saco de ready y lo pongo en exec

	hilo *unHilo = malloc(sizeof(hilo));
	unHilo->estimacion = ((hilo*)list_get(listaDeReady, 0))->estimacion;
	unHilo->pid = ((hilo*)list_get(listaDeReady, 0))->pid;
	unHilo->rafaga = ((hilo*)list_get(listaDeReady, 0))->rafaga;
	unHilo->tid = ((hilo*)list_get(listaDeReady, 0))->tid;

	list_remove(listaDeReady, 0);

	dictionary_remove_and_destroy(diccionarioDeListasDeReady, string_itoa(unHilo->pid), (void*)free);
	dictionary_put(diccionarioDeListasDeReady, string_itoa(unHilo->pid), listaDeReady);
	sem_post(&sem_diccionario_ready);
	return unHilo;

}

void crearHilo(int sd){

	//Tomo el sd como pid
	int pid = sd;

	int *tid = malloc(sizeof(int));
	read(sd, tid, sizeof(int));


	printf("Llamaste a create con tid: %i\n", *tid);

	hilo *unHilo = malloc(sizeof(hilo));
	unHilo->pid = pid;
	unHilo->tid = *tid;
	unHilo->estimacion = *tid;
	queue_push(new, unHilo);
	sem_post(&hayNuevos);
	free(tid);
}

void pasarAReady(hilo *unHilo){
	sem_wait(&sem_diccionario_ready);
	if(!dictionary_has_key(diccionarioDeListasDeReady, string_itoa(unHilo->pid))){
		t_list *listaDeReady = list_create();
		list_add(listaDeReady, unHilo);
		dictionary_put(diccionarioDeListasDeReady, string_itoa(unHilo->pid), listaDeReady);

		programa *unPrograma = malloc(sizeof(programa));
		unPrograma->exec = NULL;
		unPrograma->pid = unHilo->pid;
		//sem_wait(&sem_programas);
		dictionary_put(diccionarioDeProgramas, string_itoa(unPrograma->pid), unPrograma);
		//sem_post(&sem_programas);
	}
	else{
		hilo *otroHilo = malloc(sizeof(hilo));
		otroHilo->estimacion = unHilo->estimacion;
		otroHilo->pid = unHilo->pid;
		otroHilo->rafaga = unHilo->rafaga;
		otroHilo->tid = unHilo->tid;

		t_list *listaDeReadyNueva = list_create();
		list_add_all(listaDeReadyNueva, (t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(unHilo->pid))));
		list_add(listaDeReadyNueva, otroHilo);
		dictionary_remove_and_destroy(diccionarioDeListasDeReady, string_itoa(unHilo->pid), (void*)free);
		dictionary_put(diccionarioDeListasDeReady, string_itoa(unHilo->pid), listaDeReadyNueva);
		//printf("tid :%i\n", ((hilo*)list_get((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(unHilo->pid))), 0) )->tid );
	}
	sem_post(&sem_diccionario_ready);
}

//todo: optimizar la region critica
int siguienteAEjecutar(int pid){

	/*bool tieneMenorEstimacion(hilo *unHilo, hilo *otroHilo){
		return unHilo->estimacion < otroHilo->estimacion;
	}
	t_list *listaDeReady = list_create();

	sem_wait(&sem_diccionario_ready);
	list_add_all(listaDeReady, ((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid)))) );

	printf("diccionario antes :%i\n", (list_size((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid)))) ) );

	list_sort(listaDeReady, (void*)tieneMenorEstimacion);
	int tid = ((hilo*)list_get(listaDeReady, 0))->tid;

	//Lo saco de ready y lo pongo en exec

	hilo *unHilo = malloc(sizeof(hilo));
	unHilo->estimacion = ((hilo*)list_get(listaDeReady, 0))->estimacion;
	unHilo->pid = ((hilo*)list_get(listaDeReady, 0))->pid;
	unHilo->rafaga = ((hilo*)list_get(listaDeReady, 0))->rafaga;
	unHilo->tid = ((hilo*)list_get(listaDeReady, 0))->tid;
	list_remove(listaDeReady, 0);
	pasarAExec(unHilo);

	//Actualizo diccionarioDeListasDeReady con la nueva lista

	//dictionary_remove(diccionarioDeListasDeReady, string_itoa(unHilo->pid));
	dictionary_remove_and_destroy(diccionarioDeListasDeReady, string_itoa(unHilo->pid), (void*)free);
	dictionary_put(diccionarioDeListasDeReady, string_itoa(unHilo->pid), listaDeReady);

	//printf("diccionario despues :%i\n", (list_size((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid)))) ) );

	sem_post(&sem_diccionario_ready);*/

	/*
	 * La Region critica abarca hasta aca porque si mientras calculo cual es el
	 * siguiente a ejecutar, se agrega otro a ready lo podria perder
	 *
	 */

	//return tid;

	return 0;
}

/*void pasarAExec(hilo* unHilo){
	printf("---- TID: %i\n", unHilo->tid);
}*/

void actualizarEstimacion(hilo* unHilo){
	//En+1 = (1-alpha)En + alpha*Rn
	unHilo->estimacion = (1-configuracion.ALPHA_SJF)*unHilo->estimacion +
			(configuracion.ALPHA_SJF * unHilo->rafaga);
}

void sem_suse_signal (char* semID){
	if(dictionary_has_key(configuracion.SEMAFOROS, semID)){
		int valorActualSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO;
		int valorMaximoSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_MAXIMO_SEMAFORO;
		if(valorActualSemaforo+1 <= valorMaximoSemaforo){
			semaforo *unSemaforo = malloc(sizeof(semaforo));
			unSemaforo->VALOR_ACTUAL_SEMAFORO = valorActualSemaforo+1;
			unSemaforo->VALOR_MAXIMO_SEMAFORO = valorMaximoSemaforo;
			dictionary_remove_and_destroy(configuracion.SEMAFOROS, semID, (void*)free);
			dictionary_put(configuracion.SEMAFOROS, semID, unSemaforo);
		}
	}

}

void sem_suse_wait(char* semID){
	if(dictionary_has_key(configuracion.SEMAFOROS, semID)){
		int valorActualSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO;
		int valorMaximoSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_MAXIMO_SEMAFORO;
		//Se queda colgado hasta que le hagan un signal
		while(((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO==0);
		semaforo *unSemaforo = malloc(sizeof(semaforo));
		unSemaforo->VALOR_ACTUAL_SEMAFORO = valorActualSemaforo-1;
		unSemaforo->VALOR_MAXIMO_SEMAFORO = valorMaximoSemaforo;
		dictionary_remove_and_destroy(configuracion.SEMAFOROS, semID, (void*)free);
		dictionary_put(configuracion.SEMAFOROS, semID, unSemaforo);
	}
}

int32_t iniciarConexion() {
	int opt = 1;
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30,
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
	 address.sin_addr.s_addr = INADDR_ANY;
	//address.sin_addr.s_addr = inet_addr("127.0.0.1");
	//address.sin_addr.s_addr = inet_addr(structConfiguracionLFS.IP);
	address.sin_port = htons(5005);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *) &address, sizeof(address))
			< 0) {
		perror("Bind fallo en el FS");
		return 1;
	}
	printf("Escuchando en el puerto: %d \n",
			5005);

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
				int *operacion = malloc(sizeof(int));
				if ((valread = read(sd, operacion, sizeof(int))) == 0) {
					//printf("tamanio: %d", *tamanio);
					getpeername(sd, (struct sockaddr *) &address,
							(socklen_t *) &addrlen);
					printf("Host disconected, ip: %s, port: %d\n",
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;
				} else {

					switch (*operacion) {
						case 1: //suse_create
							//Tomo el sd como pid
							crearHilo(sd);
							break;
						case 2: //suse_schedule_next
							;
							int tid = siguienteAEjecutar(sd);
							char* buffer = malloc(sizeof(int));
							memcpy(buffer, &tid, sizeof(int));
							send(sd, buffer, sizeof(int), 0);
							free(buffer);
							break;
						case 3: //suse_wait
							break;
						case 4: //suse_signal
							break;
						case 5: //suse_join
							break;
						case 6: //suse_return
							break;
						default:
							break;
					}

					free(operacion);
				}
			}
		}
	}
}
