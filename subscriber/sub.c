#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
        return -1;
    }

    uint8_t code = 2;
    if (strlen(argv[2]) <= 256) {
        char const client_named_pipe_path = argv[2];
    } else {
        WARN("client_named_pipe_path too long");
        return -1;
    }

    if (strlen(argv[3]) <= 32) {
        char const box_name = argv[3];
    } else {
        WARN("box_name too long");
        return -1;
    }

    uint8_t buf[sizeof(uint8_t) + 257 * sizeof(char) + 33 * sizeof(char)] = {0};
    memcpy(buf, &code, sizeof(uint8_t));
    memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t)+ sizeof(char), client_named_pipe_path, strlen(client_named_pipe_path));
    memcpy(buf + sizeof(uint8_t) + 257 * sizeof(char), "|", sizeof(char));
    memcpy(buf + sizeof(uint8_t) + 258 * sizeof(char), argv[3], strlen(argv[3]));

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

    int rx = open(argv[1], O_RDONLY);
    if (rx < 0) {
        WARN("open failed");
        return -1;
    }

    int counter = 0;
    while (true) {
        char buffer[1024];
        char message[1024];
        if (read(rx, buffer, sizeof(buffer)) < 0) {
            WARN("read failed");
            return -1;
        }
        strcpy(message, buffer + 4);
        fprintf(stdout, "%s\n", message);
        counter++;
        signal(SIGINT, SIG_DFL);
    }

    printf("%i\n", counter);
    return 0;
}