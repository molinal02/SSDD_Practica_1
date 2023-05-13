#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

const char* filename_register_prefix= "data_";     // Prefijo en ruta para fichero de datos de registo de usuario N
const char* filename_msg_prefix = "msg_";          // Prefijo en ruta para fichero de mensajes pendientes para usuario N
const char* ext = ".dat";                           // Extension del fichero de usuario (data y mensajes pendientes)
const char* dirname =  "users";                   // Nombre del directorio principal de registros
const char* register_prefix = "register_";          // Prefiero para directorio de registro N


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
    char* status;
    unsigned int id;
    int num_users;
    char users;
} Service;

// Estructura para almacenar datos del usuario
typedef struct user{
    char usuario[64];
    char alias[64];
    char fecha[11];
    int estado;
    char IP[INET_ADDRSTRLEN];
    char port_escucha[6];
    char pend_mensajes[256];
    unsigned int id_msg;
} User;

// Registra a un usuario
char* register_serv(char *username, char *alias, char *date){
    
    DIR* user_dir;

    // Creamos el nombre del directorio
    char user_dirname [1024];
    sprintf(user_dirname, "./%s/%s%s", dirname, register_prefix, alias);

    // Comprobamos si existe
    if ((user_dir = opendir(user_dirname)) != NULL){
        perror("[SERVIDOR][ERROR] El directorio del usuario ya existe\n");
        return "1";
    }
    
    if (mkdir(user_dirname, 0755) == -1) {
        perror("[SERVIDOR][ERROR] El directorio no pudo ser creado\n");
        return "2";
    }

    // Creamos el nombre del fichero de datos personales de usuario
    char user_file [2048];
    sprintf(user_file, "%s/%s%s%s", user_dirname, filename_register_prefix, alias, ext);

    // Obtiene nombre completo del fichero de mensajes
    char messages_file [2048];
    sprintf(messages_file, "%s/%s%s%s", user_dirname, filename_msg_prefix, alias, ext);

    // Creamos el fichero de registro
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "w")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero para el registro del usuario no pudo ser creado\n");
        return "2";
    }

    // Creamos el fichero de  mensajes
    FILE* messages_fp;
    
    if ((messages_fp = fopen(messages_file, "w")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario para mensajes no pudo ser abierto\n");
        return "2";
    }

    fclose(messages_fp);

    // Creamos el fichero
    User user;
    strcpy(user.usuario, username);
    strcpy(user.alias, alias);
    strcpy(user.fecha, date);
    user.estado = 0;
    strcpy(user.pend_mensajes, messages_file);
    user.id_msg = 0;

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return "2";
    }

    fclose(user_fp);
    return "0";
}


// Da de baja a un usuario
char* unregister_serv(char *alias){

    DIR* user_dir;
    struct dirent *user_entry;
    char entry_path[512];

    // Creamos el nombre del directorio
    char user_dirname [1024];
    sprintf(user_dirname, "./%s/%s%s", dirname, register_prefix, alias);

    // Comprobamos si existe
    if ((user_dir = opendir(user_dirname)) == NULL){
        perror("[SERVIDOR][ERROR] El directorio del usuario no existe\n");
        return "1";
    }

    // Elimina los ficheros con extesion .dat
    while ((user_entry = readdir(user_dir)) != NULL) {
            if (strstr(user_entry->d_name, ".dat") != NULL){
                snprintf(entry_path, 512, "%s/%s%s/%s", dirname, register_prefix, alias, user_entry->d_name);
                remove(entry_path);
            }
    }

    if (rmdir(user_dirname) == -1) {
        perror("[SERVIDOR][ERROR] El directorio no pudo ser eliminado\n");
        return "2";
    }

    return "0";
}


// Conecta a un usuario
char* connect_serv(char *alias, char *IP, char *port_escucha){

    DIR* user_dir;

    // Creamos el nombre del directorio
    char user_dirname [1024];
    sprintf(user_dirname, "./%s/%s%s", dirname, register_prefix, alias);

    // Comprobamos si existe
    if ((user_dir = opendir(user_dirname)) == NULL){
        perror("[SERVIDOR][ERROR] El directorio del usuario no existe\n");
        return "1";
    }

    // Creamos el nombre del fichero de datos personales de usuario
    char user_file [2048];
    sprintf(user_file, "%s/%s%s%s", user_dirname, filename_register_prefix, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Registro de usuario no existe\n");
        return "1";
    }

    // Abrimos el fichero registro
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario para registro no pudo ser abierto\n");
        return "3";
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        return "3";
    }

    printf("Usuario: %s\n", user.usuario);
    printf("Alias: %s\n", user.alias);
    printf("Estado: %d\n", user.estado);
    printf("IP: %s\n", user.IP);
    printf("Port: %s\n", user.port_escucha);


    if (user.estado == 1) {
        perror("[SERVIDOR][ERROR] Usuario ya conectado\n");
        fclose(user_fp);
        return "2";
    }

    // Actualizamos el fichero
    strcpy(user.IP, IP);
    strcpy(user.port_escucha, port_escucha);

    user.estado = 1;

    rewind(user_fp);

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return "3";
    }

    fclose(user_fp);

    return "0";

    /*-----------  REVISAR ESTO -------------
    
    Falta ver como enviar los mensajes pendientes al conectarse

      -----------  REVISAR ESTO -------------*/
}

// Desconecta a un usuario
char* disconnect_serv(char *alias){

    DIR* user_dir;

    // Creamos el nombre del directorio
    char user_dirname [1024];
    sprintf(user_dirname, "./%s/%s%s", dirname, register_prefix, alias);

    // Comprobamos si existe
    if ((user_dir = opendir(user_dirname)) == NULL){
        perror("[SERVIDOR][ERROR] El directorio del usuario no existe\n");
        return "1";
    }

    // Creamos el nombre del fichero de datos personales de usuario
    char user_file [2048];
    sprintf(user_file, "%s/%s%s%s", user_dirname, filename_register_prefix, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Registro de usuario no existe\n");
        return "1";
    }

    // Abrimos el fichero registro
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del usuario para registro no pudo ser abierto\n");
        return "3";
    }

    // Leemos el fichero
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero\n");
        fclose(user_fp);
        return "3";
    }

    if (user.estado == 0) {
        perror("[SERVIDOR][ERROR] Usuario no está conectado\n");
        fclose(user_fp);
        return "2";
    }

    // Actualizamos el fichero
    user.estado = 0;

    rewind(user_fp);

    if (fwrite(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo escribir en el fichero\n");
        fclose(user_fp);
        return "3";
    }

    fclose(user_fp);

    return "0";

}

// Procesa pedición de envío de mensaje a un usuario
int send_serv(char *alias, char *destino, char *mensaje){

    Service service;

    DIR* user_dir;

    // Creamos el nombre del directorio del emisor
    char user_dirname [1024];
    sprintf(user_dirname, "./%s/%s%s", dirname, register_prefix, alias);

    // Comprobamos si existe el emisor
    if ((user_dir = opendir(user_dirname)) == NULL){
        perror("[SERVIDOR][ERROR] El directorio del emisor no existe\n");
        service.status = "2";
        return service;
    }

    // Creamos el nombre del fichero de datos personales del emisor
    char user_file [2048];
    sprintf(user_file, "%s/%s%s%s", user_dirname, filename_register_prefix, alias, ext);

    if (access(user_file, F_OK)){
        perror("[SERVIDOR][ERROR] Registro de usuario no existe\n");
        service.status = "2";
        return service;
    }

    // Abrimos el fichero registro del emisor
    FILE* user_fp;

    if ((user_fp = fopen(user_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del emisor para registro no pudo ser abierto\n");
        service.status = "2";
        return service;
    }

    // Leemos el fichero emisor
    User user;

    if (fread(&user, sizeof(User), 1, user_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero del emisor\n");
        fclose(user_fp);
        service.status = "2";
        return service;
    }

    if (user.estado == 0) {
        perror("[SERVIDOR][ERROR] Emisor no está conectado\n");
        fclose(user_fp);
        service.status = "2";
        return service;
    }

    DIR* dest_dir;

    // Creamos el nombre del directorio
    char dest_dirname [1024];
    sprintf(dest_dirname, "./%s/%s%s", dirname, register_prefix, destino);

    // Comprobamos si existe
    if ((dest_dir = opendir(dest_dirname)) == NULL){
        perror("[SERVIDOR][ERROR] El directorio del destinatario no existe\n");
        service.status = "1";
        return service;
    }

    // Creamos el nombre del fichero de datos personales del destinatario
    char dest_file [2048];
    sprintf(dest_file, "%s/%s%s%s", dest_dirname, filename_register_prefix, destino, ext);

    if (access(dest_file, F_OK)){
        perror("[SERVIDOR][ERROR] Registro de destinatario no existe\n");
        service.status = "1";
        return service;
    }

    // Abrimos el fichero registro del destinatario
    FILE* dest_fp;

    if ((dest_fp = fopen(dest_file, "r+")) == NULL){
        perror("[SERVIDOR][ERROR] El fichero del destinatario para registro no pudo ser abierto\n");
        service.status = "2";
        return service;
    }

    // Leemos el fichero emisor
    User dest;

    if (fread(&dest, sizeof(User), 1, dest_fp) == 0){
        perror("[SERVIDOR][ERROR] No se pudo leer el fichero del destinatario\n");
        fclose(dest_fp);
        service.status = "2";
        return service;
    }

    if (dest.estado == 0) {
        fclose(user_fp);
        fclose(dest_fp);
        service.status = "0";
        service.id = user.id_msg;
        return service;
    } else {
        fclose(user_fp);
        fclose(dest_fp);
        service.status = "3";
        service.id = user.id_msg;
        return service;
    }

}
/*
// Devuelve usuarios conectados
int connected_users_serv(char *alias){

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
}*/