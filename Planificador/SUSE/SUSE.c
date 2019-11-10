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
#include <commons/log.h>

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
	int rafaga;
} hilo;

typedef struct {
	int pid;
	hilo *exec;
	t_dictionary *blocked;
} programa;

void iniciarSUSE();
int32_t iniciarConexion();
void planificarReady();
void sem_suse_wait(int pid, int tid, char*);
void sem_suse_signal(int pid, int tid, char*);
void pasarAReady(hilo*);
void actualizarEstimacion(hilo*);
bool tieneMenorEstimacion(hilo *unHilo, hilo *otroHilo);
void crearHilo(int);
int siguienteAEjecutar(int);
hilo* siguienteDeReadyAExec(int);
void planificarExecParaUnPrograma(char *, programa *);
void planificarExec();
void planificarBlocked();
void sacar1HiloDeLaColaDeBloqueadosPorSemaforo(int pid, char *semID);
void atenderSignal(int sd);
void atenderWait(int sd);
void atenderJoin(int sd);
void atenderClose(int sd);
void ponerEnBlockedPorHilo(int pid, hilo* hiloAPonerEnBlocked, int tidAEsperar);
void pasarAExit(hilo *unHilo);
void realizarClose(int pid, int tid);
void realizarJoin(int pid, int tidAEsperar);
unsigned long long getMicrotime();
int calcularSiguienteHilo(int pid);
void replanificarHilos(int pid);
void recalcularEstimacion(int pid);
void liberarBloqueadosPorElHilo(hilo *);
void postSemaforoMultiprogramacion();
hilo *tomarHiloBloqueadoPor(hilo *hiloBloqueante);
int bloqueadoPorAlgunHilo(hilo *hiloBloqueado);
int dictionary_algunoCumple(t_dictionary *self, int(*cumpleCondicion)(char*,void*, void*), void* parametro);
int tieneBloqueadoAlHilo(char* key, t_list* listaDeBloqueados, void* posibleBloqueado);
void tomarMetricas();
void atenderMetricas();
void limpiarEstructuras(int pid);

void tomarMetricasGradoDeMultiprogramacion();
void tomarMetricasSemaforos();
void logearValorActualSemaforo(char *semID, semaforo *unSemaforo);


pthread_t hiloLevantarConexion;
pthread_t hiloPlanificadorReady;
pthread_t hiloTomarMetricas;
archivoConfiguracion configuracion;
t_config *config;
t_dictionary *diccionarioDeListasDeReady;
t_dictionary *diccionarioDeExec;
t_dictionary *diccionarioDeBlockedPorSemaforo;
t_dictionary *diccionarioDeListasDeBlockedPorHilo;
t_dictionary *diccionarioDeListasDeExit;
t_dictionary *diccionarioDeProgramas;
sem_t MAXIMOPROCESAMIENTO;
sem_t sem_diccionario_ready;
sem_t sem_programas;
sem_t hayNuevos;
sem_t SEMAFOROS;
sem_t blockedPorSemaforo;
sem_t sem_exit;
sem_t hayQueActualizarUnExec;
sem_t semaforoConfiguracion;
t_queue *new;
t_queue *exitCola;

char* pathConfiguracion;
t_log *logger;

unsigned long long inicioRafaga = 0;


int main(int argc, char *argv[]){

	pathConfiguracion = string_new();

	if(argv[1]==NULL){
		printf("No se encontro archivo de configuracion\n");
		exit(-1);
	}
	string_append(&pathConfiguracion, argv[1]);
	config = config_create(pathConfiguracion);

	iniciarSUSE();

	//printf("%i\n", ((semaforo*) dictionary_get(configuracion.SEMAFOROS, "B"))->VALOR_ACTUAL_SEMAFORO );

	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciarConexion, NULL);
	pthread_create(&hiloPlanificadorReady, NULL, (void*) planificarReady, NULL);
	pthread_create(&hiloTomarMetricas, NULL, (void*) atenderMetricas, NULL);
	//pthread_create(&hiloPlanficadorBlocked, NULL, (void*) planificarBlocked, NULL);
	pthread_join(hiloTomarMetricas, NULL);
	pthread_join(hiloLevantarConexion, NULL);
	pthread_join(hiloPlanificadorReady, NULL);
	//pthread_join(hiloPlanficadorBlocked, NULL);
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
	//printf("%llu\n", (unsigned long long)configuracion.ALPHA_SJF);

	diccionarioDeBlockedPorSemaforo = dictionary_create();
	//todo modelar el exit
	diccionarioDeListasDeExit = dictionary_create();

	int i = 0;

	while((config_get_array_value(config, "SEM_IDS"))[i] != NULL){
		semaforo *unSemaforo = malloc(sizeof(semaforo));

		unSemaforo->VALOR_ACTUAL_SEMAFORO = atoi((config_get_array_value(config, "SEM_INIT"))[i]);
		unSemaforo->VALOR_MAXIMO_SEMAFORO = atoi((config_get_array_value(config, "SEM_MAX"))[i]);
		dictionary_put(configuracion.SEMAFOROS, (config_get_array_value(config, "SEM_IDS"))[i], unSemaforo);

		t_queue *colaParaElSemaforo = queue_create();
		dictionary_put(diccionarioDeBlockedPorSemaforo, (config_get_array_value(config, "SEM_IDS"))[i], colaParaElSemaforo);

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
	sem_init(&SEMAFOROS, 0, 1);
	sem_init(&blockedPorSemaforo, 0, 1);
	sem_init(&sem_exit, 0, 1);
	sem_init(&semaforoConfiguracion, 0, 1);
	//sem_init(&hayQueActualizarUnExec, 0, 0);
	logger = log_create("Metricas.log", "SUSE", 0, LOG_LEVEL_INFO);
}

void atenderMetricas(){
	while(1){
		sem_wait(&semaforoConfiguracion);
		config = config_create(pathConfiguracion);
		configuracion.METRICAS_TIMER = config_get_int_value(config, "METRICS_TIMER");
		sem_post(&semaforoConfiguracion);
		sleep(configuracion.METRICAS_TIMER);
		tomarMetricas();
	}
}

void tomarMetricas(){
	/*//Por cada hilo
		tomarMetricasTiemposDeEjecucion();
		tomarMetricasTiemposDeEspera();
		tomarMetricasTiemposDeUsoDeCPU();
		tomarMetricasPorcentajeDelTiempoDeEjecucion();
	//Por cada semaforo
		tomarMetricasCantidadDeHilosEnCadaEstado();*/
	//Del sistema
		tomarMetricasSemaforos();
		tomarMetricasGradoDeMultiprogramacion();
}

void tomarMetricasSemaforos(){
	sem_wait(&SEMAFOROS);
	dictionary_iterator(configuracion.SEMAFOROS, (void*) logearValorActualSemaforo);
	sem_post(&SEMAFOROS);
}

void logearValorActualSemaforo(char *semID, semaforo *unSemaforo){
	log_info(logger,"Valor actual del semaforo %s: %i", semID, unSemaforo->VALOR_ACTUAL_SEMAFORO);
}

void tomarMetricasGradoDeMultiprogramacion(){
	sem_wait(&semaforoConfiguracion);
	config = config_create(pathConfiguracion);
	configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	sem_post(&semaforoConfiguracion);
	log_info(logger,"Grado actual de multiprogramacion: %i", configuracion.GRADO_DE_MULTIPROGRAMACION);
}

//Esto es para limitar la cantidad de post que se hace sobre el semaforo
void postSemaforoMultiprogramacion(){
	int valorSemaforo;
	sem_getvalue(&MAXIMOPROCESAMIENTO, &valorSemaforo);
	sem_wait(&semaforoConfiguracion);
	config = config_create(pathConfiguracion);
	configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	sem_post(&semaforoConfiguracion);
	if(valorSemaforo < configuracion.GRADO_DE_MULTIPROGRAMACION){
		sem_post(&MAXIMOPROCESAMIENTO);
	}

}

void planificarReady(){
	while(1){
		sem_wait(&hayNuevos);
		hilo *unHilo = queue_pop(new);
		sem_wait(&MAXIMOPROCESAMIENTO);
		pasarAReady(unHilo);
	}
}

/*void planificarExec(){
	while(1){
		sem_wait(&hayQueActualizarUnExec);
		sem_wait(&sem_programas);
		dictionary_iterator(diccionarioDeProgramas, (void*)planificarExecParaUnPrograma);
		sem_post(&sem_programas);
	}
}*/


unsigned long long getMicrotime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return ((unsigned long long)( (tv.tv_sec)*1000 + (tv.tv_usec)/1000 ));
}

/*void planificarExecParaUnPrograma(char *pid, programa *unPrograma){
	if(unPrograma->exec==NULL){
		sem_wait(&sem_diccionario_ready);
		//Me fijo en la lista de ready del diccionario el que sigue para ejecutar
		hilo *unHilo = siguienteDeReadyAExec(atoi(pid));
		sem_post(&sem_diccionario_ready);

		if(unHilo!=NULL){
			//Lo elimino de las listas de ready
			list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, pid)),0);
			unPrograma->exec = unHilo;
			inicioRafaga = getMicrotime();
			printf("tid en exec %i\n", ((programa*)dictionary_get(diccionarioDeProgramas, pid))->exec->tid );
		}
		else{
			//XXX: termino el programa, limpiar todas las estructuras?
		}
	}
}*/

hilo* siguienteDeReadyAExec(int pid){
	//Como lo inserte ordenado, el primero que saco de la lista es el que va
	hilo *unHilo = (hilo*)list_get((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))), 0);

	if(unHilo==NULL){
		return NULL;
	}
	return unHilo;	//XXX: puede ser que "unHilo" necesite proteccion de semaforo?
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
	unHilo->estimacion = 0;
	unHilo->rafaga = 0;
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

		t_dictionary *diccionarioDeListasDeBlockedPorHilo = dictionary_create();

		programa *unPrograma = malloc(sizeof(programa));
		//Si es la primera vez que llega pongo en exec el hilo main
		unPrograma->exec = unHilo;
		unPrograma->pid = unHilo->pid;
		unPrograma->blocked = diccionarioDeListasDeBlockedPorHilo;

		sem_wait(&sem_programas);
		dictionary_put(diccionarioDeProgramas, string_itoa(unPrograma->pid), unPrograma);
		//sem_post(&hayQueActualizarUnExec);
		sem_post(&sem_programas);
	}
	else{

		//Lo agrego a la lista correspondiente
		list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(unHilo->pid))), unHilo);

		//Ordeno la lista por el que tiene menor estimacion
		//list_sort((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(unHilo->pid))) , (void*)tieneMenorEstimacion);

		//printf("1° tid en ready:%i\n", ((hilo*)list_get((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(unHilo->pid))), 0) )->tid );
	}
	sem_post(&sem_diccionario_ready);
}

//Esta es la funcion que se invoca cuando se llama a schedule_next
int siguienteAEjecutar(int pid){

	recalcularEstimacion(pid);
	replanificarHilos(pid);

	sem_wait(&sem_diccionario_ready);
	sem_wait(&sem_programas);
	int siguienteTIDAEjecutar = calcularSiguienteHilo(pid);

	sem_post(&sem_diccionario_ready);
	sem_post(&sem_programas);

	//printf("tid a ejec. %i\n", siguienteTIDAEjecutar);
	return siguienteTIDAEjecutar;
	//printf("microtime: %f\n", (float)getMicrotime());
	//return ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid;
}

int calcularSiguienteHilo(int pid){

	hilo *candidatoAEjecutar = ((hilo*)list_get((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))), 0) );

	//El programa existe cuando algun hilo paso a ready en algun momento
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, string_itoa(pid));

	hilo *elQueEstaEjecutando = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec;

	if(existeElPrograma && elQueEstaEjecutando != NULL){

		//Si el que esta en ready tiene menos ejecucion ese sigue, sino el que estaba en exec
		if(candidatoAEjecutar!=NULL && candidatoAEjecutar->estimacion < ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->estimacion){
			//printf("TID en exec: %i - Estimacion: %f\n TID candidato: %i - Estimacion: %f\n\n", elQueEstaEjecutando->tid, elQueEstaEjecutando->estimacion, candidatoAEjecutar->tid, candidatoAEjecutar->estimacion);
			//Paso a ready el que esta ejecutando y a exec el que estaba en ready (sacandolo de ready)
			candidatoAEjecutar = ((hilo*)list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))), 0) );
			list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))), elQueEstaEjecutando);
			((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec = candidatoAEjecutar;
			inicioRafaga = getMicrotime();
			return candidatoAEjecutar->tid;
		}
		else{
			return elQueEstaEjecutando->tid;
		}
	}

	//Si no hay ninguno en exec paso el siguiente
	candidatoAEjecutar = ((hilo*)list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))), 0) );
	((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec = candidatoAEjecutar;
	inicioRafaga = getMicrotime();
	postSemaforoMultiprogramacion();
	//printf("ES NULL %i\n", candidatoAEjecutar==NULL);
	return candidatoAEjecutar->tid;
}

void replanificarHilos(int pid){
	sem_wait(&sem_diccionario_ready);
	//Ordeno la lista por el que tiene menor estimacion
	list_sort((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))) , (void*)tieneMenorEstimacion);
	sem_post(&sem_diccionario_ready);
}

void recalcularEstimacion(int pid){

	sem_wait(&sem_programas);
	//El programa existe cuando algun hilo paso a ready en algun momento
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, string_itoa(pid));

	//Si existe el programa y hay alguien en ejecucion
	if(existeElPrograma && ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec != NULL){
		int loQueTardo = getMicrotime() - inicioRafaga;
		//Actualizo la rafaga y la estimacion del que esta en ejecucion
		//printf("Rafaga del tid %i :%i\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid, loQueTardo);
		((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->rafaga = loQueTardo;
		actualizarEstimacion( ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec );
		//printf("Estimacion del tid %i :%f\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid, ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->estimacion);

	}
		//printf("TID: %i - Rafaga: %f\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid,((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->rafaga);
	sem_post(&sem_programas);

	//No hace falta calcular la estimacion de los demas hilos
	/*sem_wait(&sem_diccionario_ready);
	//Actualizo la estimacion de todos los que estan en ready
	list_iterate( (t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(pid))) , (void*)actualizarEstimacion);
	sem_post(&sem_diccionario_ready);*/

}


void actualizarEstimacion(hilo* unHilo){

	sem_wait(&semaforoConfiguracion);
	config = config_create(pathConfiguracion);
	configuracion.ALPHA_SJF = config_get_double_value(config, "ALPHA_SJF");
	sem_post(&semaforoConfiguracion);

	//En+1 = (1-alpha)En + alpha*Rn
	unHilo->estimacion = (1-configuracion.ALPHA_SJF)*unHilo->estimacion +
			(configuracion.ALPHA_SJF * unHilo->rafaga);
}

bool tieneMenorEstimacion(hilo *unHilo, hilo *otroHilo){
	return unHilo->estimacion <= otroHilo->estimacion;
}

//todo: preguntar si sirve de algo el tid en el signal
void sem_suse_signal (int pid, int tid, char* semID){
	sem_wait(&SEMAFOROS);
	if(dictionary_has_key(configuracion.SEMAFOROS, semID)){
		int valorActualSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO;
		int valorMaximoSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_MAXIMO_SEMAFORO;
		if(valorActualSemaforo+1 <= valorMaximoSemaforo){
			sem_wait(&blockedPorSemaforo);
			int noHayNadieBloqueado =
					queue_is_empty((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID));
			if(noHayNadieBloqueado){
				((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO++;
			}
			else{
				sacar1HiloDeLaColaDeBloqueadosPorSemaforo(pid, semID);
			}
			sem_post(&blockedPorSemaforo);
		}
	}
	sem_post(&SEMAFOROS);
}

void sem_suse_wait(int pid, int tid, char* semID){
	sem_wait(&SEMAFOROS);
	if(dictionary_has_key(configuracion.SEMAFOROS, semID)){
		int valorActualSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO;
		//int valorMaximoSemaforo = ((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_MAXIMO_SEMAFORO;

		if(valorActualSemaforo > 0){
			((semaforo*)dictionary_get(configuracion.SEMAFOROS, semID))->VALOR_ACTUAL_SEMAFORO--;
		}

		else{

			if( ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid == tid ){
				sem_wait(&blockedPorSemaforo);
				//Mandar el hilo a blocked por semaforo
				queue_push((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID), ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec);
				sem_post(&blockedPorSemaforo);
				//Lo saco de ejecucion
				((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec = NULL;
				postSemaforoMultiprogramacion();
				//sem_post(&hayQueActualizarUnExec);
			}
			else{
				printf("El hilo en ejecucion no pidio el wait\n");
				exit(-1);
			}
		}

	}
	sem_post(&SEMAFOROS);
}

//Saco del diccionario en la cola de semID el primero y lo paso a la cola de ready correspondiente
void sacar1HiloDeLaColaDeBloqueadosPorSemaforo(int pid, char *semID){
	//Si esta vacia no hace nada
	if( !queue_is_empty((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID)) ){
		pasarAReady( queue_pop((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID)) );
	}
}

void realizarJoin(int pid, int tidAEsperar){
	sem_wait(&sem_programas);
	//printf("pediste join\n");
	/*
	 * Puede pasar que todavia no haya llegado a ready alguno de los hilos de mi programa
	 * por lo que no se crearia mi clave para ese programa dentro del diccionarioDeProgramas
	 * y al querer ver si hay alguien en exec valgrind chilla, por eso primero me fijo si existe
	 * el programa y despues si hay alguien en exec para hacer el cambio
	 */
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, string_itoa(pid));
	int hayAlguienEnExec = existeElPrograma && ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec != NULL;
	printf("existeElPrograma %i - hayAlguienEnExec %i\n", existeElPrograma, hayAlguienEnExec);
	if(hayAlguienEnExec){
		//printf("tid en exec: %i\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid);
		//Pongo el que esta ejecutando en blockeado
		ponerEnBlockedPorHilo(pid, ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec, tidAEsperar);
		//Lo saco de exec
		((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec = NULL;
		postSemaforoMultiprogramacion();
		//sem_post(&hayQueActualizarUnExec);
	}
	sem_post(&sem_programas);
}

void realizarClose(int pid, int tid){
	if( ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid == tid ){
		hilo *elQueEstaEjecutando = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec;
		//Pongo el que esta ejecutando en exit
		pasarAExit(elQueEstaEjecutando);
		//Lo saco de exec
		((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec = NULL;
		postSemaforoMultiprogramacion();
		//sem_post(&hayQueActualizarUnExec);

		//Libero la lista de los bloqueados por este hilo que termino
		liberarBloqueadosPorElHilo(elQueEstaEjecutando);
		/*pthread_t hiloLiberarBloqueados;
		printf("CLOSE1\n");
		pthread_create(&hiloLiberarBloqueados, NULL, (void*) liberarBloqueadosPorElHilo,
								(void *) elQueEstaEjecutando);
		printf("CLOSE2\n");
		pthread_detach(hiloLiberarBloqueados);*/
	}
	else{
		//XXX se fija en los de ready, bloqueados, etc?
	}

}

void pasarAExit(hilo *unHilo){
	sem_wait(&sem_exit);
	//Si ya existe una entrada para el proceso en la lista de exit, lo agrego a su lista
	if(!dictionary_has_key(diccionarioDeListasDeExit, string_itoa(unHilo->pid))){
			t_list *listaDeExit = list_create();
			list_add(listaDeExit, unHilo);
			dictionary_put(diccionarioDeListasDeExit, string_itoa(unHilo->pid), listaDeExit);
	}
	else{
		//Lo agrego a la lista correspondiente
		list_add((t_list*)(dictionary_get(diccionarioDeListasDeExit, string_itoa(unHilo->pid))), unHilo);
	}
	sem_post(&sem_exit);
}

//todo puede haber mas de 1 hilo bloqueado por 1 mismo hilo. Hacer una lista aca
void ponerEnBlockedPorHilo(int pid, hilo* hiloAPonerEnBlocked, int tidAEsperar){
	t_dictionary *diccionarioDeBloqueados = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->blocked;
	if( !dictionary_has_key(diccionarioDeBloqueados, string_itoa(tidAEsperar)) ){
		t_list *listaDeBloqueados = list_create();
		dictionary_put(diccionarioDeBloqueados, string_itoa(tidAEsperar), listaDeBloqueados);
	}
	//printf("hiloAPonerEnBlocked %i - tidAEsperar %i\n", hiloAPonerEnBlocked->tid, tidAEsperar),
	list_add((t_list*)dictionary_get(diccionarioDeBloqueados, string_itoa(tidAEsperar)), hiloAPonerEnBlocked);
}

//fixme estos semaforos estan raros
void liberarBloqueadosPorElHilo(hilo *hiloBloqueante){
	sem_wait(&sem_programas);
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(hiloBloqueante->pid)))->blocked;
	t_list *listaDeBloqueados = (t_list *)dictionary_get(diccionarioDeBloqueadosPorProceso, string_itoa(hiloBloqueante->tid));

	//Puede ser que no se haya bloqueado ningun hilo nunca todavia para ese proceso
	if(listaDeBloqueados!=NULL){

		while(!list_is_empty(listaDeBloqueados)){
			hilo *hiloBloqueado = tomarHiloBloqueadoPor(hiloBloqueante);
			//sem_post(&sem_programas);
			/*
			 * Si al hilo no lo bloquea ningun otro hilo lo mando a ready, sino no solo lo saco de
			 * la lista de bloqueados por el hiloBloqueante
			 */
			if(!bloqueadoPorAlgunHilo(hiloBloqueado)){
				sem_wait(&MAXIMOPROCESAMIENTO);
				sem_wait(&sem_diccionario_ready);
				//Lo agrego a ready
				list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, string_itoa(hiloBloqueado->pid))), hiloBloqueado);
				printf(" liberando tid bloqueado %i\n", hiloBloqueado->tid);
				sem_post(&sem_diccionario_ready);
			}

			//sem_wait(&sem_programas);
		}

	}

	sem_post(&sem_programas);
}

hilo *tomarHiloBloqueadoPor(hilo *hiloBloqueante){
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(hiloBloqueante->pid)))->blocked;
	t_list *listaDeBloqueados = (t_list *)dictionary_get(diccionarioDeBloqueadosPorProceso, string_itoa(hiloBloqueante->tid));
	return (hilo*)list_remove(listaDeBloqueados, 0);
}

int bloqueadoPorAlgunHilo(hilo *hiloBloqueado){
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(hiloBloqueado->pid)))->blocked;
	int algunoLoBloquea = dictionary_algunoCumple(diccionarioDeBloqueadosPorProceso, (void *) tieneBloqueadoAlHilo, (void *) hiloBloqueado);
	return algunoLoBloquea;
}


int dictionary_algunoCumple(t_dictionary *self, int(*cumpleCondicion)(char*,void*, void*), void* parametro) {
	int table_index;
	for (table_index = 0; table_index < self->table_max_size; table_index++) {
		t_hash_element *element = self->elements[table_index];

		while (element != NULL) {

			if(cumpleCondicion(element->key, element->data, parametro)){
				//printf("key: %s\n", element->key);
				return 1;
			}
			element = element->next;

		}
	}
	return 0;
}

int tieneBloqueadoAlHilo(char* key, t_list* listaDeBloqueados, void* posibleBloqueado){
	bool estaElHilo(void* unHiloBloqueado){
		return ((hilo*)unHiloBloqueado)->tid == ((hilo*)posibleBloqueado)->tid;
	}
	return list_any_satisfy(listaDeBloqueados, estaElHilo);;
}


void atenderWait(int sd){
	int *tid = malloc(sizeof(int));
	read(sd, tid, sizeof(int));
	int *longitudIDSemaforo= malloc(sizeof(int));
	read(sd, longitudIDSemaforo, sizeof(int));
	char *semID = malloc(*longitudIDSemaforo+1);
	read(sd, semID, *longitudIDSemaforo+1);
	sem_suse_wait(sd, *tid, semID);
	free(tid);
	free(longitudIDSemaforo);
	free(semID);
}

void atenderSignal(int sd){
	int *tid = malloc(sizeof(int));
	read(sd, tid, sizeof(int));
	int *longitudIDSemaforo= malloc(sizeof(int));
	read(sd, longitudIDSemaforo, sizeof(int));
	char *semID = malloc(*longitudIDSemaforo+1);
	read(sd, semID, *longitudIDSemaforo+1);
	sem_suse_signal(sd, *tid, semID);
	free(tid);
	free(longitudIDSemaforo);
	free(semID);
}

void atenderJoin(int sd){
	int *tidAEsperar = malloc(sizeof(int));
	read(sd, tidAEsperar, sizeof(int));
	realizarJoin(sd, *tidAEsperar);
	free(tidAEsperar);
}

void atenderClose(int sd){
	int *tidACerrar = malloc(sizeof(int));
	read(sd, tidACerrar, sizeof(int));
	realizarClose(sd, *tidACerrar);
	free(tidACerrar);
}

void limpiarEstructuras(int pid){
	//limpiarDiccionarioDeListasDeExit
	dictionary_remove(diccionarioDeListasDeReady, string_itoa(pid));
	dictionary_remove(diccionarioDeExec, string_itoa(pid));
	dictionary_remove(diccionarioDeProgramas, string_itoa(pid));
}

/*void sincronizarJoinYSchedule(int pid){
	sem_wait(&sem_programas);
	programa *elPrograma = ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)));
	if(!elPrograma->yaHizoScheduleNext1Vez){
		sem_post(elPrograma->sincronizadorJoinYScheduleNext);
		elPrograma->yaHizoScheduleNext1Vez = 1;
	}
	sem_post(&sem_programas);

}*/

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

					limpiarEstructuras(sd);
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
							//sincronizarJoinYSchedule(sd);
							free(buffer);
							break;
						case 3: //suse_wait
							atenderWait(sd);
							break;
						case 4: //suse_signal
							atenderSignal(sd);
							break;
						case 5: //suse_join
							atenderJoin(sd);
							break;
						case 6: //suse_close
							atenderClose(sd);
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
