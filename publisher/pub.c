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

int main(int argc, char **argv) {

    char inp[1024];


    if(argc != 4) {
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        return -1;
    }

    uint8_t code = 1;
    if (strlen(argv[2]) <= 256){
        char const client_named_pipe_path = argv[2]; 
    }
    else{
        WARN("client_named_pipe_path too long");
        return -1;
    }


    if (strlen(argv[3]) <= 32){
        char const box_name = argv[3];
    }
    else{
        WARN("box_name too long");
        return -1;
    }

    uint8_t buf[sizeof(uint8_t) + 256*sizeof(char)+ 32*sizeof(char)] = {0};
    memcpy(buf, &code, sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), client_named_pipe_path, strlen(client_named_pipe_path));
    memcpy(buf + sizeof(uint8_t) + 256*sizeof(char), argv[3], strlen(argv[3]));


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
    
    int rx = open(argv[2], O_WRONLY);
    if (rx < 0) {
        WARN("open failed");
        return -1;
    } 
    
    while(true) {
        scanf("%s", inp);
        uint8_t mesg[sizeof(uint8_t) + 1024*sizeof(char)] = {0};
        memcpy(mesg, 9, sizeof(uint8_t));
        memcpy(mesg + sizeof(uint8_t), inp, strlen(inp));

        if (write(rx, mesg, sizeof(buf)) < 0) {
            WARN("write failed");
            return -1;
        }
        if (inp == EOF){
            break;
        }
    }
    return 0;
}
