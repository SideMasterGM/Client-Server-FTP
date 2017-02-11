#ifndef SERVICE_H
#define SERVICE_H

#include "siftp.h"

	/* constants */
	
		/**máxima longitud del nombre de un comando de servicio */
		#define SERVICE_COMMAND_SIZE	4
		
		/** Separadores de argumentos en una cadena de comandos */
		#define SERVICE_COMMAND_ARGUMENT_DELIMITERS	" \t"
		
		#define SERVICE_PERMS_READ_TEST	"r"
		#define SERVICE_PERMS_WRITE_TEST	"a"
		
	/* servicios */
	
		/**
		 * Establece una sesión de FTP.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean session_create(const int a_socket);
		
		/**
		 * Destruye una sesión FTP establecida.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean session_destroy(const int a_socket);
		
		/**
		 * Realiza interacción / dialouge.
		 * @param	a_socket	Socket descriptor.
		 */
		void service_loop(const int a_socket);
	
		/**
		 * Realiza una simple consulta bidireccional y dialouge de respuesta. 
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_query	Query message.
		 * @param	ap_response	Storage for response message.
		 */
		Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response);
		
		/**
		 * Devuelve una cadena que contiene la ruta absoluta de la extensión relativa a la ruta base.
		 * @param	a_basePath El origen al que se refiere la extensión, a menos que la extensión sea en sí misma un camino absoluto.
		 * @param	a_extension	El camino que queremos hacer absoluto.
		 * @param a_result	Almacenamiento, que debe tener una capacidad mínima de <tt> PATH_MAX </ tt>, para la ruta resultante.
		 */
		Boolean service_getAbsolutePath(const String a_basePath, const String a_extension, String a_result);
		
		/**
		 * Envía un reconocimiento de estado de comando para un comando.
		 * @param	a_socket	Socket descriptor.
		 * @param	a_wasSuccess	Status value.
		 */
		Boolean service_sendStatus(const int a_socket, const Boolean a_wasSuccess);

		/**
		 *Devuelve una matriz de palabras analizadas de la cadena de comandos. Supone que las palabras están separadas por caracteres <tt> SERVICE_COMMAND_ARGUMENT_DELIMITERS </ tt>.
		 * @param	a_cmdStr	String to parse.
		 * @param	ap_argc	Storage for length of array.
		 * @note	returns a dynamically allocated object.
		 */
		String* service_parseArgs(const String a_cmdStr, int *ap_argc);
		
		/**
		 * Libera memoria asociada con la matriz creada exclusivamente por <tt>parseArgs()</tt>.
		 * @param	ap_argv	Array of arguments gotten from <tt>parseArgs()</tt>.
		 * @param	a_argc	Number of items in array.
		 */
		void service_freeArgs(String *ap_argv, const int a_argc);
		
		/**
		 * Devuelve el valor del comando staus recognlegdement.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean service_recvStatus(const int a_socket);

		/**
		 * Realiza un comando remoto y devuelve su estado.
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_query	Message containing command.
		 */
		Boolean remote_exec(const int a_socket, Message *ap_query);
		
		/**
		 * handle es un comando que ocurre en la interacción / dialouge..
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_argv	Array of arguments.
		 * @param	a_argc	Number of arguments in array.
		 */
		Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc);
		
		/**
		 * Cambia la ruta del directorio de trabajo actual.
		 * @param	a_currPath	Almacenamiento para valor resuelto y valor del directorio de trabajo actual.
		 * @param	a_newPath	La ruta a la que queremos cambiar el directorio de trabajo actual.
		 */
		Boolean service_handleCmd_chdir(String a_currPath, const String a_newPath);
		
		/**
		 * Devuelve true si la ruta de acceso es accesible bajo los permisos dados.
		 * @param	a_path	Path to test.
		 * @param	a_mode	Access mode (same as fopen()).
		 * @see <tt>fopen</tt> for access modes.
		 */
		Boolean service_permTest(const String a_path, const String a_mode);
		
		/**
		 * Devuelve true si la ruta tiene todos los atributos dados. <Tt> errno </ tt> también se establece al fallar.
		 * @param	a_path	Ruta de acceso a la prueba..
		 * @param	a_testMode	Atributos para la prueba.
		 * @param	a_resultMode	Resultados esperados de la prueba.
		 * @see	stat.st_mode
		 */
		Boolean service_statTest(const String a_path, const int a_testMode, const int a_resultMode);
		
		/**
		 * Devuelve el contenido de un archivo o NULL al error.
		 * @param	a_path	Ruta del archivo a leer.
		 * @param	ap_length	almacenamiento de la longitud de los datos.
		 * @note	Devuelve un objeto asignado dinámicamente.
		 */
		String service_readFile(const String a_path, int *ap_length);
		
		/**
		 * Devuelve los nombres de todos los archivos del directorio especificado. Cada nombre de entrada de directorio está separado por una nueva línea. La nueva línea de arrastre se reemplaza por un terminador nulo.
		 * @param	a_path	Ruta del directoriio a leer.
		 * @param	ap_length	Almacenamiento para la longitud de los datos.
		 * @note	Devuelve un objeto asignado dinámicamente.
		 */
		String service_readDir(const String a_path, int *ap_length);
		
		/**
		 * Escribe datos mientras sobrescribe cualquier archivo existente en la ruta dada.
		 * @param	a_path	Ruta a la que se escribirán los datos.
		 * @param	a_data	Datos a escribir.
		 * @param	a_length	numero d bytes de los datos a escribir.
		 */
		Boolean service_writeFile(const String a_path, const String a_data, const int a_length);
		
#endif