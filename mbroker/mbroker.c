#include "logging.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct Session {
    int max_sessions;
    char* pipe_name;
    clients active_sessions[];
} Session;

struct clients{
    int type;
    char* name;
} clients;


int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename>\n");
        return -1;
    }

    char* buffer;
    int op_code;
    char* client_named_pipe_path;
    char* box_name;

    Session s;
    s.pipe_name = argv[1];
    s.max_sessions = atoi(argv[2]);

    if (mkfifo(s.pipe_name, 0666) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int fd = open(s.pipe_name, O_RDONLY);

    while(true){
        /* Leitura de pedidos de registo */
        if(read(fd, buffer, sizeof(buffer)) < 0) {
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        /* Verificação do op_code */
        for(int i = 0; i < strlen(buffer); i++){
            if(buffer[i] == ' '){
                op_code = atoi(buffer[i]);
                break;
            }
        }
        if (op_code < 1){
            fprintf(stderr, "[ERR]: invalid op_code: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        /* Leitura de client_named_pipe_path */
        for(int i = uint8_t; i < strlen(buffer); i++){
            if(buffer[i] == ' '){
                client_named_pipe_path = buffer[i];
                break;
            }
        }
        
        /* Leitura de box_name */
        for(int i = uint8_t + 256*sizeof(char); i < strlen(buffer); i++){
            if(buffer[i] == ' '){
                box_name = buffer[i];
                break;
            }
        }

        switch (op_code){
            /* Criação de processo publisher */        
            case 1:





                break;
        
            default:
                break;
        }













    }


    /* Criação processos para cada tipo de client*/
    




    return -1;
}
