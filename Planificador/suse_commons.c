#include "suse_commons.h"
#include "suse.h"

//extern de las colas creadas en SUSE.c
extern t_list* suse_programlist;
extern t_list* suse_exit;
extern t_queue* suse_new;
extern t_list* suse_block;

/**************************///MISC
/**************************/

t_config* leer_config() {
	return config_create("suse_config");
}

t_log * crear_log() {
	return log_create("suse.log", "suse", 1, LOG_LEVEL_DEBUG);
}

void levantarConfigFile(config* pconfig){
	t_config* configuracion = leer_config();

	pconfig->puerto = config_get_int_value(configuracion, "LISTEN_PORT");
	pconfig->metrics_timer = config_get_int_value(configuracion, "METRICS_TIMER");
	pconfig->max_multiprog = config_get_int_value(configuracion, "MAX_MULTIPROG");
	pconfig->alpha = config_get_int_value(configuracion, "ALPHA_SJF");

}

void limpiarListasyColas(){
	list_destroy(suse_programlist);
	queue_destroy(suse_new);
	list_destroy(suse_exit);
	list_destroy(suse_block);
}

/**************************/
/**************************/

//agregar programa a la lista de programas SUSE_INIT llama a esta función solo se realiza una vez por programa.
void addProgramList(t_programa * newProgram){

	int repetido = 0;
	t_programa* element;

	for (int i = 0; i < list_size(suse_programlist); ++i) //recorro la lista hasta encontrar el pid
	{
		element = list_get(suse_programlist, i);
		if (element->pid == newProgram->pid){ //si lo encontre enta repetido

			printf("\nPID repetido, solo de debe hacer init una vez por programa\n");
			repetido = 1;
			break;
		}
	}

	if (repetido == 0) //si no esta repetido se agrega a la lista
	{
		printf("\nSe agrego el PID: %d, a la lista de programas\n",newProgram->pid);
		list_add(suse_programlist, newProgram);
	}

	return;
}

//DEBUG: mostrar la lista para programas planificador y la cantidad de threads en cola de ready y exec. ordenadas por pid.
void show_program_list(){
	printf("\nProgram List Begin\n");

	t_programa* a;

	for (int i = 0; i < list_size(suse_programlist); ++i)
	{
		a = list_get(suse_programlist, i);
		printf("\nPID: %d\t Threads: %d\t Exec:%d\t Ready:%d\n",a->pid,(queue_size(a->exec)+queue_size(a->ready)),queue_size(a->exec),queue_size(a->ready));
	}

	printf("\nProgram List End\n");
}

/**************************/

//recibo el buffer del cliente y desparceo el pid. creo la estructura de programa e inicializo los recursos
//se agrega a la lista de programas planificados
void suse_init(void* buf){

	int pid;
	memcpy (&pid, buf+4, 4);

	printf("\nRealizar INIT de PID:%d\n", pid);

	t_programa * nuevoPrograma;
	nuevoPrograma= malloc(sizeof(t_programa));

	t_queue* execQ = queue_create();
	t_queue* readyQ = queue_create();

	nuevoPrograma->pid = pid;
	nuevoPrograma->exec = execQ;
	nuevoPrograma->ready = readyQ;

	addProgramList(nuevoPrograma);

	printf("\ncantidad de elementos en el program list: %d\n",list_size(suse_programlist)); //debug
	show_program_list(); //debug
	return;
}

//indica si el thread tiene un pid planificado
bool programInit (int pid)
{
	t_programa* a;

	for (int i = 0; i < list_size(suse_programlist); ++i)
	{
		a = list_get(suse_programlist, i);

		if(a->pid==pid){
			return true;
		}
	}
	return false;
}


//retorna valor de multiprogramacion actual, contabilizando todas las colas de ready exec y block cross programas
int calcular_multiprog(){

	t_programa* a;
	int contador = 0;

	for (int i = 0; i < list_size(suse_programlist); ++i)
	{
		a = list_get(suse_programlist, i);

		contador = contador + queue_size(a->exec) + queue_size(a->ready);
	}

	contador = contador + list_size(suse_block);

	return contador;
}

//busca un programa en la lista de programas planificados y devuelve un puntero a la estructura
t_programa* buscar_programa(int pid)
{
	t_programa* a;

	for (int i = 0; i < list_size(suse_programlist); ++i)
	{
		a = list_get(suse_programlist, i);

		if (a->pid == pid){

			return a;
		}
	}
	return NULL;
}


//toma el nuevo hilo y lo agrega a la cola de listo de programa
//o lo agrega a la cola de NEW en caso de que se supere la multiprog
void suse_new_event(t_hilo* hilo){

	t_programa * a;

	config* config = malloc(5 * sizeof(int));	//levantar del config el grado maximo de multiprogramacion
	levantarConfigFile(config);

	int multiprog = calcular_multiprog();

	printf("\nmultiprog max = %d / multiprog actual: %d\n",config->max_multiprog, multiprog );

	if (config->max_multiprog > multiprog)
	{
		a = buscar_programa(hilo->pid);

		if (a == NULL){
			printf("\nerror al encontrar el programa PID: %d\n",hilo->pid);
			return;
		}else{
			printf("\nHilo TID: %d fue agregado en la cola de Ready del programa PID: %d\n",hilo->tid,hilo->pid);
			queue_push(a->ready,hilo);
			return;
		}
	}else{
		printf("\nGrado de Multiprogramación superado. Se agrego a la cola de new.\n");
		queue_push(suse_new,hilo);
		return;
	}

}

//recibo el buffer del cliente y desparceo la info
//TID : thread id - PID : program id
//los guardo en la estructura y los envio agrego al planificador
void suse_create(void*buf){

	int pid;
	memcpy (&pid, buf+4, 4);
	int tid;
	memcpy (&tid, buf+8, 4);

	printf("\nRealizar CREATE por PID:%d\n",pid);

	if (programInit(pid))
	{
		printf("\nPID %d INIT OK\n",pid); //agrego a la lista de new

		t_hilo* hilo = malloc(sizeof(t_hilo));
		hilo -> pid = pid;
		hilo -> tid = tid;

		suse_new_event(hilo);

		show_program_list();
		return;

	}else{ //como no esta initeado el programa N se llama a suse_init y se recurseo el create
		printf("\nPID %d INIT ERR\n",pid);
		printf("\nDOING INIT PID %d\n",pid);
		suse_init(buf);

		return suse_create(buf);
	}
}

/**************************/

