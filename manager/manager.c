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

typedef struct box{
    char box_name[32] = "";
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
} box;

int compare(const void* a, const void* b) {
    struct box* entry1 = (struct box*) a;
    struct box* entry2 = (struct box*) b;
    return strcmp(entry1->box_name, entry2->box_name);
}

void sort_boxes(struct box* list, int size) {
    qsort(list, size, sizeof(struct box), compare);
}



static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}


int main(int argc, char **argv) {
    char error_message[1025];
    char answer[1025];
    char client_named_pipe_path[257] = {"\0"};
    char box_name[33] = {"\0"};

    if(argc < 4) {
        print_usage();
        return -1;
    }

    if(strlen(argv[2]) <= 256) {
        strcpy(client_named_pipe_path, argv[2]); 
    } else {
        WARN("client_named_pipe_path too long");
        return -1;
    }

    if(strlen(argv[4]) <= 32) {
        strcpy(box_name, argv[4]);
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

            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, 3, sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char) client_named_pipe_path, 257*sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 258*sizeof(char), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 259*sizeof(char), box_name, 33*sizeof(char));

            if(write(tx, buf, sizeof(buf)) < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            close(tx);
            
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
            
            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, 5, sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char) client_named_pipe_path, 257*sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 258*sizeof(char), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 259*sizeof(char), box_name, 33*sizeof(char));

            
            if(write(tx, buf, sizeof(buf)) < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            close(tx);
            
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
        case "list":{
            
            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, 7, sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char) client_named_pipe_path, 257*sizeof(char));

            int j = 0;
            uint8_t last = 1;
            box box_list[1024];



            if(write(tx, buf, sizeof(buf)) < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            close(tx);
            
            int rx = open(argv[2], O_RDONLY);
            if (rx == -1) {
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }


            while(last != 0){
                if(read(rx, answer, 1) < 0) {
                    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                char* token = strtok(answer, "|");
                for (int i = 0; token != NULL; i++) {
                    if(i == 1) last = atoi(token);
                    if(i == 2) box_list[j].box_name = token;
                    if(i == 3) box_list[j].box_size = atoi(token);
                    if(i == 4) box_list[j].n_publishers = atoi(token);
                    if(i == 5) box_list[j].n_subscribers = atoi(token);
                    token = strtok(NULL, "|");
                }
                j++;
            }


            if(strcmp(box_list[0].box_name, "") == 0){
                fprintf(stdout, "NO BOXES FOUND\n");                
                break;
            }
            else{
                sort_boxes(box_list, j);
                for(int i = 0; i < j; i++){
                    fprintf(stdout, "%s %zu %zu %zu\n", box_list[i].box_name, box_list[i].box_name,
                        box_list[i].n_publishers, box_list[i].n_subscribers);
                }
            }
            break;
        }
        default:
            print_usage();
            break;
    }
    close(rx);
    return 0;
}
