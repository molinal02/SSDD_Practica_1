#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ifaddrs.h>
#include <limits.h>

#include "comunicacion.h"

int mensaje_no_copiado = 1;                                 // Condicion para el mutex de paso de mensajes
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex para ejecucion de operaciones en direcotrio y ficheros
pthread_mutex_t mutex_msg = PTHREAD_MUTEX_INITIALIZER;      // Mutex para control de paso de mensajes
pthread_cond_t condvar_msg = PTHREAD_COND_INITIALIZER;      // Var. condicion asociada al mutex de paso de mensajes

// Prototipos de la API del lado servidor
char* register_serv(char *user, char *alias, char *date);
char* unregister_serv(char *alias);
char* connect_serv(char *alias, char *IP, char *port_escucha);
char* disconnect_serv(char *alias);
char* send_serv(char *alias, char *destino, char *mensaje);
int connected_users_serv(char *alias);


void tratar_pet (void* pet){

    Request peticion;
    Service respuesta;

    // Hilo obtiene la peticion del cliente
    pthread_mutex_lock(&mutex_msg);
    peticion = (*(Request *) pet);
    mensaje_no_copiado = 0;
    pthread_cond_signal(&condvar_msg);
    pthread_mutex_unlock(&mutex_msg);

    if (strcmp(peticion.op, "REGISTER") == 0){
        
        // Obtiene acceso exclusivo al directorio "users" y sus ficheros
        pthread_mutex_lock(&users_mutex);
        respuesta.status = register_serv(peticion.content.usuario, peticion.content.alias, peticion.content.fecha);
        pthread_mutex_unlock(&users_mutex);
        //respuesta.status = ntohl(respuesta.status);

        // Se envia la respuesta al cliente
        if (sendMessage(peticion.sock_client, respuesta.status, strlen(respuesta.status)+1) == -1)
            perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente\n");
            
    } else if(strcmp(peticion.op, "UNREGISTER") == 0){

        // Obtiene acceso exclusivo al directorio "users" y sus ficheros
        pthread_mutex_lock(&users_mutex);
        respuesta.status = unregister_serv(peticion.content.alias);
        pthread_mutex_unlock(&users_mutex);

        // Se envia la respuesta al cliente
        if (sendMessage(peticion.sock_client, respuesta.status, strlen(respuesta.status)+1) == -1)
            perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente\n");

    } else if(strcmp(peticion.op, "CONNECT") == 0){

        // Obtiene acceso exclusivo al directorio "users" y sus ficheros
        pthread_mutex_lock(&users_mutex);
        respuesta.status = connect_serv(peticion.content.alias, peticion.content.IP, peticion.content.port_escucha);

        // Se envia la respuesta al cliente
        if (sendMessage(peticion.sock_client, respuesta.status, strlen(respuesta.status)) == -1)
            perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente\n");

        if (strcmp(respuesta.status, "0") == 0){

            // Se crea la ruta al fichero de mensajes del usuario
            char ruta_fichero[2048];
            sprintf(ruta_fichero, "users/register_%s/msg_%s.dat",peticion.content.alias, peticion.content.alias);
            
            // Creamos el fichero de registro
            FILE* user_fp;

            if ((user_fp = fopen(ruta_fichero, "r+")) == NULL){
                perror("[SERVIDOR][ERROR] El fichero de mensajes del usuario no pudo ser abierto\n");
            }

            char mensaje[256];
            while (fgets(mensaje, sizeof(mensaje), user_fp) != NULL) {
                // Enviamos el mensaje
                if (sendMessage(peticion.sock_client, mensaje, strlen(mensaje)+1) == -1)
                    perror("[SERVIDOR][ERROR] No se pudo enviar el mensaje al cliente\n");
            }

            // Cerramos el archivo
            fclose(user_fp);
        }

        pthread_mutex_unlock(&users_mutex);        
        
    } else if(strcmp(peticion.op, "DISCONNECT") == 0){

            // Obtiene acceso exclusivo al directorio "users" y sus ficheros
            pthread_mutex_lock(&users_mutex);
            respuesta.status = disconnect_serv(peticion.content.alias);
            pthread_mutex_unlock(&users_mutex);

            // Se envia la respuesta al cliente
            if (sendMessage(peticion.sock_client, respuesta.status, strlen(respuesta.status)+1) == -1)
                perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente\n");
                
    } else if (strcmp(peticion.op, "SEND") == 0){
        
        // Obtiene acceso exclusivo al directorio "users" y sus ficheros
        pthread_mutex_lock(&users_mutex);
        respuesta.content = send_serv(peticion.content.alias, peticion.content.destinatario, peticion.content.mensaje); 
        
        if (strcmp(respuesta.content.status, "1") || strcmp(respuesta.content.status, "2")){
            // Se envia la respuesta de status al cliente
            if (sendMessage(peticion.sock_client, respuesta.content.status, strlen(respuesta.content.status)+1) == -1)
                perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (status)\n");
        }

        if (strcmp(respuesta.content.status, "0")){
            Message mensaje;
            mensaje.id += respuesta.content.id;
            mensaje.id %= UINT_MAX;
            mensaje.alias_emisor = peticion.content.alias;
            mensaje.mensaje = peticion.content.mensaje;

            // Enviamos el mensaje al destinatario
            
        }

        pthread_mutex_unlock(&users_mutex);

        // Se envia la respuesta de status al cliente
        if (sendMessage(peticion.sock_client, (char*) &respuesta.content.status, sizeof(int32_t)) == -1)
            perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (status)\n");
        
        
        // Se comprueba si ha salido bien
        if (status == 0) {
            // Se envia la respuesta de id al cliente
            if (sendMessage(peticion.sock_client, (char*) &respuesta.content.id, sizeof(unsigned int)) == -1)
                perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (id)\n");
        }

    } /*else if (strcmp(peticion.op, "CONNECTEDUSERS") == 0){

        // Obtiene acceso exclusivo al directorio "users" y sus ficheros
        pthread_mutex_lock(&users_mutex);
        respuesta.status = connected_users_serv(peticion.content.alias);
        pthread_mutex_unlock(&users_mutex);
        int status = respuesta.content.status;
        respuesta.content.status = ntohl(respuesta.content.status);

        // Se envia la respuesta de status al cliente
        if (sendMessage(peticion.sock_client, (char*) &respuesta.content.status, sizeof(int32_t)) == -1)
            perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (status)\n");
        
        
        // Se comprueba si ha salido bien
        if (status == 0) {

            // Se convierte el número a cadena
            char num_users[50]; // Se da por hecho que no habrá más de estos usuarios
            sprintf(num_users, "%d", respuesta.content.num_users);

            // Se envia el número de usuarios conectados
            if (sendMessage(peticion.sock_client, (char*) num_users, sizeof(num_users)) == -1)
                perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (número de usuarios)\n");
            
            // Se envían los usuarios conectados
            if (sendMessage(peticion.sock_client, (char*) &respuesta.content.users, sizeof(respuesta.content.users)) == -1)
                perror("[SERVIDOR][ERROR] No se pudo enviar la respuesta al cliente (número de usuarios)\n");
            
        }

    } */else {
        perror("[SERVIDOR][ERROR] Operación no válida\n");
    }
    // Se cierra el socket del cliente
    if (close(peticion.sock_client) == -1){
        perror("[SERVIDOR][ERROR] Socket del cliente no pudo cerrarse\n");
    }
    pthread_exit(NULL);
}


int main(int argc, char* argv[]){

    /*-----------  REVISAR ESTO -------------
    
    Mirar como hacer para que el servidor muestre en la interfaz que se ha conectado (no estoy seguro de que sea asi)

      -----------  REVISAR ESTO -------------*/

    // Estructura para agrupar la información
    Request peticion;

    // Manejo de hilos
    pthread_t thid;
    pthread_attr_t th_attr; 

    // Informacion para el socket del servidor y del cliente
    int sock_serv_fd, sock_client_fd;
    struct sockaddr_in serv_addr, client_addr;

    // Variables para guardar lo recibido
    char op[15];
    char usuario[64];
    char destinatario[64];
    char alias[32];
    char fecha[11];
    char client_ip[INET_ADDRSTRLEN];
    char port_escucha[6];
    char mensaje[256];

    // Se comprueba que se ha introducido el puerto como argumento
    if (argc != 2){
        printf("[SERVIDOR][ERROR] Debe introducir el puerto como argumento\n");
        return -1;
    }

    // Se almacena el puerto introducido como argumento y se comprueba que es valido
    short puerto = (short) atoi(argv[1]);

    if (puerto < 1024 || puerto > 65535){
        printf("[SERVIDOR][ERROR] El puerto introducido no es valido\n");
        return -1;
    }

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char *desiredInterface = "eth0";  // Nombre de la interfaz de red deseada
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("Error al obtener la información de las interfaces de red");
        return 1;
    }

    // Recorre la lista de interfaces de red
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        // Filtra las interfaces de red IPv4 y coincide con el nombre deseado
        if (family == AF_INET && strcmp(ifa->ifa_name, desiredInterface) == 0) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("[SERVIDOR][ERROR] Error al obtener la dirección IP del servidor: %s\n", gai_strerror(s));
                return 1;
            }

        }
    }

    // Se muestra mensaje de inicio
    printf("init server <%s>:%d\n", host, puerto);
    printf("s>\n");

    freeifaddrs(ifaddr);

    // Se crea el socket del servidor
    if ((sock_serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[SERVIDOR][ERROR] No se pudo crear socket de recepción de peticiones\n");
        return -1;
    }
    
    // Se establece la opcion de reutilizacion de direcciones
    int val = 1;
    setsockopt(sock_serv_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));

    // Se inicializa la estructura de datos para el socket del servidor
    socklen_t client_addr_len = sizeof(client_addr);
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(puerto);

    // Se enlaza el socket del servidor con la direccion y puerto y se procede a ponerlo en modo escucha
    if (bind(sock_serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        perror("[SERVIDOR][ERROR] No se pudo enlazar el socket de recepción de peticiones\n");
        return -1;
    }

    if (listen(sock_serv_fd, SOMAXCONN) == -1){
        perror("[SERVIDOR][ERROR] No se pudo poner el socket en modo escucha\n");
        return -1;
    }

    // Se inicializa el mutex para el directorio "users" y sus ficheros
    pthread_attr_init(&th_attr);
    pthread_attr_setdetachstate(&th_attr,PTHREAD_CREATE_DETACHED);

    while(1){

        // Se acepta la conexion del cliente
        if ((sock_client_fd = accept(sock_serv_fd, (struct sockaddr*) &client_addr, &client_addr_len)) == -1){
            perror("[SERVIDOR][ERROR] No se pudo aceptar la conexión del cliente\n");
            break;
        }

        // Convertir dirección IP del cliente a una cadena legible
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);  

        // Se recibe la operación a realizar
        if (readLine(sock_client_fd, (char*) &op, sizeof(op)) == -1){
            perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (operacion)\n");
            break;
        }

        if (strcmp(op, "REGISTER") != 0){
            // Se recibe el alias
            if (readLine(sock_client_fd, (char*) &alias, sizeof(alias)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (alias)\n");
                break;
            }
        }

        if (strcmp(op, "REGISTER") == 0){

            // Se recibe el usuario
            if (readLine(sock_client_fd, (char*) &usuario, sizeof(usuario)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (usuario)\n");
                break;
            }

            // Se recibe el alias
            if (readLine(sock_client_fd, (char*) &alias, sizeof(alias)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (alias)\n");
                break;
            }
        
            // Se recibe la fecha de nacimiento
            if (readLine(sock_client_fd, (char*) &fecha, sizeof(fecha)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (fecha de nacimiento)\n");
                break;
            }
        }

        if (strcmp(op, "CONNECT") == 0) {

            // Se recibe el puerto de escucha del cliente
            if (readLine(sock_client_fd, (char*) &port_escucha, sizeof(port_escucha)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (puerto de escucha)\n");
                break;
            }

        }

        if (strcmp(op, "SEND") == 0) {

            // Se recibe el destinatario
            if (readLine(sock_client_fd, (char*) &destinatario, sizeof(destinatario)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (destinatario)\n");
                break;
            }

            // Se recibe el mensaje
            if (readLine(sock_client_fd, (char*) &mensaje, sizeof(mensaje)) == -1){
                perror("[SERVIDOR][ERROR] No se pudo recibir la petición del cliente (mensaje)\n");
                break;
            }

        }

        // Almacenamos lo leído en la estructura
        strcpy(peticion.op, op);
        peticion.sock_client = sock_client_fd;
        strcpy(peticion.content.IP, client_ip);
        strcpy(peticion.content.alias, alias);

        if (strcmp(op, "REGISTER") == 0){
            strcpy(peticion.content.usuario, usuario);
            strcpy(peticion.content.fecha, fecha);
        }
        if (strcmp(op, "CONNECT") == 0) {
            strcpy(peticion.content.port_escucha, port_escucha);
        }
        if (strcmp(op, "SEND") == 0) {
            strcpy(peticion.content.destinatario, destinatario);
            strcpy(peticion.content.mensaje, mensaje);
        }
        

        // Crea un hilo por peticion
        if(pthread_create(&thid, &th_attr, (void*) &tratar_pet, (void *) &peticion) == -1){
            perror("[SERVIDOR][ERROR] Hilo no pudo ser creado\n");
            break;
        }

        // Asegura que la peticion se copia correctamente en el hilo que atiende al cliente
        pthread_mutex_lock(&mutex_msg);
        while (mensaje_no_copiado) pthread_cond_wait(&condvar_msg, &mutex_msg);
            mensaje_no_copiado = 1;
            pthread_mutex_unlock(&mutex_msg);
        }

        // Se cierra el socket del servidor
        if (close(sock_serv_fd) == -1){
            perror("[SERVIDOR][ERROR] No se pudo cerrar el socket de recepción de peticiones\n");
            return -1;
        }
        return 0;
    }
