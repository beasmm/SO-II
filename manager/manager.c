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


static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    char error_message[1024];
    char answer[1034];
    
    if(argc < 4) {
        print_usage();
        return -1;
    }

    if(strlen(argv[2]) <= 256) {
        char const client_named_pipe_path = argv[2];
    } else {
        WARN("client_named_pipe_path too long");
        return -1;
    }

    if(strlen(argv[4]) <= 32) {
        char const box_name = argv[3];
    } else {
        WARN("box_name too long");
        return -1;
    }


    int tx = open(argv[1], O_WRONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if(mkfifo(argv[2], 0666) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }




    switch(argv[3]) {
        case "create":{

            uint8_t buf[sizeof(uint8_t) + 256*sizeof(char)+ 32*sizeof(char)] = {0};
            memcpy(buf, 3, sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), client_named_pipe_path, strlen(client_named_pipe_path));
            memcpy(buf + sizeof(uint8_t) + 256*sizeof(char), argv[3], strlen(argv[3]));

            if(write(tx, buf, sizeof(buf)) < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            int rx = open(argv[2], O_RDONLY);
            if (rx == -1) {
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(read(rx, answer, 1) < 0) {
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(answer[4] == 0){
                fprintf(stdout, "OK\n");
            }
            else{
                for(int i = 8; i < 1024; i++){
                    error_message += answer[i];
                    if(answer[i] == "\0") break;
                }
                fprintf(stdout, "ERROR %s\n", error_message);
            }
            break;
        }
        case "remove":{
            
            uint8_t buf[sizeof(uint8_t) + 256*sizeof(char)+ 32*sizeof(char)] = {0};
            memcpy(buf, 5, sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), client_named_pipe_path, strlen(client_named_pipe_path));
            memcpy(buf + sizeof(uint8_t) + 256*sizeof(char), argv[3], strlen(argv[3]));

            
            if(write(tx, buf, sizeof(buf)) < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            int rx = open(argv[2], O_RDONLY);
            if (rx == -1) {
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(read(rx, answer, 1) < 0) {
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(answer[4] == 0){
                fprintf(stdout, "OK\n");
            }
            else{
                int j = 0
                for(int i = 8 ; i < 1024; i++){
                    if(answer[i] == "\0") break;
                    error_message[j] = answer[i];
                    j++;
                }
                fprintf(stdout, "ERROR %s\n", error_message);
            }
            break;
        }
        case "list":
            write(tx, "list", 4);
            break;
        default:
            print_usage();
            break;
    }
    WARN("unimplemented"); // TODO: implement
    return -1;
}
