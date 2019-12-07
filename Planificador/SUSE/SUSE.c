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
	char *IP_SUSE;
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

	int tiempoDeEjecucion;
	int timestampCreacion;
	int sumaDeIntervalosEnReady;
	int sumaDeIntervalosEnExec;

	unsigned long long timestampExec;
	unsigned long long timestampReady;

} hilo;

typedef struct {
	bool estaBloqueado;
	pthread_mutex_t mutexProgama;
	int pid;
	hilo *exec;
	t_dictionary *blocked;

	int cantidadDeHilosEnNew;
	int cantidadDeHilosEnReady;
	int cantidadDeHilosEnBlocked;

	int tiempoDeEjecucionDeTodosLosHilos;

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
void sacar1HiloDeLaColaDeBloqueadosPorSemaforo(int pid, char *semID);
void atenderSignal(int sd);
void atenderWait(int sd);
void atenderJoin(int sd);
void atenderClose(int sd);
void atenderScheduleNext(int sd);
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

int terminoElHilo(int pid, int tid);
int estaEnExit(char* key, t_list* listaDeExit, void* tidAEsperar);

void tomarMetricasParaHilos();
void reiniciarTiempoDeEjecucionDe1Programa(char *pid, void *unPrograma);
void reiniciarTiempoDeEjecucionTotal();

void tomarMetricasPorcentajeDeEjecucionPara1Hilo(void *unHilo);
void tomarMetricasPorcentajeDeEjecucionParaSem(char *semID, t_queue *cola);
void tomarMetricasPorcentajeDeEjecucionParaBlocked(char* pid, void *unPrograma);
void tomarMetricasPorcentajeDeEjecucionParaReady(char* pid, void *unPrograma);
void tomarMetricasPorcentajeDeEjecucionParaExec(char* pid, void *unPrograma);
void tomarMetricasPorcentajeParaLaLista(char *key, void* lista);

void tomarMetricasTiemposDeCpuParaLaLista(char *key, void* lista);
void tomarMetricasTiemposDeCpuParaExec(char* pid, void *unPrograma);
void tomarMetricasTiemposDeCpuParaReady(char* pid, void *unPrograma);
void tomarMetricasTiemposDeCpuParaBlocked(char* pid, void *unPrograma);
void tomarMetricasTiemposDeCpuParaSem(char *semID, t_queue *cola);
void tomarMetricasTiemposDeCpuPara1Hilo(void *unHilo);

void tomarMetricasTiemposDeEsperaParaSem(char *semID, t_queue *cola);
void tomarMetricasTiemposDeEsperaParaExec(char* pid, void *unPrograma);
void tomarMetricasTiemposDeEsperaParaLaLista(char *key, void* lista);
void tomarMetricasTiemposDeEsperaParaReady(char* pid, void *unPrograma);
void tomarMetricasTiemposDeEsperaParaBlocked(char* pid, void *unPrograma);
void tomarMetricasTiemposDeEsperaPara1Hilo(void *unHilo);

void tomarMetricasTiemposDeEjecucionPara1Hilo(void *unHilo);
void tomarMetricasTiemposDeEjecucionParaSem(char *semID, t_queue *cola);
void tomarMetricasParaLosNews(void (*criterioMetrica)(void*) );
void tomarMetricasParaLosReady(void (*criterioMetrica)(char*, void*) );
void tomarMetricasTiemposDeEjecucionParaLaLista(char *key, void* listaDeBloqueados);
void tomarMetricasTiemposDeEjecucionParaReady(char* pid, void *unPrograma);
void tomarMetricasTiemposDeEjecucionParaBlocked(char* pid, void *unPrograma);
void tomarMetricasTiemposDeEjecucionParaExec(char* pid, void *unPrograma);
void tomarMetricasParaLosProcesos(void (*criterioMetrica)(char*, void*) );
void tomarMetricasParaBloqueadosPorSem(void (*criterioMetrica)(char*, void*) );

void tomarMetricasGradoDeMultiprogramacion();
void tomarMetricasSemaforos();
void logearValorActualSemaforo(char *semID, semaforo *unSemaforo);
void tomarMetricasCantidadDeHilosEnCadaEstado();
void tomarMetricasPorProceso(char *key, void *unPrograma);

void loguearInfo(char* texto);
void loguearTransicion(int pid, int tid, char *estadoATransicionar);

pthread_t hiloLevantarConexion;
pthread_t hiloPlanificadorReady;
pthread_t hiloTomarMetricas;
archivoConfiguracion configuracion;
t_config *config;
t_dictionary *diccionarioDeListasDeReady;
//t_dictionary *diccionarioDeExec;
t_dictionary *diccionarioDeBlockedPorSemaforo;
t_dictionary *diccionarioDeListasDeExit;
t_dictionary *diccionarioDeProgramas;
sem_t MAXIMOPROCESAMIENTO;
sem_t sem_diccionario_ready;
sem_t sem_programas;
sem_t hayNuevos;
sem_t SEMAFOROS;
sem_t blockedPorSemaforo;
sem_t sem_new;
sem_t sem_exit;
sem_t hayQueActualizarUnExec;
sem_t semaforoConfiguracion;
sem_t sem_metricas;
sem_t transicionesLog;
sem_t semScheduleNext;
t_queue *new;

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
	free(pathConfiguracion); //XXX: Si tengo que actualizar los valores, borrar este free

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

void loguearTransicion(int pid, int tid, char *estadoATransicionar){
	char *unPid = string_itoa(pid);
	char *unTid = string_itoa(tid);
	char *mensajeALoguear = string_new();
	string_append(&mensajeALoguear, "El hilo ");
	string_append(&mensajeALoguear, unTid);
	string_append(&mensajeALoguear, " del programa ");
	string_append(&mensajeALoguear, unPid);
	string_append(&mensajeALoguear, " paso a ");
	string_append(&mensajeALoguear, estadoATransicionar);
	loguearInfo(mensajeALoguear);
	free(unPid);
	free(unTid);
	free(mensajeALoguear);
}

void loguearInfo(char* texto) {
	char* mensajeALogear = malloc(strlen(texto) + 1);
	strcpy(mensajeALogear, texto);
	t_log* g_logger;
	sem_wait(&transicionesLog);
	g_logger = log_create("Transiciones.log", "SUSE", 1, LOG_LEVEL_INFO);
	log_info(g_logger, mensajeALogear);
	sem_post(&transicionesLog);
	log_destroy(g_logger);
	free(mensajeALogear);
}

void iniciarSUSE(){
	configuracion.IP_SUSE = malloc(strlen(config_get_string_value(config, "IP_SUSE"))+1);
	strcpy(configuracion.IP_SUSE, config_get_string_value(config, "IP_SUSE"));

	configuracion.SEMAFOROS = dictionary_create();

	configuracion.ALPHA_SJF = config_get_double_value(config, "ALPHA_SJF");
	configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	configuracion.METRICAS_TIMER = config_get_int_value(config, "METRICS_TIMER");
	//todo: El puerto no lo tengo que levantar aca sino cuando se levanta suse
	configuracion.PUERTO = config_get_int_value(config, "LISTEN_PORT");

	//printf("%f\n", configuracion.ALPHA_SJF);
	//printf("%llu\n", (unsigned long long)configuracion.ALPHA_SJF);

	diccionarioDeBlockedPorSemaforo = dictionary_create();
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
	diccionarioDeListasDeReady = dictionary_create();
	//diccionarioDeExec = dictionary_create();
	diccionarioDeProgramas = dictionary_create();
	sem_init(&MAXIMOPROCESAMIENTO, 0, configuracion.GRADO_DE_MULTIPROGRAMACION);
	sem_init(&sem_diccionario_ready,0,1);
	sem_init(&sem_programas,0,1);
	sem_init(&hayNuevos, 0, 0);
	sem_init(&SEMAFOROS, 0, 1);
	sem_init(&blockedPorSemaforo, 0, 1);
	sem_init(&sem_exit, 0, 1);
	sem_init(&semaforoConfiguracion, 0, 1);
	sem_init(&sem_new, 0, 1);
	sem_init(&sem_metricas, 0, 1);
	sem_init(&transicionesLog, 0, 1);
	sem_init(&semScheduleNext, 0, 1);
	logger = log_create("Metricas.log", "SUSE", 0, LOG_LEVEL_INFO);
}

void atenderMetricas(){
	while(1){
		/*sem_wait(&semaforoConfiguracion);
		//config = config_create(pathConfiguracion);
		//configuracion.METRICAS_TIMER = config_get_int_value(config, "METRICS_TIMER");
		sem_post(&semaforoConfiguracion);*/
		sleep(configuracion.METRICAS_TIMER);
		sem_wait(&sem_metricas);
		tomarMetricas();
		sem_post(&sem_metricas);
	}
}

void tomarMetricas(){
	//Por cada hilo
		tomarMetricasParaHilos();
	//Por cada programa
		tomarMetricasCantidadDeHilosEnCadaEstado();
	//Del sistema
		tomarMetricasSemaforos();
		tomarMetricasGradoDeMultiprogramacion();
}

void tomarMetricasParaHilos(){
	//Tiempo de ejecucion
	tomarMetricasParaLosNews((void *) tomarMetricasTiemposDeEjecucionPara1Hilo);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeEjecucionParaBlocked);
	tomarMetricasParaLosReady((void *) tomarMetricasTiemposDeEjecucionParaReady);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeEjecucionParaExec);
	tomarMetricasParaBloqueadosPorSem((void *) tomarMetricasTiemposDeEjecucionParaSem);

	//Tiempo de espera
	tomarMetricasParaLosNews((void *) tomarMetricasTiemposDeEsperaPara1Hilo);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeEsperaParaBlocked);
	tomarMetricasParaLosReady((void *) tomarMetricasTiemposDeEsperaParaReady);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeEsperaParaExec);
	tomarMetricasParaBloqueadosPorSem((void *) tomarMetricasTiemposDeEsperaParaSem);

	//Tiempo de uso de CPU
	/*tomarMetricasParaLosNews((void *) tomarMetricasTiemposDeCpuPara1Hilo);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeCpuParaBlocked);
	tomarMetricasParaLosReady((void *) tomarMetricasTiemposDeCpuParaReady);
	tomarMetricasParaLosProcesos((void *) tomarMetricasTiemposDeCpuParaExec);
	tomarMetricasParaBloqueadosPorSem((void *) tomarMetricasTiemposDeCpuParaSem);*/

	//Porcentaje tiempo de ejecucion
	tomarMetricasParaLosNews((void *) tomarMetricasPorcentajeDeEjecucionPara1Hilo);
	tomarMetricasParaLosProcesos((void *) tomarMetricasPorcentajeDeEjecucionParaBlocked);
	tomarMetricasParaLosReady((void *) tomarMetricasPorcentajeDeEjecucionParaReady);
	tomarMetricasParaLosProcesos((void *) tomarMetricasPorcentajeDeEjecucionParaExec);
	tomarMetricasParaBloqueadosPorSem((void *) tomarMetricasPorcentajeDeEjecucionParaSem);

	reiniciarTiempoDeEjecucionTotal();
}

void reiniciarTiempoDeEjecucionTotal(){
	dictionary_iterator(diccionarioDeProgramas, (void*)reiniciarTiempoDeEjecucionDe1Programa);
}

void reiniciarTiempoDeEjecucionDe1Programa(char *pid, void *unPrograma){
	char *unPid = string_itoa(((programa*)unPrograma)->pid);
	sem_wait(&sem_programas);
	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->tiempoDeEjecucionDeTodosLosHilos = 0;
	sem_post(&sem_programas);
	free(unPid);
}

//Porcentaje tiempo de ejecucion
void tomarMetricasPorcentajeDeEjecucionPara1Hilo(void *unHilo){
	char *unPid = string_itoa(((hilo*)unHilo)->pid);
	int tiempoDeEjecucionDelHilo = ((hilo*)unHilo)->tiempoDeEjecucion;
	int tiempoDeEjecucionDeTodosLosHilosDelPrograma = 	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->tiempoDeEjecucionDeTodosLosHilos;
	int porcentajeDeEjecucion = ((float)tiempoDeEjecucionDelHilo / (float)tiempoDeEjecucionDeTodosLosHilosDelPrograma)*100;
	log_info(logger,"Porcentaje del tiempo de ejecucion para el hilo %i del programa %i: %i %%", ((hilo*)unHilo)->tid, ((hilo*)unHilo)->pid, porcentajeDeEjecucion);
	free(unPid);
}

void tomarMetricasPorcentajeDeEjecucionParaSem(char *semID, t_queue *cola){
	t_queue *colaAux = queue_create();
	hilo *unHilo = queue_pop(new);
	while (unHilo != NULL) {
		tomarMetricasPorcentajeDeEjecucionPara1Hilo(unHilo);
		queue_push(colaAux, unHilo);
		unHilo = queue_pop(new);
	}
	unHilo = queue_pop(colaAux);
	while (unHilo != NULL) {
		queue_push(new, unHilo);
		unHilo = queue_pop(colaAux);
	}
	queue_destroy(colaAux);
}

void tomarMetricasPorcentajeDeEjecucionParaBlocked(char* pid, void *unPrograma){
	dictionary_iterator(((programa*)unPrograma)->blocked, tomarMetricasPorcentajeParaLaLista);
}

void tomarMetricasPorcentajeDeEjecucionParaReady(char* pid, void *unPrograma){
	dictionary_iterator(diccionarioDeListasDeReady, tomarMetricasPorcentajeParaLaLista);
}

void tomarMetricasPorcentajeDeEjecucionParaExec(char* pid, void *unPrograma){
	if(((programa*)unPrograma)->exec!=NULL){
		tomarMetricasPorcentajeDeEjecucionPara1Hilo(((programa*)unPrograma)->exec);
	}
}

void tomarMetricasPorcentajeParaLaLista(char *key, void* lista){
	list_iterate((t_list*)lista, tomarMetricasPorcentajeDeEjecucionPara1Hilo);
}

//Tiempo de uso de CPU
void tomarMetricasTiemposDeCpuPara1Hilo(void *unHilo){
	log_info(logger,"Tiempo de uso de CPU para el hilo %i del programa %i: %i",
			((hilo*)unHilo)->tid,
			((hilo*)unHilo)->pid,
			((hilo*)unHilo)->sumaDeIntervalosEnExec);
}

void tomarMetricasTiemposDeCpuParaSem(char *semID, t_queue *cola){
	t_queue *colaAux = queue_create();
	hilo *unHilo = queue_pop(new);
	while (unHilo != NULL) {
		tomarMetricasTiemposDeCpuPara1Hilo(unHilo);
		queue_push(colaAux, unHilo);
		unHilo = queue_pop(new);
	}
	unHilo = queue_pop(colaAux);
	while (unHilo != NULL) {
		queue_push(new, unHilo);
		unHilo = queue_pop(colaAux);
	}
	queue_destroy(colaAux);
}

void tomarMetricasTiemposDeCpuParaBlocked(char* pid, void *unPrograma){
	dictionary_iterator(((programa*)unPrograma)->blocked, tomarMetricasTiemposDeCpuParaLaLista);
}

void tomarMetricasTiemposDeCpuParaReady(char* pid, void *unPrograma){
	dictionary_iterator(diccionarioDeListasDeReady, tomarMetricasTiemposDeCpuParaLaLista);
}

void tomarMetricasTiemposDeCpuParaExec(char* pid, void *unPrograma){
	if(((programa*)unPrograma)->exec!=NULL){
		tomarMetricasTiemposDeCpuPara1Hilo(((programa*)unPrograma)->exec);
	}
}

void tomarMetricasTiemposDeCpuParaLaLista(char *key, void* lista){
	list_iterate((t_list*)lista, tomarMetricasTiemposDeCpuPara1Hilo);
}

//Tiempo de espera
void tomarMetricasTiemposDeEsperaPara1Hilo(void *unHilo){
	log_info(logger,"Tiempo de espera para el hilo %i del programa %i: %i", ((hilo*)unHilo)->tid, ((hilo*)unHilo)->pid, ((hilo*)unHilo)->sumaDeIntervalosEnReady);
}

void tomarMetricasTiemposDeEsperaParaSem(char *semID, t_queue *cola){
	t_queue *colaAux = queue_create();
	hilo *unHilo = queue_pop(new);
	while (unHilo != NULL) {
		tomarMetricasTiemposDeEsperaPara1Hilo(unHilo);
		queue_push(colaAux, unHilo);
		unHilo = queue_pop(new);
	}
	unHilo = queue_pop(colaAux);
	while (unHilo != NULL) {
		queue_push(new, unHilo);
		unHilo = queue_pop(colaAux);
	}
	queue_destroy(colaAux);
}

void tomarMetricasTiemposDeEsperaParaBlocked(char* pid, void *unPrograma){
	dictionary_iterator(((programa*)unPrograma)->blocked, tomarMetricasTiemposDeEsperaParaLaLista);
}

void tomarMetricasTiemposDeEsperaParaReady(char* pid, void *unPrograma){
	dictionary_iterator(diccionarioDeListasDeReady, tomarMetricasTiemposDeEsperaParaLaLista);
}

void tomarMetricasTiemposDeEsperaParaLaLista(char *key, void* lista){
	list_iterate((t_list*)lista, tomarMetricasTiemposDeEsperaPara1Hilo);
}

void tomarMetricasTiemposDeEsperaParaExec(char* pid, void *unPrograma){
	if(((programa*)unPrograma)->exec!=NULL){
		tomarMetricasTiemposDeEsperaPara1Hilo(((programa*)unPrograma)->exec);
	}
}

//Tiempo de ejecucion
void tomarMetricasTiemposDeEjecucionPara1Hilo(void *unHilo){
	int tiempoDeEjecucion = getMicrotime() - ((hilo*)unHilo)->timestampCreacion;
	((hilo*) unHilo)->tiempoDeEjecucion = tiempoDeEjecucion;
	log_info(logger,"Tiempo de ejecucion para el hilo %i del programa %i: %i", ((hilo*)unHilo)->tid, ((hilo*)unHilo)->pid, tiempoDeEjecucion);
	char *unPid = string_itoa(((hilo*)unHilo)->pid);
	//Acumulo en el programa el tiempo de ejecucion de cada hilo para calcuar el porcentaje
	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->tiempoDeEjecucionDeTodosLosHilos += tiempoDeEjecucion;
	free(unPid);
}

void tomarMetricasTiemposDeEjecucionParaSem(char *semID, t_queue *cola){
	t_queue *colaAux = queue_create();
	hilo *unHilo = queue_pop(new);
	while (unHilo != NULL) {
		tomarMetricasTiemposDeEjecucionPara1Hilo(unHilo);
		queue_push(colaAux, unHilo);
		unHilo = queue_pop(new);
	}
	unHilo = queue_pop(colaAux);
	while (unHilo != NULL) {
		queue_push(new, unHilo);
		unHilo = queue_pop(colaAux);
	}
	queue_destroy(colaAux);
}

void tomarMetricasTiemposDeEjecucionParaBlocked(char* pid, void *unPrograma){
	dictionary_iterator(((programa*)unPrograma)->blocked, tomarMetricasTiemposDeEjecucionParaLaLista);
}

void tomarMetricasTiemposDeEjecucionParaExec(char* pid, void *unPrograma){
	if(((programa*)unPrograma)->exec!=NULL){
		tomarMetricasTiemposDeEjecucionPara1Hilo(((programa*)unPrograma)->exec);
	}
}

void tomarMetricasTiemposDeEjecucionParaReady(char* pid, void *unPrograma){
	dictionary_iterator(diccionarioDeListasDeReady, tomarMetricasTiemposDeEjecucionParaLaLista);
}

void tomarMetricasTiemposDeEjecucionParaLaLista(char *key, void* lista){
	list_iterate((t_list*)lista, tomarMetricasTiemposDeEjecucionPara1Hilo);
}

void tomarMetricasParaLosProcesos(void (*criterioMetrica)(char*, void*) ){
	sem_wait(&sem_programas);
	dictionary_iterator(diccionarioDeProgramas, criterioMetrica);
	sem_post(&sem_programas);
}


void tomarMetricasParaBloqueadosPorSem(void (*criterioMetrica)(char*, void*) ){
	sem_wait(&blockedPorSemaforo);
	dictionary_iterator(diccionarioDeBlockedPorSemaforo, criterioMetrica);
	sem_post(&blockedPorSemaforo);
}

void tomarMetricasParaLosReady(void (*criterioMetrica)(char*, void*) ){
	sem_wait(&sem_diccionario_ready);
	dictionary_iterator(diccionarioDeListasDeReady, criterioMetrica);
	sem_post(&sem_diccionario_ready);
}

void tomarMetricasParaLosNews(void (*criterioMetrica)(void*) ){
	sem_wait(&sem_new);
	t_queue *colaAux = queue_create();
	hilo *unHilo = queue_pop(new);
	while (unHilo != NULL) {
		criterioMetrica(unHilo);
		queue_push(colaAux, unHilo);
		unHilo = queue_pop(new);
	}
	unHilo = queue_pop(colaAux);
	while (unHilo != NULL) {
		queue_push(new, unHilo);
		unHilo = queue_pop(colaAux);
	}
	queue_destroy(colaAux);
	sem_post(&sem_new);
}

void tomarMetricasCantidadDeHilosEnCadaEstado(){
	sem_wait(&sem_programas);
	dictionary_iterator(diccionarioDeProgramas, tomarMetricasPorProceso);
	sem_post(&sem_programas);
}

void tomarMetricasPorProceso(char *key, void *unPrograma){
	log_info(logger,"Programa %s: %i en new, %i en ready, %i en blocked, %i en exec", key,
			((programa*) unPrograma)->cantidadDeHilosEnNew,
			((programa*) unPrograma)->cantidadDeHilosEnReady,
			((programa*) unPrograma)->cantidadDeHilosEnBlocked,
			((programa*) unPrograma)->exec!=NULL);

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
	/*sem_wait(&semaforoConfiguracion);
	//config = config_create(pathConfiguracion);
	//configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	sem_post(&semaforoConfiguracion);*/
	log_info(logger,"Grado actual de multiprogramacion: %i", configuracion.GRADO_DE_MULTIPROGRAMACION);
}

//Esto es para limitar la cantidad de post que se hace sobre el semaforo
/*void postSemaforoMultiprogramacion(){
	int valorSemaforo;
	sem_getvalue(&MAXIMOPROCESAMIENTO, &valorSemaforo);
	sem_wait(&semaforoConfiguracion);
	config = config_create(pathConfiguracion);
	configuracion.GRADO_DE_MULTIPROGRAMACION = config_get_int_value(config, "MAX_MULTIPROG");
	sem_post(&semaforoConfiguracion);
	if(valorSemaforo < configuracion.GRADO_DE_MULTIPROGRAMACION){
		sem_post(&MAXIMOPROCESAMIENTO);
	}

}*/

void planificarReady(){
	while(1){
		sem_wait(&hayNuevos);
		sem_wait(&sem_new);
		hilo *unHilo = queue_pop(new);
		sem_post(&sem_new);
		sem_wait(&MAXIMOPROCESAMIENTO);

		char *unPid = string_itoa(unHilo->pid);

		sem_wait(&sem_programas);
		((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnNew--;
		sem_post(&sem_programas);

		free(unPid);
		pasarAReady(unHilo);

	}
}


unsigned long long getMicrotime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return ((unsigned long long)( (tv.tv_sec)*1000 + (tv.tv_usec)/1000 ));
}

void crearHilo(int sd){

	//Tomo el sd como pid
	int pid = sd;

	int *tid = malloc(sizeof(int));
	read(sd, tid, sizeof(int));


	//printf("Llamaste a create con tid: %i\n", *tid);

	hilo *unHilo = malloc(sizeof(hilo));
	unHilo->pid = pid;
	unHilo->tid = *tid;
	unHilo->estimacion = 0;
	unHilo->rafaga = 0;

	unHilo->timestampCreacion = getMicrotime();
	unHilo->sumaDeIntervalosEnReady = 0;
	unHilo->sumaDeIntervalosEnExec = 0;
	unHilo->tiempoDeEjecucion = 0;

	char *unPid = string_itoa(unHilo->pid);
	loguearTransicion(unHilo->pid, unHilo->tid, "New");

	sem_wait(&sem_new);
	queue_push(new, unHilo);
	sem_post(&sem_new);
	sem_post(&hayNuevos);
	free(tid);

	sem_wait(&sem_programas);
	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnNew++;
	sem_post(&sem_programas);
	free(unPid);
}

void pasarAReady(hilo *unHilo){
	char *unPid = string_itoa(unHilo->pid);

	sem_wait(&sem_diccionario_ready);
	if(!dictionary_has_key(diccionarioDeListasDeReady, unPid)){
		t_list *listaDeReady = list_create();
		dictionary_put(diccionarioDeListasDeReady, unPid, listaDeReady);

		sem_wait(&sem_programas);
		//Si es la primera vez que llega pongo en exec (es el hilo main)
		((programa *)dictionary_get(diccionarioDeProgramas, unPid))->exec = unHilo;
		sem_post(&sem_programas);

		loguearTransicion(unHilo->pid, unHilo->tid, "Ready");
		loguearTransicion(unHilo->pid, unHilo->tid, "Execute");
	}
	else{

		//Lo agrego a la lista de ready correspondiente
		list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), unHilo);
		unHilo->timestampReady = getMicrotime();

		sem_wait(&sem_programas);

		((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnReady++;
		sem_post(&sem_programas);

		loguearTransicion(unHilo->pid, unHilo->tid, "Ready");

	}
	sem_post(&sem_diccionario_ready);

	free(unPid);
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

	char *unPid = string_itoa(pid);

	hilo *candidatoAEjecutar = ((hilo*)list_get((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), 0) );

	//El programa existe cuando algun hilo paso a ready en algun momento
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, unPid);

	hilo *elQueEstaEjecutando = ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec;

	if(existeElPrograma && elQueEstaEjecutando != NULL){

		//Si el que esta en ready tiene menos ejecucion ese sigue, sino el que estaba en exec
		if(candidatoAEjecutar!=NULL && candidatoAEjecutar->estimacion < ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->estimacion){
			//Paso a ready el que esta ejecutando y a exec el que estaba en ready (sacandolo de ready)
			candidatoAEjecutar = ((hilo*)list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), 0) );
			list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), elQueEstaEjecutando);
			((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec = candidatoAEjecutar;
			inicioRafaga = getMicrotime();

			elQueEstaEjecutando->sumaDeIntervalosEnExec += (int)(getMicrotime() - elQueEstaEjecutando->timestampExec);
			elQueEstaEjecutando->timestampReady = getMicrotime();
			candidatoAEjecutar->sumaDeIntervalosEnReady += (int)(getMicrotime() - candidatoAEjecutar->timestampReady);
			candidatoAEjecutar->timestampExec = getMicrotime();

			loguearTransicion(elQueEstaEjecutando->pid, elQueEstaEjecutando->tid, "Ready");
			loguearTransicion(candidatoAEjecutar->pid, candidatoAEjecutar->tid, "Execute");

			free(unPid);
			return candidatoAEjecutar->tid;
		}
		else{
			elQueEstaEjecutando->sumaDeIntervalosEnExec += (int)(getMicrotime() - elQueEstaEjecutando->timestampExec);
			elQueEstaEjecutando->timestampExec = getMicrotime();

			free(unPid);
			return elQueEstaEjecutando->tid;
		}
	}

	//Si no hay ninguno en exec paso el siguiente
	candidatoAEjecutar = ((hilo*)list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), 0) );

	//Si no hay ninguno listo me bloqueo hasta que haya alguno
	if(candidatoAEjecutar == NULL){
		sem_post(&sem_diccionario_ready);
		sem_post(&sem_programas);
		//printf("\nxd\n");
		programa* unPrograma = (programa*)dictionary_get(diccionarioDeProgramas, unPid);
		unPrograma->estaBloqueado = true;
		pthread_mutex_lock( &(unPrograma->mutexProgama) );

		sem_wait(&sem_diccionario_ready);
		sem_wait(&sem_programas);
		candidatoAEjecutar = ((hilo*)list_remove((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)), 0) );
		//printf("\n SALIO :O \n");
	}

	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec = candidatoAEjecutar;
	inicioRafaga = getMicrotime();
	//postSemaforoMultiprogramacion();

	((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnReady--;

	candidatoAEjecutar->sumaDeIntervalosEnReady += (int)(getMicrotime() - candidatoAEjecutar->timestampReady);
	candidatoAEjecutar->timestampExec = getMicrotime();

	loguearTransicion(candidatoAEjecutar->pid, candidatoAEjecutar->tid, "Execute");
	free(unPid);
	return candidatoAEjecutar->tid;
}

void replanificarHilos(int pid){
	char *unPid = string_itoa(pid);
	sem_wait(&sem_diccionario_ready);
	//Ordeno la lista por el que tiene menor estimacion
	list_sort((t_list*)(dictionary_get(diccionarioDeListasDeReady, unPid)) , (void*)tieneMenorEstimacion);
	sem_post(&sem_diccionario_ready);
	free(unPid);
}

void recalcularEstimacion(int pid){

	sem_wait(&sem_programas);
	char* unPid = string_itoa(pid);
	//El programa existe cuando algun hilo paso a ready en algun momento
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, unPid);

	//Si existe el programa y hay alguien en ejecucion
	if(existeElPrograma && ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec != NULL){
		int loQueTardo = getMicrotime() - inicioRafaga;
		//Actualizo la rafaga y la estimacion del que esta en ejecucion
		//printf("Rafaga del tid %i :%i\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid, loQueTardo);
		((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->rafaga = loQueTardo;
		actualizarEstimacion( ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec );
		//printf("Estimacion del tid %i :%f\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid, ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->estimacion);

	}
		//printf("TID: %i - Rafaga: %f\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid,((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->rafaga);
	sem_post(&sem_programas);

	free(unPid);

}


void actualizarEstimacion(hilo* unHilo){

	/*sem_wait(&semaforoConfiguracion);
	//config = config_create(pathConfiguracion);
	configuracion.ALPHA_SJF = config_get_double_value(config, "ALPHA_SJF");
	sem_post(&semaforoConfiguracion);*/

	//En+1 = (1-alpha)En + alpha*Rn
	unHilo->estimacion = (1-configuracion.ALPHA_SJF)*unHilo->estimacion +
			(configuracion.ALPHA_SJF * unHilo->rafaga);
}

bool tieneMenorEstimacion(hilo *unHilo, hilo *otroHilo){
	return unHilo->estimacion <= otroHilo->estimacion;
}

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
	char *unPid = string_itoa(pid);
	char *unTid = string_itoa(tid);
	char *mensaje = string_new();
	string_append(&mensaje, "EL hilo ");
	string_append(&mensaje, unTid);
	string_append(&mensaje, " del pid ");
	string_append(&mensaje, unPid);
	string_append(&mensaje, " pidio un signal sobre el semaforo ");
	string_append(&mensaje, semID);
	loguearInfo(mensaje);
	free(unPid);
	free(unTid);
	free(mensaje);
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
			char *unPid = string_itoa(pid);
			if( ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->tid == tid ){
				sem_wait(&blockedPorSemaforo);

				sem_wait(&sem_programas);
				//Mandar el hilo a blocked por semaforo
				queue_push((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID), ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec);

				loguearTransicion(((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->pid, ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->tid, "Blocked");

				((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnBlocked++;
				sem_post(&sem_programas);

				sem_post(&blockedPorSemaforo);
				//Lo saco de ejecucion
				((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec = NULL;
				//postSemaforoMultiprogramacion();
				//sem_post(&hayQueActualizarUnExec);
			}
			else{
				printf("El hilo en ejecucion no pidio el wait\n");
				free(unPid);
				exit(-1);
			}

			free(unPid);
		}

	}

	char *unPid = string_itoa(pid);
	char *unTid = string_itoa(tid);
	char *mensaje = string_new();
	string_append(&mensaje, "EL hilo ");
	string_append(&mensaje, unTid);
	string_append(&mensaje, " del pid ");
	string_append(&mensaje, unPid);
	string_append(&mensaje, " pidio un wait sobre el semaforo ");
	string_append(&mensaje, semID);
	loguearInfo(mensaje);
	free(unPid);
	free(unTid);
	free(mensaje);
	sem_post(&SEMAFOROS);
}

//Saco del diccionario en la cola de semID el primero y lo paso a la cola de ready correspondiente
void sacar1HiloDeLaColaDeBloqueadosPorSemaforo(int pid, char *semID){
	//Si esta vacia no hace nada
	if( !queue_is_empty((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID)) ){
		hilo *unHilo = queue_pop((t_queue*)dictionary_get(diccionarioDeBlockedPorSemaforo, semID));
		pasarAReady(unHilo);


		/*
		 * Si el hilo que paso a ready es de 1 programa que estaba bloqueado
		 * (pues se bloqueo al no tener uno en exec) -> desbloqueo el programa
		 */
		char *pidHiloDesbloqueado = string_itoa(unHilo->pid);
		programa *unPrograma = ((programa*) dictionary_get(
				diccionarioDeProgramas, pidHiloDesbloqueado));
		if (unPrograma->estaBloqueado) {
			pthread_mutex_unlock(&(unPrograma->mutexProgama));
			unPrograma->estaBloqueado = false;
		}
		free(pidHiloDesbloqueado);

		char *unPid = string_itoa(pid);
		sem_wait(&sem_programas);
		((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnBlocked--;
		sem_post(&sem_programas);
		free(unPid);
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
	char *unPid = string_itoa(pid);
	int existeElPrograma = dictionary_has_key(diccionarioDeProgramas, unPid);
	int hayAlguienEnExec = existeElPrograma && ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec != NULL;
	//printf("existeElPrograma %i - hayAlguienEnExec %i\n", existeElPrograma, hayAlguienEnExec);
	if(hayAlguienEnExec){
		//printf("tid en exec: %i - a esperar %i\n", ((programa*)dictionary_get(diccionarioDeProgramas, string_itoa(pid)))->exec->tid, tidAEsperar);

		//Si el hilo ya termino, ignoro el join
		if(!terminoElHilo(pid, tidAEsperar)){

			//Creo un hilo solo para comparar si ya estaba bloqueado y cambiar el contador
			hilo *hiloABloquear = malloc(sizeof(hilo));
			hiloABloquear->pid = pid;
			hiloABloquear->tid = ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->tid;
			//Si no estaba ya bloqueado por otro hilo, sumo el contador
			if(!bloqueadoPorAlgunHilo(hiloABloquear)){
				((programa*)dictionary_get(diccionarioDeProgramas, unPid))->cantidadDeHilosEnBlocked++;
			}

			free(hiloABloquear);

			//Pongo el que esta ejecutando en blockeado
			ponerEnBlockedPorHilo(pid, ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec, tidAEsperar);
			//Lo saco de exec
			((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec = NULL;
			//postSemaforoMultiprogramacion();
		}
	}
	sem_post(&sem_programas);
	free(unPid);
}

int terminoElHilo(int pid, int tid){
	int *tidAEsperar = malloc(sizeof(int));
	*tidAEsperar = tid;
	int terminoElHilo = dictionary_algunoCumple(diccionarioDeListasDeExit, (void *) estaEnExit, (void *) tidAEsperar);
	free(tidAEsperar);
	return terminoElHilo;
}

int estaEnExit(char* key, t_list* listaDeExit, void* tidAEsperar){
	bool estaElHilo(void* unHiloTerminado){
		return ((hilo*)unHiloTerminado)->tid == *((int*)tidAEsperar);
	}
	return list_any_satisfy(listaDeExit, estaElHilo);
}

void realizarClose(int pid, int tid){
	char *unPid = string_itoa(pid);
	if( ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec->tid == tid ){
		hilo *elQueEstaEjecutando = ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec;
		//Pongo el que esta ejecutando en exit
		pasarAExit(elQueEstaEjecutando);
		//Lo saco de exec
		((programa*)dictionary_get(diccionarioDeProgramas, unPid))->exec = NULL;
		//postSemaforoMultiprogramacion();
		//sem_post(&hayQueActualizarUnExec);

		//Libero la lista de los bloqueados por este hilo que termino
		liberarBloqueadosPorElHilo(elQueEstaEjecutando);
	}
	else{
		//XXX se fija en los de ready, bloqueados, etc?
	}
	free(unPid);
}

void pasarAExit(hilo *unHilo){
	sem_wait(&sem_exit);
	char *unPid = string_itoa(unHilo->pid);
	//Si ya existe una entrada para el proceso en la lista de exit, lo agrego a su lista
	if(!dictionary_has_key(diccionarioDeListasDeExit, unPid)){
			t_list *listaDeExit = list_create();
			list_add(listaDeExit, unHilo);
			dictionary_put(diccionarioDeListasDeExit, unPid, listaDeExit);
	}
	else{
		//Lo agrego a la lista correspondiente
		list_add((t_list*)(dictionary_get(diccionarioDeListasDeExit, unPid)), unHilo);
	}

	loguearTransicion(unHilo->pid, unHilo->tid, "Exit");

	free(unPid);
	sem_post(&sem_exit);

	sem_wait(&sem_metricas);
	tomarMetricas();
	sem_post(&sem_metricas);

	sem_post(&MAXIMOPROCESAMIENTO);
}

void ponerEnBlockedPorHilo(int pid, hilo* hiloAPonerEnBlocked, int tidAEsperar){
	char *unPid = string_itoa(pid);
	char *tidHiloAEsperar = string_itoa(tidAEsperar);
	t_dictionary *diccionarioDeBloqueados = ((programa*)dictionary_get(diccionarioDeProgramas, unPid))->blocked;
	if( !dictionary_has_key(diccionarioDeBloqueados, tidHiloAEsperar) ){
		t_list *listaDeBloqueados = list_create();
		dictionary_put(diccionarioDeBloqueados, tidHiloAEsperar, listaDeBloqueados);
	}
	//printf("hiloAPonerEnBlocked %i - tidAEsperar %i\n", hiloAPonerEnBlocked->tid, tidAEsperar),
	list_add((t_list*)dictionary_get(diccionarioDeBloqueados, tidHiloAEsperar), hiloAPonerEnBlocked);
	loguearTransicion(hiloAPonerEnBlocked->pid, hiloAPonerEnBlocked->tid, "Blocked");
	free(unPid);
	free(tidHiloAEsperar);
}

//fixme estos semaforos estan raros
void liberarBloqueadosPorElHilo(hilo *hiloBloqueante){
	sem_wait(&sem_programas);
	char *pidHiloBloqueante = string_itoa(hiloBloqueante->pid);
	char *tidHiloBloqueante = string_itoa(hiloBloqueante->tid);
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, pidHiloBloqueante))->blocked;
	t_list *listaDeBloqueados = (t_list *)dictionary_get(diccionarioDeBloqueadosPorProceso, tidHiloBloqueante);

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
				char *pidHiloBloqueado = string_itoa(hiloBloqueado->pid);

				sem_wait(&sem_diccionario_ready);
				//Lo agrego a ready
				list_add((t_list*)(dictionary_get(diccionarioDeListasDeReady, pidHiloBloqueado)), hiloBloqueado);
				//printf(" liberando tid bloqueado %i\n", hiloBloqueado->tid);
				loguearTransicion(hiloBloqueado->pid, hiloBloqueado->tid, "Ready");

				hiloBloqueado->timestampReady = getMicrotime();

				sem_post(&sem_diccionario_ready);

				//Disminuyo la cantidad de bloqueados
				((programa*)dictionary_get(diccionarioDeProgramas, pidHiloBloqueado))->cantidadDeHilosEnBlocked--;

				//Aumento la cantidad en ready
				((programa*)dictionary_get(diccionarioDeProgramas, pidHiloBloqueado))->cantidadDeHilosEnReady++;
				free(pidHiloBloqueado);
			}

			//sem_wait(&sem_programas);
		}

	}
	free(pidHiloBloqueante);
	free(tidHiloBloqueante);
	sem_post(&sem_programas);
}

hilo *tomarHiloBloqueadoPor(hilo *hiloBloqueante){
	char *pidHiloBloqueante = string_itoa(hiloBloqueante->pid);
	char *tidHiloBloqueante = string_itoa(hiloBloqueante->tid);
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, pidHiloBloqueante))->blocked;
	t_list *listaDeBloqueados = (t_list *)dictionary_get(diccionarioDeBloqueadosPorProceso, tidHiloBloqueante);
	free(pidHiloBloqueante);
	free(tidHiloBloqueante);
	return (hilo*)list_remove(listaDeBloqueados, 0);
}

int bloqueadoPorAlgunHilo(hilo *hiloBloqueado){
	char *pidHiloBloqueado = string_itoa(hiloBloqueado->pid);
	t_dictionary *diccionarioDeBloqueadosPorProceso = ((programa*)dictionary_get(diccionarioDeProgramas, pidHiloBloqueado))->blocked;
	int algunoLoBloquea = dictionary_algunoCumple(diccionarioDeBloqueadosPorProceso, (void *) tieneBloqueadoAlHilo, (void *) hiloBloqueado);
	free(pidHiloBloqueado);
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
	return list_any_satisfy(listaDeBloqueados, estaElHilo);
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

	void liberarHilo(hilo* unHilo){
		free(unHilo);
	}

	void borrarLista(t_list *lista){
		list_destroy_and_destroy_elements(lista, (void*)liberarHilo);
	}

	void borrarPrograma(programa* unPrograma){
		dictionary_destroy_and_destroy_elements(unPrograma->blocked, (void*)borrarLista);
		if(unPrograma->exec!=NULL){
			free(unPrograma->exec);
		}
		pthread_mutex_destroy(&(unPrograma->mutexProgama));
		free(unPrograma);
	}

	char *unPid = string_itoa(pid);
	sem_wait(&sem_exit);
	dictionary_remove_and_destroy(diccionarioDeListasDeExit, unPid, (void*)borrarLista);
	sem_post(&sem_exit);
	sem_wait(&sem_diccionario_ready);
	dictionary_remove_and_destroy(diccionarioDeListasDeReady, unPid, (void*)borrarLista);
	sem_post(&sem_diccionario_ready);
	sem_wait(&sem_programas);
	dictionary_remove_and_destroy(diccionarioDeProgramas, unPid, (void*)borrarPrograma);
	sem_post(&sem_programas);
	free(unPid);
}

void crearPrograma(int pid){
	t_dictionary *diccionarioDeListasDeBlockedPorHilo = dictionary_create();

	programa *unPrograma = malloc(sizeof(programa));

	unPrograma->pid = pid;
	unPrograma->blocked = diccionarioDeListasDeBlockedPorHilo;
	unPrograma->cantidadDeHilosEnNew = 0;
	unPrograma->cantidadDeHilosEnReady = 0;
	unPrograma->cantidadDeHilosEnBlocked = 0;
	unPrograma->tiempoDeEjecucionDeTodosLosHilos = 0;
	unPrograma->estaBloqueado = false;
	pthread_mutex_init(&(unPrograma->mutexProgama), NULL);


	unPrograma->exec = NULL;

	char *unPid = string_itoa(pid);
	sem_wait(&sem_programas);
	dictionary_put(diccionarioDeProgramas, unPid, unPrograma);
	sem_post(&sem_programas);

	//Si llega a estar en null en algun momento se vuelve hacer lock y se bloquea
	pthread_mutex_lock(&(unPrograma->mutexProgama));
	free(unPid);
}

void atenderScheduleNext(int sd){
	int tid = siguienteAEjecutar(sd);
	char* buffer = malloc(sizeof(int));
	memcpy(buffer, &tid, sizeof(int));
	send(sd, buffer, sizeof(int), 0);
	//sincronizarJoinYSchedule(sd);
	free(buffer);
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
	//address.sin_addr.s_addr = INADDR_ANY;
	//address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_addr.s_addr = inet_addr(configuracion.IP_SUSE);
	address.sin_port = htons(configuracion.PUERTO);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *) &address, sizeof(address))
			< 0) {
		perror("Bind fallo en el FS");
		return 1;
	}
	printf("Escuchando en el puerto: %d \n",
			configuracion.PUERTO);

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

					char *mensajeALoguear = string_new();
					string_append(&mensajeALoguear, "Se levanto un nuevo programa, pid: ");
					char *pid = string_itoa(client_socket[i]);
					string_append(&mensajeALoguear, pid);
					//printf("Agregado a la lista de sockets como: %d\n", i);
					loguearInfo(mensajeALoguear);
					free(pid);
					free(mensajeALoguear);

					crearPrograma(client_socket[i]);

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

					/*printf("Host disconected, ip: %s, port: %d\n",
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port));*/
					char *mensajeALoguear = string_new();
					string_append(&mensajeALoguear, "Se desconecto un programa, pid: ");
					char *pid = string_itoa(client_socket[i]);
					string_append(&mensajeALoguear, pid);
					loguearInfo(mensajeALoguear);
					free(pid);
					free(mensajeALoguear);

					close(sd);
					client_socket[i] = 0;

					sem_post(&MAXIMOPROCESAMIENTO);
					limpiarEstructuras(sd);
					/*int valorSemaforo;
					sem_getvalue(&MAXIMOPROCESAMIENTO, &valorSemaforo);
					printf("semaforo %i\n", valorSemaforo);*/
				} else {

					switch (*operacion) {
						case 1: //suse_create
							//Tomo el sd como pid
							crearHilo(sd);
							break;
						case 2: //suse_schedule_next
							;//atenderScheduleNext(sd);
							pthread_t hiloScheduleNext;
							sem_wait(&semScheduleNext);
							pthread_create(&hiloScheduleNext, NULL, (void*)atenderScheduleNext, (void*)sd);
							pthread_detach(hiloScheduleNext);
							sem_post(&semScheduleNext);
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

				}

				free(operacion);
			}
		}
	}
}
