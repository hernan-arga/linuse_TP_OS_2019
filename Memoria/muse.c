#include <stdio.h>
#include "muse.h"
#include "utils.h"

int main(){
	// Levanta archivo de configuracion
	t_config* configuracion = leer_config();
	t_log *log = crear_log();
	char* ip = config_get_string_value(configuracion, "IP");
	int puerto = config_get_int_value(configuracion, "LISTEN_PORT");
}


