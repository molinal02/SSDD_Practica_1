#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

const char* filename = "./users/user_";     // Ruta relativa del fichero de usuario
const char* ext = ".dat";                     // Extension del fichero de usuario
const char* dirname =  "users";              // Nombre del directorio

// Estructura para información
typedef struct info{
    char usuario[64];
    char destinatario[64];
    char alias[32];
    char fecha[11];
    char port_escucha[6];
    char mensaje[256];
} Info;

// Estructura para devolver información
typedef struct service{
    int status;
    unsigned int id;
    int num_users;
    char users;
} Service;

// Estructura para almacenar datos del usuario
typedef struct user{
    char usuario[64];
    char alias[64];
    char fecha[11];
    int estado = 0;
    char IP[INET_ADDRSTRLEN] = "0";
    char port_escucha[6] = "0";
    char pend_mensajes [5120] = "";
    unsigned int ultimo_recibido = 0;
} User;

// Registra a un usuario
int register_serv(char *user, char *alias, char *date){

    // Creamos el nombre del fichero
    char user_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (!access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario ya existente\n");
        return 1;
    }

    // Creamos el fichero
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "w")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero para el registro del usuario no pudo ser creado\n");
        return 2;
    }

    // Creamos el fichero
    User user;
    strcpy(user.usuario, user);
    strcpy(user.alias, alias);
    strcpy(user.fecha, date);

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return 2;
    }

    fclose(user_fp);
    return 0;

}

// Da de baja a un usuario
int unregister_serv(char *alias){

    // Obtiene nombre completo del fichero
    char user_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario no existe\n");
        return 1;
    }
    
    if (remove(user_file) == -1){
        perror("[SERVIDOR][ERROR] El registro no pudo ser eliminado\n");
        return 2;
    }

    return 0;

    /*-----------  REVISAR ESTO -------------
    
    Falta borrar todos los mensajes que iban para él de otros usuarios

      -----------  REVISAR ESTO -------------*/
}

// Conecta a un usuario
int connect_serv(char *alias, char *IP, char *port_escucha){
    
    // Obtiene nombre completo del fichero
    char user_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario no existe\n");
        return 1;
    }

    // Abrimos el fichero
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario no pudo ser abierto\n");
        return 3;
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        return 3;
    }

    // Actualizamos el fichero
    strcpy(user.IP, IP);
    strcpy(user.port_escucha, port_escucha);

    if (user.estado == 1) {
        perror("[SERVIDOR][ERROR] Usuario ya conectado\n");
        fclose(user_fp);
        return 2;
    }

    user.estado = 1;

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return 3;
    }

    fclose(user_fp);
    return 0;

    /*-----------  REVISAR ESTO -------------
    
    Falta ver como enviar los mensajes pendientes al conectarse

      -----------  REVISAR ESTO -------------*/
}

// Desconecta a un usuario
int disconnect_serv(char *alias){
    
    // Obtiene nombre completo del fichero
    char user_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario no existe\n");
        return 1;
    }

    // Abrimos el fichero
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario no pudo ser abierto\n");
        return 3;
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        return 3;
    }

    // Actualizamos el fichero
    if (user.estado == 0) {
        perror("[SERVIDOR][ERROR] Usuario ya desconectado\n");
        fclose(user_fp);
        return 3;
    }

    if (user.estado == 0) {
        perror("[SERVIDOR][ERROR] Usuario no conectado\n");
        fclose(user_fp);
        return 2;
    }

    // Actualizamos el fichero
    strcpy(user.IP, "0");
    strcpy(user.port_escucha, "0");
    user.estado = 0;

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return 3;
    }

    fclose(user_fp);
    return 0;
}

// Procesa pedición de envío de mensaje a un usuario
int send_serv(char *destino, char *alias, char *mensaje){
    // Primero se comprueba si estan ambos registrados
    // Obtiene nombre completo del fichero
    char user_file [1024];
    char destino_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario no existe\n");
        return 2;
    }

    sprintf(destino_file, "%s%d%s", filename, destino, ext);

    if (access(destino_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario destino no existe\n");
        return 2;
    }

    // Abrimos el fichero usuario
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario no pudo ser abierto\n");
        respuesta.status = 2;
        return respuesta;
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        respuesta.status = 2;
        return respuesta;
    }

    /*-----------  REVISAR ESTO -------------
    
                    Faltan cosas

      -----------  REVISAR ESTO -------------*/

    // Abrimos el fichero destino
    FILE* destino_fp;

    if ((destino_fp = fopen(destino_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del destinatario no pudo ser abierto\n");
        respuesta.status = 2;
        return respuesta;
    }

    // Leemos el fichero del usuario destino
    User destino;

    if (fread(&destino, sizeof(User), 1, destino_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero del destinatario\n");
        fclose(destino_fp);
        respuesta.status = 2;
        return respuesta;
    }

    // Comprobamos si el usuario destino está conectado
    if (destino.estado == 0) {
        perror("[SERVIDOR][ERROR] Usuario destino no conectado\n");
        fclose(destino_fp);
        respuesta.status = 2;
        return respuesta;
            /*-----------  REVISAR ESTO -------------
    
                    No se si se menciona lo que tiene que devolver en este caso

            -----------  REVISAR ESTO -------------*/
    }

    // Enviamos el mensaje a la direccion IP del hilo receptor del destinatario
    char *IP = destino.IP;

    /*-----------  REVISAR ESTO -------------
    
    Mirar si usamos sockets aqui o mandamos la ip a servidor.c para mandarlo.

    -----------  REVISAR ESTO -------------*/


}

// Devuelve usuarios conectados
int connected_users_serv(char *alias){

    /*-----------  REVISAR ESTO -------------
    
    Mirar como hacer que se verifique si está registrado y conectado quien llama a la función (ya que en teoría no manda el alias)

      -----------  REVISAR ESTO -------------*/

    Service respuesta;
    char conectados [4048];
    // Obtiene nombre completo del fichero
    char user_file [1024];
    sprintf(user_file, "%s%d%s", filename, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Usuario no existe\n");
        respuesta.status = 2;
        return respuesta;
    }

    // Abrimos el fichero
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario no pudo ser abierto\n");
        respuesta.status = 2;
        return respuesta;
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        respuesta.status = 2;
        return respuesta;
    }

    if (user.estado == 0) {
        perror("[SERVIDOR][ERROR] Usuario no conectado\n");
        fclose(user_fp);
        respuesta.status = 1;
        return respuesta;
    }

    // Leemos todos los ficheros del directorio
    DIR *users_dir;
    struct dirent *user_entry;

    if ((users_dir = opendir(dirname)) == NULL) {
        perror("[SERVIDOR][ERROR] No se pudo abrir el directorio\n");
        respuesta.status = 2;
        return respuesta;
    }

    bool primero = true;
    while ((user_entry = readdir(d)) != NULL) {
        if (strstr(user_entry->d_name, ".dat") != NULL){

            if ((user_fp = fopen(user_entry->d_name, "r+")) == NULL){
                perror("[SERVIDOR][ERROR] El fichero del usuario no pudo ser abierto\n");
                respuesta.status = 2;
                return respuesta;
            }

            if (fread(&user, sizeof(User), 1, user_fp) == 0){
                perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
                fclose(user_fp);
                respuesta.status = 2;
                return respuesta;
            }

            if (user.estado == 1) {
                char *persona = strtok(user_entry->d_name, ext);
                if (primero) {
                    sprintf(conectados, "%s", persona);
                    primero = false;
                } else {
                    sprintf(conectados, "%s,%s", conectados, persona);
                }
            }
        }
    }
    closedir(d);

    respuesta.status = 0;
    strcpy(respuesta.users, conectados);

    return respuesta;
}