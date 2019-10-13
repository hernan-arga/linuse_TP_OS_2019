Creadas las funciones de init y create.
Recibe un chuck de data de 12bytes: (operacion + pid + tid)
Desparcea y valida la data.
suse_create levanta un thread en la cola de ready del pid indicado, si se supera la multiprog va a la cola de new
