#include <stdio.h>
#include "suse.h"
#include "utils.h"

int main(){

	t_config* configuracion = leer_config();
	t_log *log = crear_log();

	char* ip = config_get_string_value(configuracion, "IP");
	char* port = config_get_string_value(configuracion, "LISTEN_PORT");

	int socket_servidor = iniciar_servidor(ip,port);
	log_info(log, "SUSE levantado correctamente\n");
	int socket_cliente = esperar_cliente(socket_servidor);

    return 0;
}


