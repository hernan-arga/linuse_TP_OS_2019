#include "utils.h"

t_config* leer_config() {
	return config_create("suse_config");
}


t_log * crear_log() {
	return log_create("suse.log", "suse", 1, LOG_LEVEL_DEBUG);
}
