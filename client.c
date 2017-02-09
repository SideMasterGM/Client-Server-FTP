#include "service.h"
#include "client.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Variable global.
char g_pwd[PATH_MAX+1]; /*Mantiene la ruta del directorio de trabajo actual*/

int main(int a_argc, char **ap_argv){
	/*Socket descriptor*/
	int socket;
		
	/*Se verifican los argumentos*/
	if (a_argc < 3){
		printf("Problema: %s Nombre del servidor y puerto\n", ap_argv[0]);
		return 1;
	}
		
	/*Iniciar la variable*/
	realpath(".", g_pwd);
		
	/*Estableciendo el enlace*/
	if(!service_create(&socket, ap_argv[1], strtol(ap_argv[2], (char**)NULL, 10))){
		fprintf(stderr, "%s: Conexion para %s:%s ha fallado.\n", ap_argv[0], ap_argv[1], ap_argv[2]);
		return 2;
	}
	
	/*Establecer una sesion*/
	if(!session_create(socket)){
		close(socket);
		
		fprintf(stderr, "%s: Sesion Fallida.\n", ap_argv[0]);
		return 3;
	}
		
	printf("\nSesion establecida con exito!.");
	
	/*Manejar comandos de usuario*/
	service_loop(socket);
	
	/*Cerrar o destruir la sesion.*/
	if(session_destroy(socket))
		printf("Sesion finalizada con exito!.\n");
		
	/*Finalizar el enlace*/
	close(socket);
		
	return 0;
}

Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort){
	/*Struct vars*/
	struct sockaddr_in serverAddr;
	struct hostent *p_serverInfo;
		
	/*Crear direccion del servidor*/
		
	memset(&serverAddr, 0, sizeof(serverAddr));
	
	/*Asignacion de ip local o dns localhost*/
	if((p_serverInfo = gethostbyname(a_serverName)) == NULL){
		herror("Problema con la funcion -> service_create()");
		return false;
	}
	
	#ifndef NODEBUG
		printf("Problema: service_create(): Nombre servidor='%s', Direccion servidor='%s'\n", a_serverName, inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
	#endif
	
	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(a_serverPort);
		
	/*Creacion de socket*/
	if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0){
		perror("service_create(): Socket creado");

		return false;
	}
	
	/*Aplicando la conexion con el socket*/
	if((connect(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0){
		perror("service_create(): connect socket");
		close(*ap_socket);

		return false;
	}
	
	return true;
}

Boolean session_create(const int a_socket){
	/*Mensajes de notificacion*/
	Message msgOut, msgIn;
		
	/*Iniciar variables*/
	Message_clear(&msgOut);
	Message_clear(&msgIn);
		
	/*Dialogo de desafio de sesion.*/
	
	/*client: greeting
	  server: identify*/

	Message_setType(&msgOut, SIFTP_VERBS_SESSION_BEGIN);
		
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_IDENTIFY)){
		fprintf(stderr, "session_create(): Solicitud de conexion rechazada!.\n");
		return false;
	}
		
	// cilent: username
	// server: accept|deny
	Message_setType(&msgOut, SIFTP_VERBS_USERNAME);
	
	/*Insertar nombre de usuario*/
	printf("\nNombre de usuario: ");
	
	/*No generado en este proyecto*/
	scanf("%s", msgOut.m_param);
	
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED)){
		fprintf(stderr, "session_create(): Nombre de usuario rechazado!..\n");
		return false;
	}
	
	// client: password
	// server: accept|deny
	
	Message_setType(&msgOut, SIFTP_VERBS_PASSWORD);
	
	/*Insertar clave de seguridad*/
	printf("\npassword: ");
	scanf("%s", msgOut.m_param);

	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED)){
		fprintf(stderr, "session_create(): password rejected.\n");
		return false;
	}

	/*Sesion establecida!.*/
	#ifndef NODEBUG
		printf("session_create(): Exitosa\n");
	#endif
			
	return true;
}

void service_loop(const int a_socket) {
	/*Variables*/
	char buf[SIFTP_MESSAGE_SIZE+1];
	Boolean status, isLooping = true;
	String bufCR;
	
	String *p_argv;
	int argc;
		
	while(isLooping){
		/*Leer la instruccion o comandos*/
		printf("\nFTP|> ");
		memset(&buf, 0, sizeof(buf)); // clear buffer
		fgets(buf, sizeof(buf), stdin);
		
		/*Remover nueva linea*/
		if((bufCR = strrchr(buf, '\n')) != NULL)
			*bufCR = '\0';
		
		#ifndef NODEBUG
			printf("service_loop(): Comando conseguido: '%s'\n", buf);
		#endif
		
		/*Manejar comandos*/
		status = true;
	
		if(strcmp(buf, "exit") == 0){
			isLooping = false;
		}

		else if(strcmp(buf, "help") == 0) {
			printf("\nls\n  Muestra el contenido del directorio de trabajo actual remoto.\n");
			printf("\nlls\n  Muestra el contenido del directorio de trabajo actual local.\n");
			printf("\npwd\n  Muestra la ruta del directorio de trabajo actual remoto.\n");
			printf("\nlpwd\n  Muestra la ruta del directorio de trabajo actual local.\n");
			printf("\ncd <path>\n  Cambia el directorio de trabajo actual remoto a la dirección <path>.\n");
			printf("\nlcd <path>\n  Cambia el directorio de trabajo actual local al <path>.\n");
			printf("\nget <src> [dest]\n  Descarga el <src> remoto al directorio de trabajo actual local con el nombre de [dest] si se especifica.\n");
			printf("\nput <src> [dest]\n  Carga el archivo local <src> en el directorio de trabajo actual remoto con el nombre de [dest] si se especifica.\n");
			printf("\nhelp\n  Muestra estas instrucciones.\n");
			printf("\nexit\n  Finaliza el programa.\n");
		} else if(strlen(buf) > 0) {
			/*Analizar argumentos de comando*/
			if((p_argv = service_parseArgs(buf, &argc)) == NULL || argc <= 0)
				status = false;
			else
				status = service_handleCmd(a_socket, p_argv, argc);
			
			/*Limpiar*/
			service_freeArgs(p_argv, argc);
			p_argv = NULL;
		}
		else
			continue;
		
		/*Mostrar estado de comandos.*/
		printf("\n(Estado) %s.", (status ? "Exito" : "Fallo"));
	}
}

Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc){
	/*Variable de mensaje de salida*/
	Message msgOut;
		
	String dataBuf;
	int dataBufLen;
	
	Boolean tempStatus = false;
	
	/*Iniciar variable*/
	Message_clear(&msgOut);

	/*Manejar comandos*/
	if(strcmp(ap_argv[0], "lls") == 0){
		if((dataBuf = service_readDir(g_pwd, &dataBufLen)) != NULL){
			printf("%s", dataBuf);
			free(dataBuf);
			
			return true;
		}
	}
	
	else if(strcmp(ap_argv[0], "lpwd") == 0){
		printf("%s", g_pwd);
		return true;
	} else if(strcmp(ap_argv[0], "lcd") == 0 && a_argc > 1){
		return service_handleCmd_chdir(g_pwd, ap_argv[1]);
	} else if(strcmp(ap_argv[0], "ls") == 0 || strcmp(ap_argv[0], "pwd") == 0){
		/*Construir comando*/
		Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
		Message_setValue(&msgOut, ap_argv[0]);
		
		/*Ejecutar comando*/
		if(remote_exec(a_socket, &msgOut)){
			if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL){
				printf("%s", dataBuf);
				free(dataBuf);
				
				return true;
			}
		}
	}
	
	else if(strcmp(ap_argv[0], "cd") == 0 && a_argc > 1){
		/*Constuir comando*/
		Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
		Message_setValue(&msgOut, ap_argv[0]);
		strcat(Message_getValue(&msgOut), " ");
		strcat(Message_getValue(&msgOut), ap_argv[1]);
		
		/*Ejecutar comando*/
		return remote_exec(a_socket, &msgOut);
	}
	
	else if(strcmp(ap_argv[0], "get") == 0 && a_argc > 1){
		char dstPath[PATH_MAX+1];
		String src, dst;
		
		/*Iniciar variables*/
		src = ap_argv[1];
		dst = (a_argc > 2) ? ap_argv[2] : src;
		
		/*Comando de construcción con param = 'get remote-path'*/
		Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
		Message_setValue(&msgOut, ap_argv[0]);
		strcat(Message_getValue(&msgOut), " ");
		strcat(Message_getValue(&msgOut), src);
			
		/*Determinar la ruta del archivo de destino*/
			if(service_getAbsolutePath(g_pwd, dst, dstPath)){
				/*Checkea los permisos de escritura del fichero*/
				if(service_permTest(dstPath, SERVICE_PERMS_WRITE_TEST) && service_statTest(dstPath, S_IFMT, S_IFREG)){
					/*Ejecutar comando*/
					if(remote_exec(a_socket, &msgOut)){
						/*Recibir el archivo de destino*/
						if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL){
							/*Escribir archivo*/
							if((tempStatus = service_writeFile(dstPath, dataBuf, dataBufLen))){
								printf("%d Bytes transferidos.", dataBufLen);
							}
							
							free(dataBuf);
							
							#ifndef NODEBUG
								printf("get(): Escribiendo archivo %s.\n", tempStatus ? "OK" : "FALLIDO");
							#endif
						}
						#ifndef NODEBUG
						else
							printf("get(): Error al obtener el archivo remoto.\n");
						#endif
					}
					#ifndef NODEBUG
					else
						printf("get(): El servidor devuelve NACK.\n");
					#endif
				}
				#ifndef NODEBUG
				else
					printf("get(): No tiene permisos de escritura.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("get(): La ruta absoluta que determina ha fallado.\n");
			#endif
			
		return tempStatus;
	}
	
	else if(strcmp(ap_argv[0], "put") == 0 && a_argc > 1){
		char srcPath[PATH_MAX+1];
		String src, dst;
		
		/*Iniciar variables*/
		src = ap_argv[1];
		dst = (a_argc > 2) ? ap_argv[2] : src;
			
		/*Construir comando con param='put remote-path'*/
		Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
		Message_setValue(&msgOut, ap_argv[0]);
		strcat(Message_getValue(&msgOut), " ");
		strcat(Message_getValue(&msgOut), dst);
			
		/*Determinar la ruta de origen*/
		if(service_getAbsolutePath(g_pwd, src, srcPath)){
			/*Comprobando permisos de lectura*/
			if(service_permTest(srcPath, SERVICE_PERMS_READ_TEST) && service_statTest(srcPath, S_IFMT, S_IFREG)){
				// try to read source file
				if((dataBuf = service_readFile(srcPath, &dataBufLen)) != NULL){
					/* Cliente: Estoy enviando un fichero*/
					if(remote_exec(a_socket, &msgOut)){
						/*Servidor: Okay, se enviare un fichero*/
						
						/*Cliente: Aqui esta el fichero*/
						if((tempStatus = (siftp_sendData(a_socket, dataBuf, dataBufLen) && service_recvStatus(a_socket)))){
							/*Servidor: Exito*/
						
							printf("%d Bytes transferidos.", dataBufLen);
						}
						
						#ifndef NODEBUG
							printf("put(): Archivo enviado %s.\n", tempStatus ? "OK" : "FALLIDO");
						#endif
					}
					#ifndef NODEBUG
					else
						printf("put(): Servidor dice un negativo ACK.\n");
					#endif
					
					free(dataBuf);
				}
				#ifndef NODEBUG
				else
					printf("put(): La lectura del fichero ha fallado.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("put(): No tienes permisos de lectura.\n");
			#endif
		}
		#ifndef NODEBUG
		else
			printf("put(): La ruta determinada es fallida.\n");
		#endif
			
		return tempStatus;
	}
	
	return false;
}