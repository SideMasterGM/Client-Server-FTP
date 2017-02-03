#ifndef CLIENT_H
#define CLIENT_H

#include "siftp.h"

	/* Servicios */
	
		/**
		 * Establece una conexión de red con el servidor especificado.
		 * @param	ap_socket	Almacenamiento para descriptor del socket.
		 * @param	a_serverName	Nombre canonical o direccion IP del host remoto.
		 * @param	a_serverPort	Numero de puerto en decimal del host remoto.
		 */
		Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort);

#endif
