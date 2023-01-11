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
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    if(argc < 4) {
        print_usage();
        return -1;
    }

    int tx = open(argv[1], O_RDONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if(mkfifo(argv[2], 0666) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    switch(argv[3]) {
        case "create":
            write(tx, "create", 6);
            break;
        case "remove":
            write(tx, "remove", 6);
            break;
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
