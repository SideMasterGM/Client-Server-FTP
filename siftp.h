/*
Proporciona servicios de capa de transporte como el envío y
  Recepción de flujos de mensajes y mensajes gramos. Esto también
  Define el vocabulario y la sintaxis para la comunicación.
*/


#ifndef SIFTP_H
#define SIFTP_H

#include <string.h>

	/* constante */
	
		/* Formato de mensaje serializado: verbo | parámetro */
		
		// dialouge
			#define SIFTP_VERBS_SESSION_BEGIN	"HELO"
			#define SIFTP_VERBS_SESSION_END	"GBYE"
			
			#define SIFTP_VERBS_IDENTIFY	"IDNT"
			#define SIFTP_VERBS_USERNAME	"USER"
			#define SIFTP_VERBS_PASSWORD	"PASS"
			
			#define SIFTP_VERBS_ACCEPTED	"ACPT"
			#define SIFTP_VERBS_DENIED	"DENY"
			
			#define SIFTP_VERBS_PROCEED	"PRCD"
			#define SIFTP_VERBS_ABORT	"ABRT"
			
			#define SIFTP_VERBS_COMMAND	"CMND"
			#define SIFTP_VERBS_COMMAND_STATUS	"CMST"
			
			#define SIFTP_VERBS_DATA_STREAM_HEADER	"DSTH" /** param = [data_size_in_bytes] */
			#define SIFTP_VERBS_DATA_STREAM_HEADER_LENFMT	"%d"
			#define SIFTP_VERBS_DATA_STREAM_PAYLOAD	"DSTP"
			#define SIFTP_VERBS_DATA_STREAM_TAILER	"DSTT"
		
			#define SIFTP_FLAG	0x10
			
		// tamaño en bytes
			#define SIFTP_MESSAGE_SIZE	( 1 << 10 )
			#define SIFTP_VERB_SIZE	4
			#define SIFTP_PARAMETER_SIZE	( SIFTP_MESSAGE_SIZE - SIFTP_VERB_SIZE )

	/* typedefs */
	
		typedef struct TAG_Message Message;
		typedef enum { false, true } Boolean;
		typedef char* String;
		
	/* estructura de datos*/
	
		/**
		 * La primitiva de comunicación básica.
		 */
		struct TAG_Message
		{
			/** El tipo de mensaje / calificador / preámbulo*/
			char m_verb[SIFTP_VERB_SIZE+1];
			
			/** El contenido del mensaje */
			char m_param[SIFTP_PARAMETER_SIZE+1];
		};
		
		/* constructores*/
			
			/**
			 * Construye un objeto de mensaje.
			 * @note	Devuelve un objeto asignado dinámicamente.
			 */
			Message* Message_create(String a_verb, String a_param);
			
			/**
			 * Destruye el objeto de mensaje*/
			void Message_destroy(Message *ap_msg);
			
		/* Accesores */
		
			/**
			 * Devuelve el tipo de mensaje.
			 * @pre	ap_msg != NULL
			 */
			#define Message_getType(ap_msg) ( (ap_msg)->m_verb )
			
			/**
			 * Changes the type of the Message.
			 * @pre	ap_msg != NULL
			 * @pre	arg is null terminated
			 */
			#define Message_setType(ap_msg, arg) ( strcpy((ap_msg)->m_verb, arg) )
			
			/**
			 * Devuelve el valor del mensaje.
			 * @pre	ap_msg != NULL
			 */
			#define Message_getValue(ap_msg) ( (ap_msg)->m_param )
			
			/**
			 * cambia el valor del mensaje.
			 * @pre	ap_msg != NULL
			 * @pre	arg is null terminated
			 */
			#define Message_setValue(ap_msg, arg) ( strcpy((ap_msg)->m_param, arg) )
			
			/**
			 * Comprueba si el tipo de mensaje es igual al tipo dado.
			 * @pre	ap_msg != NULL
			 * @pre	value is null terminated
			 */
			#define Message_hasType(ap_msg, type) ( strcmp((ap_msg)->m_verb, type) == 0 )
			
			/**
			 * Comprueba si el tipo de mensaje es igual al tipo dado.
			 * @pre	ap_msg != NULL
			 * @pre	value is null terminated
			 */
			#define Message_hasValue(ap_msg, value) ( strcmp((ap_msg)->m_param, value) == 0 )
			
			/**
			 * Rellena los valores del miembro Mensaje con ceros.
			 * @pre	ap_msg != NULL
			 */
			#define Message_clear(ap_msg) ( memset(ap_msg, 0, sizeof(Message)) )
		
		
	/* servicios*/
	
		/**
		 * Escapa los indicadores FTP en la cadena dada.
		 * @note	Devuelve un objeto asignado dinámicamente.
		 * @deprecated	Utilizando mensajes de longitud fija.
		 */
		String siftp_escape(const String a_str);
		
		/**
		 * Despeja los flags de TP en la carga útil
		 * @note	Devuelve un objeto asignado dinámicamente.
		 * @deprecated	Utilizando mensajes de longitud fija.
		 */
		String siftp_unescape(const String a_str);
		
		/**
		 * Serializa un objeto Message para el transporte de red.
		 * @param	ap_msg		Objeto a serializar.
		 * @param	a_result	Almacenamiento (tamaño mínimo = SIFTP_MESSAGE_SIZE) donde se colocará la cadena serializada resultante.
		 */
		Boolean siftp_serialize(const Message *ap_msg, String a_result);
		
		/**
		 *deserealiza un  objeto Message para el transporte de red.
		 * @param	a_str		cadena a deserealizar.
		 * @param	ap_result	Almacenamiento donde el objeto deserializado resultante será colocado
		 */
		Boolean siftp_deserialize(const String a_str, Message *ap_result);
		
		/**
		 * Realiza un dialouge unidireccional.
		 */
		Boolean siftp_send(const int a_socket, const Message *ap_query);
		
		/**
		 * Espera un dialouge unidireccional.
		 */
		Boolean siftp_recv(const int a_socket, Message *ap_response);
		
		/**
		 * Realiza un dialouge de transferencia de datos.
		 * @param	a_socket	descriptor Socket en el que enviar.
		 * @param	a_data	Los datos a enviar.
		 * @param	a_length	numero de bytes a enviar.
		 */
		Boolean siftp_sendData(const int a_socket, const String a_data, const int a_length);
		
		/**
		 * Devuelve los datos de un dialouge de transferencia. Los datos devueltos terminan en nulo.
		 * @param	a_socket	 descriptor de Socket en el que desea recibir.
		 * @param	ap_length	Almacenamiento de la longitud de los datos recibidos (excluyendo el terminador nulo).
		 * @note	Devuelve un objeto asignado dinámicamente.
		 */
		String siftp_recvData(const int a_socket, int *ap_length);
#endif