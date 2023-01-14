#include "logging.h"
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

#define MAX_MESSAGE_SIZE 1024
#define MAX_BOX_NAME_SIZE 32
#define MAX_CLIENT_NAMED_PIPE_PATH_SIZE 256


int main(int argc, char **argv) {

    char inp[MAX_MESSAGE_SIZE + 1] = {"\0"};
    char client_named_pipe_path[MAX_CLIENT_NAMED_PIPE_PATH_SIZE + 1] = {"\0"};
    char box_name[MAX_BOX_NAME_SIZE + 1] = {"\0"};

    if(argc != 4) {
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        return -1;
    }

    if (strlen(argv[2]) <= MAX_CLIENT_NAMED_PIPE_PATH_SIZE){
        strcpy(client_named_pipe_path, argv[2]);
    }
    else{
        WARN("client_named_pipe_path too long");
        return -1;
    }

    if (strlen(argv[3]) <= MAX_BOX_NAME_SIZE){
        strcpy(box_name, argv[3]);
    }
    else{
        WARN("box_name too long");
        return -1;
    }

    uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
    memcpy(buf, "1", sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t) + sizeof(char), client_named_pipe_path, strlen(client_named_pipe_path));
    memcpy(buf + sizeof(uint8_t) + 257*sizeof(char), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t) + 258*sizeof(char), argv[3], strlen(argv[3]));


    int tx = open(argv[1], O_WRONLY);
    if (tx < 0) {
        WARN("open failed");
        return -1;
    }
    
    if(mkfifo(argv[2], 0666) < 0) {
        WARN("mkfifo failed");
        return -1;
    }

    if (write(tx, buf, sizeof(buf)) < 0) {
        WARN("write failed");
        return -1;
    }
    
    close(tx);
    int rx = open(argv[2], O_WRONLY);
    if (rx < 0) {
        WARN("open failed");
        return -1;
    } 
    
    while(true) {
        if(scanf("%s", inp) == EOF) {
            break;
        };
        uint8_t mesg[sizeof(uint8_t) + 1025*sizeof(char)] = {0};
        memcpy(mesg, "9", sizeof(uint8_t));
        memcpy(mesg + sizeof(uint8_t), "|", sizeof(char));
        memcpy(mesg + sizeof(uint8_t) + sizeof(char), inp, strlen(inp));

        if (write(rx, mesg, sizeof(mesg)) < 0) {
            WARN("write failed");
            return -1;
        }
        
    }
    close(rx);
    return 0;
}
