#include "logging.h"
#include "mbroker.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fs/operations.h>



void format_msg(uint8_t buf[], char msg[]){
    memcpy(buf, "10", sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t)+sizeof(char), msg, strlen(msg));
} 

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename>\n");
        return -1;
    }

    char* buffer = NULL;
    int op_code = 0;
    char* client_named_pipe_path;
    char* box_name = NULL;
    int max_sessions = atoi(argv[2]);
    char msg[1024] = {"\0"};

    Session s;
    s.num_active_sessions = 0;
    s.num_active_box = 0;
    s.pipe_name = argv[1];
    s.active_sessions = (pipename_t*)malloc((unsigned int)max_sessions * (sizeof(pipename_t))); 

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
        int tester = 0;
        
        /* Leitura de pedidos de registo */
        if(read(fd, buffer, sizeof(buffer)) < 0) {
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }else{
            char* token = strtok(buffer, "|");
            for (int i = 0; token != NULL; i++) {
                if(i == 0) op_code = atoi(token);
                if(i == 2) client_named_pipe_path = token;
                if(i == 3) box_name = token;
                token = strtok(NULL, "|");
            }
        }

        

        /* Verificação pipes ativos com o mesmo nome */
        for(int i = 0; i < s.num_active_sessions; i++){
            if(strcmp(s.active_sessions[i], client_named_pipe_path) == 0){ 
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
                    if(strcmp(s.active_box[i].name, box_name) == 0 && s.active_box[i].pub_activity == 1){
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


                int fhandle = tfs_open(box_name, TFS_O_APPEND);
                if (fhandle < 0) {
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
                        if(tfs_write(fhandle, buffer, sizeof(buffer))){
                            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                        s.active_box[s.num_active_box-1].size += (int)sizeof(buffer);
                    }
                    if(ret == 0){
                        return 0;
                    }
                }
                tfs_close(fhandle);
                close(pipe_pub);
                s.num_active_sessions--;
                s.num_active_box--;
                s.active_box[s.num_active_box].name = box_name;
                s.active_box[s.num_active_box].pub_activity = 0;
                break;
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
                    if(strcmp(s.active_box[i].name, box_name) == 0){
                        s.active_box[i].num_active_subs++;
                        tester = 1;
                        break;
                    }
                }
                if(tester == 0){
                    s.active_box[s.num_active_box].name = box_name;
                    s.active_box[s.num_active_box].num_active_subs = 1;
                    s.num_active_box++;
                }


                int fhandle = tfs_open(box_name, TFS_O_APPEND);
                if (fhandle < 0) {
                    fprintf(stderr, "[ERR]: box open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if(tfs_read(fhandle, msg, sizeof(msg)) < 0){
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                uint8_t buf[sizeof(uint8_t) + 1025*sizeof(char)] = {0};
                format_msg(buf, msg);

                while(write(pipe_sub, buf, sizeof(buf)) >= 0){
                    if(tfs_read(fhandle, msg, sizeof(msg)) < 0){
                        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    format_msg(buf, msg);
                }

                tfs_close(fhandle);
                close(pipe_sub);
                s.num_active_sessions--; 
                break;
            }

            case 3:{
                
                uint32_t return_code;
                int pipe_man = open(client_named_pipe_path, O_WRONLY);
                if (pipe_man < 0){
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                s.num_active_sessions++;
                uint8_t buf[sizeof(uint8_t) + sizeof(uint32_t) + 1027*sizeof(char)] = {0};
                   
                int fhandle = tfs_open(box_name, TFS_O_CREAT);
                if (fhandle < 0) {
                    return_code = (uint32_t)-1;
                    strcpy(msg, "[ERR]: box creation failed");
                }else return_code = 0;

                s.active_box[s.num_active_box].name = box_name;
                s.num_active_box++;

                memcpy(buf, "4", sizeof(uint8_t));
                memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
                memcpy(buf + sizeof(uint8_t) + sizeof(char), &return_code, sizeof(uint32_t));
                memcpy(buf + sizeof(uint8_t) + sizeof(char) + sizeof(uint32_t), "|", sizeof(char));
                memcpy(buf + sizeof(uint8_t) + 2*sizeof(char) + sizeof(uint32_t),msg, 1024*sizeof(char));

                if(write(pipe_man, buf, sizeof(buf)) < 0){
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                tfs_close(fhandle);
                close(pipe_man);
                s.num_active_sessions--;
                break;
            }

            case 5:{
                uint32_t return_code;
                int pipe_man = open(client_named_pipe_path, O_WRONLY);
                if (pipe_man < 0){
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                s.num_active_sessions++;
                uint8_t buf[sizeof(uint8_t) + sizeof(uint32_t) + 1027*sizeof(char)] = {0};

                int fhandle = tfs_unlink(box_name);
                if (fhandle < 0) {
                    return_code = (uint32_t)-1;
                    strcpy(msg, "[ERR]: box deletion failed");
                }else return_code = 0;

                memcpy(buf, "6", sizeof(uint8_t));
                memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
                memcpy(buf + sizeof(uint8_t) + sizeof(char), &return_code, sizeof(uint32_t));
                memcpy(buf + sizeof(uint8_t) + sizeof(char) + sizeof(uint32_t), "|", sizeof(char));
                memcpy(buf + sizeof(uint8_t) + 2*sizeof(char) + sizeof(uint32_t),msg, 1024*sizeof(char));

                if(write(pipe_man, buf, sizeof(buf)) < 0){
                    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                tfs_close(fhandle);
                close(pipe_man);
                s.num_active_sessions--;
                for(int i = 0; i < s.num_active_box; i++){
                    if(strcmp(s.active_box[i].name, box_name) == 0){
                        s.active_box[i].name = NULL;
                        s.active_box[i].num_active_subs = 0;
                        s.active_box[i].pub_activity = 0;
                        s.num_active_box--;
                        break;
                    }
                }break;
                break;
            }

            case 7:{
                int pipe_man = open(client_named_pipe_path, O_WRONLY);
                if(pipe_man < 0){
                    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                s.num_active_sessions++;
                uint8_t buf[2*sizeof(uint8_t) + 3*sizeof(uint64_t) + 37*sizeof(char)] = {0};

                for(int i= 0; i < s.num_active_box; i++){
                    memcpy(buf, "8", sizeof(uint8_t));
                    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
                    /* last */
                    if(i == s.num_active_box - 1){
                        memcpy(buf + sizeof(uint8_t) + sizeof(char), "1", sizeof(uint8_t));
                    }else{
                        memcpy(buf + sizeof(uint8_t) + sizeof(char), "0", sizeof(uint8_t));}
                    memcpy(buf + 2*sizeof(uint8_t) + sizeof(char), "|", sizeof(char));
                    /* box_name */
                    memcpy(buf + 2*sizeof(uint8_t) + 2*sizeof(char), &s.active_box[i].name, sizeof(char)*32);
                    memcpy(buf + 2*sizeof(uint8_t) + 34*sizeof(char), "|", sizeof(char));
                    /* box_size */
                    memcpy(buf + 2*sizeof(uint8_t) + 35*sizeof(char), &s.active_box[i].size, sizeof(uint64_t));
                    memcpy(buf + 2*sizeof(uint8_t) + 35*sizeof(char) + sizeof(uint64_t), "|", sizeof(char));
                    /* n_publishers */
                    memcpy(buf + 2*sizeof(uint8_t) + 36*sizeof(char) + sizeof(uint64_t), &s.active_box[i].pub_activity, sizeof(uint64_t));
                    memcpy(buf + 2*sizeof(uint8_t) + 36*sizeof(char) + 2*sizeof(uint64_t), "|", sizeof(char));
                    /* n_subscribers */
                    memcpy(buf + 2*sizeof(uint8_t) + 37*sizeof(char) + 2*sizeof(uint64_t), &s.active_box[i].num_active_subs, sizeof(uint64_t));               

                    if(write(pipe_man, buf, sizeof(buf)) < 0){
                        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            }       
            default:
                break;
        }
    }
    close(fd);
    return 0;
}