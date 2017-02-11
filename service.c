#include "service.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

Boolean session_destroy(const int a_socket){
	#ifndef NODEBUG
		printf("session_destroy(): Cerrando sesion.");
	#endif
	
	// variables
		Message msgOut;
		
	// Inicializar variable
		Message_clear(&msgOut);
		
	// Enviar noticia
		Message_setType(&msgOut, SIFTP_VERBS_SESSION_END);
		return siftp_send(a_socket, &msgOut);
}

Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response){
	return siftp_send(a_socket, ap_query) && siftp_recv(a_socket, ap_response);
}

Boolean service_getAbsolutePath(const String a_basePath, const String a_extension, String a_result){
	char tempPath[PATH_MAX+1];
	int tempLen;
	
	// Iniciar
	memset(&tempPath, 0, sizeof(tempPath));
		
	/*Ensamblando la ruta absoluta desde los argumentos*/
	if(a_extension[0] == '/'){
		strcpy(tempPath, a_extension);
	} else {
		strcpy(tempPath, a_basePath);
		
		// Aperturando la extension
		if((tempLen = strlen(a_basePath)) + strlen(a_extension) < sizeof(tempPath)){
			tempPath[tempLen++] = '/'; // relative path
			strcpy(&tempPath[tempLen], a_extension);
		}
	}
	
	realpath(tempPath, a_result);
	
	#ifndef NODEBUG
		printf("service_getAbsolutePath(): Antes='%s', Despues='%s'\n", tempPath, a_result);
	#endif
	
	return true;
}

Boolean service_sendStatus(const int a_socket, const Boolean a_wasSuccess){
	Message msg;
	
	//Inicializando variables.
	Message_clear(&msg);
	Message_setType(&msg, SIFTP_VERBS_COMMAND_STATUS);
	Message_setValue(&msg, a_wasSuccess ? "true" : "false");
		
	return siftp_send(a_socket, &msg);
}

String* service_parseArgs(const String a_cmdStr, int *ap_argc){
	String buf, *p_args, arg;
	int i;
	
	/*Inicializando variable*/
	if((buf = calloc(strlen(a_cmdStr)+1, sizeof(char))) == NULL)
		return NULL;
		
	strcpy(buf, a_cmdStr);
	
	/*Parseo de palabras*/
	for(p_args = NULL, i=0, arg = strtok(buf, " \t"); arg != NULL; i++, arg = strtok(NULL, " \t")){
		// Expandir array
		if((p_args = realloc(p_args, (i+1) * sizeof(String))) == NULL){
			free(buf);
			return NULL;
		}
	
		// Expandir elemento
		if((p_args[i] = calloc(strlen(arg)+1, sizeof(char))) == NULL){
			service_freeArgs(p_args, i);
			free(buf);
			
			return NULL;
		}
			
		strcpy(p_args[i], arg);
	}
		
	*ap_argc = i;
	
	#ifndef NODEBUG
		for(i = 0; i<*ap_argc; i++)
			printf("parseArgs(): argv[%d]='%s', IP=%p\n", i, p_args[i], p_args[i]);
	#endif
	
	/*Limpiar variable*/
	free(buf);
		
	return p_args;
}

void service_freeArgs(String *ap_argv, const int a_argc){
	// check args
	if(ap_argv == NULL || a_argc <= 0)
		return;
	
	int i;
	
	/*Liberar los elementos del array*/
	for(i = 0; i<a_argc; i++){
		if(ap_argv[i] != NULL)
			free(ap_argv[i]);
	}
	
	free(ap_argv);
}

Boolean service_recvStatus(const int a_socket) {
	Message msg;
	
	/*Inicializando variable*/
	Message_clear(&msg);
	
	return (siftp_recv(a_socket, &msg) &&  Message_hasType(&msg, SIFTP_VERBS_COMMAND_STATUS) && Message_hasValue(&msg, "true"));
}

Boolean remote_exec(const int a_socket, Message *ap_query) {
	return (siftp_send(a_socket, ap_query) &&  service_recvStatus(a_socket));
}

Boolean service_handleCmd_chdir(String a_currPath, const String a_newPath) {
	/*Checkiando argumentos*/
	if(a_newPath == NULL)
		return false;

	/*Variables*/
	char path[PATH_MAX+1];
	
	/*Inicializando variable*/
	memset(&path, 0, sizeof(path));

	/*Validando la nueva ruta*/
	if(service_getAbsolutePath(a_currPath, a_newPath, path)){
		/*Permisos del tipo de inode*/
		if(service_permTest(path, SERVICE_PERMS_READ_TEST) && service_statTest(path, S_IFMT, S_IFDIR)){
			strcpy(a_currPath, path);
		} else {
			perror("cd()");
			return false;
		}
	}

	return true;
}

String service_readFile(const String a_path, int *ap_length){
	String buf = NULL;
	FILE *p_fileFd;

	/*Checkiando permisos*/
	if(service_statTest(a_path, S_IFMT, S_IFREG)){
		if((p_fileFd = fopen(a_path, "r")) != NULL){
			/*Determina la longitud del fichero*/
				fseek(p_fileFd, 0, SEEK_END);
				*ap_length = ftell(p_fileFd);
				rewind(p_fileFd);
				
			/*Allocate el buffer*/
				if((buf = calloc(*ap_length, sizeof(char))) == NULL){
					fclose(p_fileFd);
					
					fprintf(stderr, "readFile(): Lectura del fichero en el buffer ha fallado.\n");
					return false;
				}
				
				/*Leer contenido de el buffer*/
				if(fread(buf, sizeof(char), *ap_length, p_fileFd) != *ap_length){
					perror("readFile()");
					fclose(p_fileFd);
					
					free(buf);
					buf = NULL;
				}
				
			fclose(p_fileFd);
		} else {
			perror("readFile()");
		}
	}
	
	return buf;
}

String service_readDir(const String a_path, int *ap_length){
	String buf = NULL;
	int i, j;
	
	DIR *p_dirFd;
	struct dirent *p_dirInfo;

	if((p_dirFd = opendir(a_path)) == NULL){
		perror("service_readDir()");
	} else {
		/*Leyendo el contenido del buffer*/
		for(i = j = 0; (p_dirInfo = readdir(p_dirFd)); i = j){
			j += strlen(p_dirInfo->d_name);
			
			#ifndef NODEBUG
				printf("service_readDir(): archivo='%s', i=%d, j=%d\n", p_dirInfo->d_name, i, j);
			#endif
			
			/*Expandiendo el buffer + 1 para nueva linea*/
			if((buf = realloc(buf, (j+1) * sizeof(char))) == NULL) {
				closedir(p_dirFd);
				
				fprintf(stderr, "service_readDir(): la expansion del buffer ha fallado.\n");
				return false;
			}
			
			strcpy(&buf[i], p_dirInfo->d_name);
			buf[j++] = '\n'; /*Estableciendo nueva linea*/
		}
		
		closedir(p_dirFd);
		
		buf[j-1] = '\0'; /*Reemplazando la ultima nueva linea w/ null term*/
		*ap_length = j; /*Longitud*/
	}
	
	return buf;
}

Boolean service_writeFile(const String a_path, const String a_data, const int a_length){
	FILE* p_fileFd;
	Boolean result = false;
	
	if((p_fileFd = fopen(a_path, "w"))){
		if(fwrite(a_data, sizeof(char), a_length, p_fileFd) != -1){
			result = true;
			
			#ifndef NODEBUG
				printf("writeFile(): escribir archivo='%s', longitud=%d, &data=%p, data='%s'.\n", a_path, a_length, a_data, a_data);
			#endif
		} else {
			perror("service_writeFile() Problema");
		}
		
		fclose(p_fileFd);
	} else {
		perror("service_writeFile() Problema");
	}
	
	return result;
}

Boolean service_permTest(const String a_path, const String a_mode) {
	FILE *p_fileFd;
	Boolean result = false;
	
	if((p_fileFd = fopen(a_path, a_mode))) {
		fclose(p_fileFd);
		result = true;
	}
	
	return result;
}

Boolean service_statTest(const String a_path, const int a_testMode, const int a_resultMode){
	struct stat s;
	
	return (stat(a_path, &s) != -1) && ((s.st_mode & a_testMode) == a_resultMode);
}