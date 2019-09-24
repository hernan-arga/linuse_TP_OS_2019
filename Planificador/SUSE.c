/*
 * 1 - correr algoritmo fifo
 * 2 - correr algoritmo sjf
 * 3 - Respetar grado de mutiprog
 * 4 - mostrar resultados por pantalla
 * 5 - implementar cola extra de multiprog
 * */

#include "suse.h"

int main(void){

	// Levanta archivo de configuracion
	t_log *log = crear_log();
	config* pconfig = malloc(5 * sizeof(int)); //sumarle al malloc los arrays
	levantarConfigFile(pconfig);

	t_queue * th_readyq=queue_create();

	int suse_fd = iniciar_servidor(pconfig->ip,pconfig->puerto);
	log_info(logger, "Suse listo para recibir a cliente!");

	int cliente_fd = esperar_cliente(suse_fd);
	log_info(logger, "!");

	char mensaje = recibir_mensaje(cliente_fd);
	queue_push(th_readyq, &mensaje);

	int contador;

	if (strlen(mensaje) == 0)
	{
		contador=0;
	}
	else
	{
		contador=1;
	}

	while (strlen(mensaje) == 0) //condicion de suse para seguir corriendo
		{

			if(contador > pconfig->max_multiprog ) //chequeo nivel de multiprog
			{
				printf("\nMultiprog overhead exit\n");
				break;

			}else
			{
				queue_push(th_readyq, &mensaje); //agrego hilo a la cola de ready
				printf("\nAgregue valor %s a la cola\n",mensaje);
			}

			printf("ready queue:%d\t", queue_size(th_readyq)); //loggear metricas

			contador+=1;
			mensaje = recibir_mensaje(cliente_fd);

		}


	printf("mensajes recibidos: %i=%i\n", queue_size(th_readyq),contador);
	int i = queue_size(th_readyq);

	printf("valores");
	while (list_is_empty(th_readyq)!=0){
		printf(" ->%i", queue_peek(th_readyq));
		queue_pop(th_readyq);
	}
	printf("fin");

	queue_destroy_and_destroy_elements(th_readyq,0);
	return 0;
}
/*	// Levanta conexion por socket
	pthread_create(&hiloLevantarConexion, NULL, (void*) iniciar_conexion(pconfig->ip, pconfig->puerto), NULL);
	log_info(log, "SUSE levantado correctamente\n");

	pthread_join(hiloLevantarConexion, NULL);


	int m=1;

	t_thread * th_new; //nuevo proceso que recibo de hilolay
	t_thread * th_live; //contador de multiprog

	th_new = malloc(sizeof(t_thread) * 1);
	th_live = malloc(sizeof(t_thread) * 1);

	t_queue * th_readyq=queue_create();
	t_queue * th_new_mgr=queue_create();

	*th_live=0;

	while (m != 0) //condicion de suse para seguir corriendo
	{
		printf("\nEnter new Thread: "); //recibo nuevo thread de hilolay
		scanf("%d", th_new);
		*th_live=*th_live+1;

		if( *th_live > pconfig->max_multiprog ) //chequeo nivel de multiprog
		{
			queue_push(th_new_mgr, th_new);
			printf("\nMultiprog overhead\n");
			*th_live=*th_live-1;

		}else
		{
			queue_push(th_readyq, th_new); //agrego hilo a la cola de ready
			printf("\nMultiprog ok\n");
		}

		printf("ready queue:%d\t", queue_size(th_readyq)); //loggear metricas
		printf("new queue:%d\t", queue_size(th_new_mgr));

		if(*th_new == 0) //salir
			m=0;
	}

	queue_destroy_and_destroy_elements(th_readyq,0);
	queue_destroy_and_destroy_elements(th_new_mgr,0);

void _suse_init () //inicializa los recursos de la biblioteca
{

}

/* se la llama para crear un nuevo hilo, donde LA FUNCION que se pase por
 * parametro actuará como main del misma, finalizando el hilo al terminar esa funcion
 */
/*
void _suse_create (void(*ptr) (void))
{
							// ejecutar la funcion y agregarla a la cola de new
	ptr(); 		        	// fin del proceso matar hilo


}

void _suse_schedule_next () //obtiene el proximo hilo a ejecutar
{

}

void _suse_wait () //genera una operacion wait sobre el semaforo dado
{

}

void _suse_signal () //genera una operacion signal sobre el semaforo dado
{

}

void _suse_join () //puntero del thread recibido por hilolay
{

}

void _suse_sjf (t_thread thread) //puntero del thread recibido por hilolay
{

	int arrival_time[10], burst_time[10], temp[10];
	int i, smallest, count = 0, time, limit;
	double wait_time = 0, turnaround_time = 0, end;
	float average_waiting_time, average_turnaround_time;
	printf("nEnter the Total Number of Processes:t");
	scanf("%d", &limit);
	printf("nEnter Details of %d Processesn", limit);
	for(i = 0; i < limit; i++)
	{
		printf("nEnter Arrival Time:t");
		scanf("%d", &arrival_time[i]);
		printf("Enter Burst Time:t");
		scanf("%d", &burst_time[i]);
		temp[i] = burst_time[i];
	}
	burst_time[9] = 9999;
	for(time = 0; count != limit; time++)
	{
		smallest = 9;
		for(i = 0; i < limit; i++)
		{
			  if(arrival_time[i] <= time && burst_time[i] < burst_time[smallest] && burst_time[i] > 0)
			  {
					smallest = i;
			  }
		}
		burst_time[smallest]--;
		if(burst_time[smallest] == 0)
		{
			  count++;
			  end = time + 1;
			  wait_time = wait_time + end - arrival_time[smallest] - temp[smallest];
			  turnaround_time = turnaround_time + end - arrival_time[smallest];
		}
	}
	average_waiting_time = wait_time / limit;
	average_turnaround_time = turnaround_time / limit;
	printf("nnAverage Waiting Time:t%lfn", average_waiting_time);
	printf("Average Turnaround Time:t%lfn", average_turnaround_time);
	return 0;
}





*/











