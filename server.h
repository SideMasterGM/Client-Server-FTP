#ifndef SERVER_H
#define SERVER_H

#include "siftp.h"

	/* constante */
	
		#define SERVER_SOCKET_BACKLOG	5
		#define SERVER_PASSWORD	"Inform@tico"
	
	/* servicios*/
	
		/**
		 * Establece un servicio de red en el puerto especificado.
		 * @param	ap_socket	Almacenamiento para el descriptor de socket.
		 * @param	a_port	número de puerto en decimal.
		 */
		Boolean service_create(int *ap_socket, const int a_port);
		
#endif
