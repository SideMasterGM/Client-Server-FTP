#include "service.h"
#include "server.h"

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*Variable global*/
char g_pwd[PATH_MAX+1];

/*Funciones*/

void sigchld_handler(int s){
	while(wait(NULL) > 0);
}

int main(int a_argc, char **ap_argv){
	/*Variables*/
	int serverSocket, clientSocket, clientAddrSize, childPID;
	struct sockaddr_in clientAddr;
	struct sigaction signalAction;
		
	/*Validacion de la cantidad de argumentos*/
	if(a_argc < 3){
		printf("Problema: %s nombre del directorio y numero de puerto\n", ap_argv[0]);
		return 1;
	}
		
	/*Inicilizando las variables*/
	realpath(ap_argv[1], g_pwd);
		
	/*Creando el servicio*/
	if(!service_create(&serverSocket, strtol(ap_argv[2], (char**)NULL, 10))){
		fprintf(stderr, "%s: Problema al intentar crear el servicio en el puerto: %s\n", ap_argv[0], ap_argv[2]);
		
		return 2;
	}
	
	/*Manejador de terminacion de la instalacion*/
	signalAction.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_flags = SA_RESTART;
	
	if (sigaction(SIGCHLD, &signalAction, NULL) == -1){
		perror("main(): Error");
		
		return 3;
	}
		
	/*Loop*/
	while(true){
		clientAddrSize = sizeof(clientAddr);
		
		/*Esperando por un cliente*/
	
		#ifndef NODEBUG
			printf("\nmain(): Esperando por clientes...\n");
		#endif
	
		if((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize)) != -1){
			#ifndef NODEBUG
				printf("\nmain(): Consiguiendo conexion con el cliente: [IP=%s, puerto=%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
			#endif
			
			/*Codigo hijo*/
			if((childPID = fork()) == 0) {
				close(serverSocket); /*El hijo no necesita este socket*/
				
				/*Servicio del cliente*/
					if(session_create(clientSocket))
						service_loop(clientSocket);
					
					session_destroy(clientSocket);
				
				/*Finalizando con el cliente*/
					close(clientSocket);
					return 0;
			}
			
			#ifndef NODEBUG
				printf("\nmain(): Procesando el hijo; pid=%d\n", childPID);
			#endif
			
			close(clientSocket); /*El pariente no necesita este socket*/
		}
		else
			perror("main()");
	}
		
	/*Destruyendo la sesion*/
	close(serverSocket);

	return 0;
}

Boolean service_create(int *ap_socket, const int a_port) {
	/*Variables*/
	struct sockaddr_in serverAddr;
	
	#ifdef _PLATFORM_SOLARIS
		char yes='1';
	#else
		int yes=1;
	#endif
		
	/*Creando una direccion*/
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(a_port);
	
	/*Creando el Socket*/
	if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0){
		perror("service_create(): Crear socket");
		
		return false;
	}
	
	/*Establecer opciones*/
	if(setsockopt(*ap_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0){
		perror("service_create(): socket opts");

		return false;
	}

	/*Bind Socket*/
	if(bind(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
		perror("service_create(): bind socket");
		close(*ap_socket);
		
		return false;
	}
	
	/*Escuchando el socket*/
	if(listen(*ap_socket, SERVER_SOCKET_BACKLOG) < 0){
		perror("service_create(): listen socket");
		close(*ap_socket);

		return false;
	}
	
	return true;
}

Boolean session_create(const int a_socket){
	/*Mensajes de entrada y salida*/
	Message msgOut, msgIn;
	
	/*Inicializando variables*/
	Message_clear(&msgOut);
	Message_clear(&msgIn);
	
	/*Cambiando los dialogos de sesion*/

	// cliente: greeting
	if(!siftp_recv(a_socket, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_SESSION_BEGIN)){
		fprintf(stderr, "service_create(): sesion no respondida por el cliente.\n");
		return false;
	}
	
	// server: identify
	// client: username
	Message_setType(&msgOut, SIFTP_VERBS_IDENTIFY);
	
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_USERNAME)){
		fprintf(stderr, "service_create(): Nombre de usuario no especificado por el cliente.\n");
		return false;
	}

	// server: accept|deny
	// client: password
	Message_setType(&msgOut, SIFTP_VERBS_ACCEPTED); //XXX check username... No requerido para este proyecto	
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_PASSWORD)){
		fprintf(stderr, "service_create(): Clave personal no especificada por el cliente.\n");
		return false;
	}

	// server: accept|deny
	if(Message_hasValue(&msgIn, SERVER_PASSWORD)){
		Message_setType(&msgOut, SIFTP_VERBS_ACCEPTED);
		siftp_send(a_socket, &msgOut);
	} else{
		Message_setType(&msgOut, SIFTP_VERBS_DENIED);
		siftp_send(a_socket, &msgOut);
		
		fprintf(stderr, "service_create(): Cliente, clave incorrecta.\n");
		return false;
	}
		
	/*La sesion ha sido establecida*/
	#ifndef NODEBUG
		printf("session_create(): Satisfactorio\n");
	#endif
			
	return true;
}

void service_loop(const int a_socket) {
	/*Variables*/
	Message msg;
		
	String *p_argv;
	int argc;
	
	/*Inicializando variable*/
	Message_clear(&msg);
	
	/*Esperando...*/
	while(siftp_recv(a_socket, &msg)) {
		if(Message_hasType(&msg, SIFTP_VERBS_SESSION_END) || Message_hasType(&msg, "")){
			break;
		} else {
			#ifndef NODEBUG
				printf("service_loop(): Resolviendo el comando: '%s'\n", Message_getValue(&msg));
			#endif
			
			/*Parseo de la respuesta*/
			if((p_argv = service_parseArgs(Message_getValue(&msg), &argc)) == NULL || argc <= 0) {
				service_freeArgs(p_argv, argc);
				
				if(!service_sendStatus(a_socket, false)) // Enviado un NACK 
					break;
				
				continue;
			}
			
			/*Manejando la respuesta*/
			if(!service_handleCmd(a_socket, p_argv, argc))
				service_sendStatus(a_socket, false); // El envio del NACK ha fallado
			
			/*Limpiar*/
			service_freeArgs(p_argv, argc);
			p_argv = NULL;
			
			/*
			if(!service_handleCmd(a_socket, Message_getValue(&msg))) // send failure notification
			{
				Message_setType(&msg, SIFTP_VERBS_ABORT);
				
				if(!siftp_send(a_socket, &msg))
				{
					fprintf(stderr, "service_loop(): unable to send faliure notice.\n");
					break;
				}
			}
			*/
		}
	}
}


Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc) {
	/*Variables*/
	Message msg;
	
	String dataBuf;
	int dataBufLen;
	
	Boolean tempStatus = false;
	
	/*Inicializando variables*/
	Message_clear(&msg);
		
	/*Manejando comandos*/
	if(strcmp(ap_argv[0], "ls") == 0) {
		if((dataBuf = service_readDir(g_pwd, &dataBufLen)) != NULL) {
			/*Transmitir datos*/
			if(service_sendStatus(a_socket, true))
				tempStatus = siftp_sendData(a_socket, dataBuf, dataBufLen);
			
			#ifndef NODEBUG
				printf("ls(): Estado=%d\n", tempStatus);
			#endif
			
			/*Limpiar dataBuf*/
			free(dataBuf);
				
			return tempStatus;
		}
	}
	
	else if(strcmp(ap_argv[0], "pwd") == 0){
		if(service_sendStatus(a_socket, true))
			return siftp_sendData(a_socket, g_pwd, strlen(g_pwd));
	}
	
	else if(strcmp(ap_argv[0], "cd") == 0 && a_argc > 1){
		return service_sendStatus(a_socket, service_handleCmd_chdir(g_pwd, ap_argv[1]));
	}
	
	else if(strcmp(ap_argv[0], "get") == 0 && a_argc > 1){
		char srcPath[PATH_MAX+1];
		
		/*Ruta absoluta determinada*/
		if(service_getAbsolutePath(g_pwd, ap_argv[1], srcPath)){
			/*Comprobar los permisos de lectura en el fichero*/
			if(service_permTest(srcPath, SERVICE_PERMS_READ_TEST) && service_statTest(srcPath, S_IFMT, S_IFREG)){
				/*Leyendo el fichero*/
				if((dataBuf = service_readFile(srcPath, &dataBufLen)) != NULL){
					if(service_sendStatus(a_socket, true)){
						/*Envio del fichero*/
						tempStatus = siftp_sendData(a_socket, dataBuf, dataBufLen);
						
						#ifndef NODEBUG
							printf("get(): fichero enviado: %s.\n", tempStatus ? "OK" : "FALLIDO");
						#endif
					}
					#ifndef NODEBUG
					else
						printf("get(): El host remoto ha obtenido el ACK.\n");
					#endif
					
					free(dataBuf);
				}
				#ifndef NODEBUG
				else
					printf("get(): La lectura del fichero ha fallado.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("get(): No tienes permisos de lectura.\n");
			#endif
		}
		#ifndef NODEBUG
		else
			printf("get(): Fallo en la ruta absoluta determinada\n");
		#endif
			
		return tempStatus;
	}

	
	else if(strcmp(ap_argv[0], "put") == 0 && a_argc > 1){
		char dstPath[PATH_MAX+1];
		
		/*Determinar el destino de la ruta del fichero*/
		if(service_getAbsolutePath(g_pwd, ap_argv[1], dstPath)){
			/*Comprobar permisos de escritura*/
			if(service_permTest(dstPath, SERVICE_PERMS_WRITE_TEST) && service_statTest(dstPath, S_IFMT, S_IFREG)){
				/*Enviar ACK primario: Permisos del fichero / OK*/
				if(service_sendStatus(a_socket, true)){
					/* Archivo recibido*/
					if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL){
						#ifndef NODEBUG
							printf("put(): Acerca de la escritura del fichero: '%s'.\n", dstPath);
						#endif
						
						tempStatus = service_writeFile(dstPath, dataBuf, dataBufLen);
						
						free(dataBuf);
						
						#ifndef NODEBUG
							printf("put(): escribiendo archivo: %s.\n", tempStatus ? "OK" : "FALLIDO");
						#endif
						
						/*Enviar ACK secundario: el fichero fue escrito correctamente*/
						if(tempStatus){
							return service_sendStatus(a_socket, true);
						}
					}
					#ifndef NODEBUG
					else
						printf("put(): Fallo al intentar obtener el fichero remoto.\n");
					#endif
				}
				#ifndef NODEBUG
				else
					printf("put(): No se ha logrado obtener el estado ACK del host remoto.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("put(): No tiene permisos de escritura.\n");
			#endif
		}
		#ifndef NODEBUG
		else
			printf("put(): Fallo al determinar la ruta absoluta.\n");
		#endif
	}
	
	return false;
}