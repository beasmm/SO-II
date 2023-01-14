#include "logging.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_MESSAGE_SIZE 1024
#define MAX_BOX_NAME_SIZE 32
#define MAX_CLIENT_NAMED_PIPE_PATH_SIZE 256

int counter = 0;

void sigint_handler(int signum) {
    if (signum == SIGINT) {
        printf("%i messages received\n", counter);
        exit(0);
    }
}

int main(int argc, char **argv) {
    char client_named_pipe_path[MAX_CLIENT_NAMED_PIPE_PATH_SIZE + 1] = {"\0"};
    char box_name[MAX_BOX_NAME_SIZE + 1] = {"\0"};

    struct sigaction act;
    act.sa_handler = &sigint_handler;
    sigaction(SIGINT, &act, NULL);


    if (argc != 4) {
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
        return -1;
    }

    uint8_t code = 2;
    if (strlen(argv[2]) <= MAX_CLIENT_NAMED_PIPE_PATH_SIZE) {
        strcpy(client_named_pipe_path, argv[2]);
    } else {
        WARN("client_named_pipe_path too long");
        return -1;
    }

    if (strlen(argv[3]) <= MAX_BOX_NAME_SIZE) {
        strcpy(box_name, argv[3]);
    } else {
        WARN("box_name too long");
        return -1;
    }

    uint8_t buf[sizeof(uint8_t) + (MAX_CLIENT_NAMED_PIPE_PATH_SIZE+1) * sizeof(char) + (MAX_BOX_NAME_SIZE+1) * sizeof(char)] = {0};
    memcpy(buf, &code, sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t)+ sizeof(char), client_named_pipe_path, strlen(client_named_pipe_path));
    memcpy(buf + sizeof(uint8_t) + (MAX_CLIENT_NAMED_PIPE_PATH_SIZE+1) * sizeof(char), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t) + (MAX_CLIENT_NAMED_PIPE_PATH_SIZE+2) * sizeof(char), argv[3], strlen(argv[3]));

    int tx = open(argv[1], O_WRONLY);
    if (tx < 0) {
        WARN("open failed");
        return -1;
    }

    if (mkfifo(argv[2], 0666) < 0) {
        WARN("mkfifo failed");
        return -1;
    }

    if (write(tx, buf, sizeof(buf)) < 0) {
        WARN("read failed");
        return -1;
    }

    close(tx);
    int rx = open(argv[1], O_RDONLY);
    if (rx < 0) {
        WARN("open failed");
        return -1;
    }

    while (true) {
        char buffer[MAX_MESSAGE_SIZE + 1];
        char message[MAX_CLIENT_NAMED_PIPE_PATH_SIZE + 1];
        if (read(rx, buffer, sizeof(buffer)) < 0) {
            WARN("read failed");
            return -1;
        }
        strcpy(message, buffer + 4);
        fprintf(stdout, "%s\n", message);
    }
    return 0;
}