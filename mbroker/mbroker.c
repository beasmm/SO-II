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


typedef struct Session {
    int num_active_sessions = 0;
    char* pipe_name;
    Boxes active_box[256];
    int num_active_box = 0;
} Session;

typedef struct Boxes {
    char* name;
    int num_active_subs = 0;
    int pub_activity = 0; 
} Boxes;

char* format_msg(char* buf, char msg[]){
    memcpy(buf, 10, sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t)+sizeof(char), msg, strlen(msg));
    return buf;
} 

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename>\n");
        return -1;
    }


    char* buffer;
    int op_code;
    char* client_named_pipe_path;
    char* box_name;
    int max_sessions = atoi(argv[2]);
    char msg[1024];

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
    tfs_init(NULL);


    while(true){
        /* Leitura de pedidos de registo */
        if(read(fd, buffer, sizeof(buffer)) < 0) {
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        char* token = strtok(answer, "|");
        for (int i = 0; token != NULL; i++) {
            if(i == 0) op_code = atoi(token);
            if(i == 2) client_named_pipe_path = token;
            if(i == 3) box_name = token;
            token = strtok(NULL, "|");
        }       
        

        /* Verificação pipes ativos com o mesmo nome */
        for(int i = 0; i < max_sessions; i++){
            if(strcmp(s.active_sessions[i].name, client_named_pipe_path) == 0){
                fprintf(stderr, "[ERR]: pipe already exists: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        /* Verificação sessões ativas */
        if(s.num_active_sessions == max_sessions){
            fprintf(stderr, "[ERR]: max sessions reached: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        switch (op_code){
            
            /* Processo publisher */        
            case 1:{
                for(int i = 0; i < s.num_active_box; i++){
                    if(strcmp(s.active_box[i], box_name) == 0 && s.active_box[i].pub_activity == 1){
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
                s.active_box[s.num_active_box].name = box_name;
                s.active_box[s.num_active_box].pub_activity = 1;
                s.num_active_box++;


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
                s.active_box[s.num_active_box].name = box_name;
                s.active_box[s.num_active_box].pub_activity = 0;
            }
            
            /* Processo subscriber */
            case 2:{
                int pipe_sub = open(client_named_pipe_path, O_WRONLY);
                if (pipe_sub < 0) {
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                
                s.num_active_sessions++;
                for(int i = 0; i < s.num_active_box; i++){
                    if(strcmp(s.active_box[i], box_name) == 0){
                        s.active_box[i].sub_activity++;
                        break;
                    }
                    else{
                        s.active_box[s.num_active_box].name = box_name;
                        s.active_box[s.num_active_box].sub_activity = 1;
                        s.num_active_box++;
                        break;
                    }
                }


                if(tfs_open(box_name, TFS_O_APPEND) < 0){
                    fprintf(stderr, "[ERR]: box open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if(tfs_read(box_name, msg, sizeof(msg)) < 0){
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                uint8_t buf[sizeof(uint8_t) + 1025*sizeof(char)] = {0};
                buf = format_msg(buf, msg);

                while(write(client_named_pipe_path, buf, sizeof(buf)) >= 0){
                    if(tfs_read(box_name, msg, sizeof(msg)) < 0){
                        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    char *new_box = box_name + 1024;
                    buf = format_msg(buf, msg);
                }

                tfs_close(box_name);
                close(pipe_sub);
                s.num_active_sessions--;
                for(int i = 0; i < s.num_active_box; i++){
                    if(strcmp(s.active_box[i], box_name) == 0){
                        s.active_box[i].sub_activity--;
                    }
                    if(s.active_box[i].sub_activity == 0 && s.active_box[i].pub_activity == 0){
                        s.active_box[i].name = NULL;
                        s.num_active_box--;
                    }                    
                } 
            }
        
            default:
                break;
        }













    }


    




    return -1;
}