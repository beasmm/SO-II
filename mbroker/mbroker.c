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
#include <fs/operations.h>

struct Session {
    int num_active_sessions;
    char* pipe_name;
    clients active_sessions[];
    char* active_box[];
    int num_active_box;
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
    char* op_code_str;
    int op_code;
    char* client_named_pipe_path;
    char* box_name;
    int max_sessions = atoi(argv[2]);

    Session s;
    s.pipe_name = argv[1];
    s.num_active_sessions = 0;



    if (mkfifo(s.pipe_name, 0666) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int fd = open(s.pipe_name, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    tfs_init();


    while(true){
        /* Leitura de pedidos de registo */
        if(read(fd, buffer, sizeof(buffer)) < 0) {
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        /* Verificação do op_code */
        for(int i = 0; i < strlen(buffer); i++){
            if(buffer[i] != '|'){
                op_code_str = buffer[i];
                break;
            }
        }
        op_code = atoi(op_code_str);
        if (op_code < 1){
            fprintf(stderr, "[ERR]: invalid op_code: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        /* Leitura de client_named_pipe_path */
        for(int i = uint8_t; i < strlen(buffer); i++){
            if(buffer[i] != '|'){
                client_named_pipe_path += buffer[i];
            }
            else{
                break;
            }
        }
        
        /* Leitura de box_name */
        for(int i = uint8_t + 256*sizeof(char); i < strlen(buffer); i++){
            if(buffer[i] != '|'){
                box_name += buffer[i];
            }
            else{
                break;
            }
        }

        /* Verificação pipes ativos com o mesmo nome */
        for(int i = 0; i < s.max_sessions; i++){
            if(strcmp(s.active_sessions[i].name, client_named_pipe_path) == 0){
                fprintf(stderr, "[ERR]: pipe already exists: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        /* Verificação sessões ativas */
        if(s.num_active_sessions == s.max_sessions){
            fprintf(stderr, "[ERR]: max sessions reached: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        switch (op_code){
            /* Criação de processo publisher */        
            case 1:{
                for(int i = 0; i < s.num_active_box; i++){
                    if(strcmp(s.active_box[i], box_name) == 0){
                        fprintf(stderr, "[ERR]: box already active: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }           

                int pipe_pub = open(client_named_pipe_path, O_RDONLY);
                if (pipe_pub < 0) {
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                s.num_active_sessions++;
                s.num_active_box++;
                s.active_sessions[s.num_active_sessions-1].name = client_named_pipe_path;
                s.active_sessions[s.num_active_sessions-1].type = 1;
                s.active_box[s.num_active_box-1] = box_name;


                if(tfs_open(box_name, TFS_O_APPEND) < 0){
                    fprintf(stderr, "[ERR]: box open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                while(true){
                    ssize_t ret = read(pipe_pub, buffer, sizeof(buffer));
                    if(ret < 0) {
                        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    if(ret < 0){
                        if(tfs_write(box_name, buffer, sizeof(buffer))){
                            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                    }
                    if(ret == 0){
                        return 0;
                    }
                }
                tfs_close(box_name);
                close(pipe_pub);
                s.num_active_sessions--;
                s.num_active_box--;
                s.active_sessions[s.num_active_sessions-1].name = "";
                s.active_sessions[s.num_active_sessions-1].type = 0;
                s.active_box[s.num_active_box-1] = "";

            }
            /* Criação de processo subscriber */
            case 2:{
                int pipe_sub = open(client_named_pipe_path, O_RDONLY);
                if (pipe_sub < 0) {
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                s.num_active_sessions++;
                s.active_sessions[s.num_active_sessions-1].name = client_named_pipe_path;
                s.active_sessions[s.num_active_sessions-1].type = 2;

                if(tfs_open(box_name, TFS_O_APPEND) < 0){
                    fprintf(stderr, "[ERR]: box open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if(tfs_read(box_name, buffer, sizeof(buffer)) < 0){
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }


                




            }
















                break;
        
            default:
                break;
        }













    }


    




    return -1;
}