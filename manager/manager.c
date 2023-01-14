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
#include <stdint.h>

typedef struct boxes{
    char box_name[32];
    size_t box_size;
    size_t n_publishers;
    size_t n_subscribers;
} box;


int compare_box_names(const void *a, const void *b) {
    box *box1 = (box *)a;
    box *box2 = (box *)b;
    return strcmp(box1->box_name, box2->box_name);
}

void sort_boxes(box list[], size_t n) {
    qsort(list, n, sizeof(box), compare_box_names);
}


static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}


int main(int argc, char **argv) {
    char client_named_pipe_path[257] = {"\0"};
    char box_name[33] = {"\0"};
    char answer[1027+sizeof(uint32_t)+sizeof(uint8_t)] = {"\0"};
    int operation=0;

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

    if(strcmp(argv[3], "create")== 0){
        operation = 3;
    }else if(strcmp(argv[3], "remove")== 0){
        operation = 5;
    }else if(strcmp(argv[3], "list")== 0){
        operation = 7;
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

    switch(operation) {
        case 3:{

            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, "3", sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char), client_named_pipe_path, 256*sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 257*sizeof(char), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 258*sizeof(char), box_name, 32*sizeof(char));


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
                char error_message[1024] = {"\0"};
                memcpy(error_message, answer + 5, 1024);
                fprintf(stdout, "ERROR %s\n", error_message);
            }
            close(rx);
            break;
        }
        case 5:{

            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, "5", sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char), client_named_pipe_path, 256*sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 257*sizeof(char), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t) + 258*sizeof(char), box_name, 32*sizeof(char));

            
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
                char error_message[1024] = {"\0"};
                memcpy(error_message, answer + 5, 1024);
                fprintf(stdout, "ERROR %s\n", error_message);
            }
            close(rx);
            break;
        }
        case 7:{
            uint8_t buf[sizeof(uint8_t) + 257*sizeof(char)+ 33*sizeof(char)] = {0};
            memcpy(buf, "7", sizeof(uint8_t));
            memcpy(buf + sizeof(uint8_t), "|", sizeof(char));
            memcpy(buf + sizeof(uint8_t)+ sizeof(char), client_named_pipe_path, 257*sizeof(char));

            int num_boxes = 0;
            int last = 1;
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
                    if(i == 1) last = (size_t) atoi(token);
                    if(i == 2) strcpy(box_list[num_boxes].box_name, token);
                    if(i == 3) box_list[num_boxes].box_size =(size_t) atoi(token);
                    if(i == 4) box_list[num_boxes].n_publishers =(size_t) atoi(token);
                    if(i == 5) box_list[num_boxes].n_subscribers =(size_t) atoi(token);
                    token = strtok(NULL, "|");
                }
                num_boxes++;
            }


            if(strcmp(box_list[0].box_name, "") == 0){
                fprintf(stdout, "NO BOXES FOUND\n");                
                break;
            }
            else{
                sort_boxes(box_list, (size_t) num_boxes);
                for(int i = 0; i < num_boxes; i++){
                    fprintf(stdout, "%s %zu %zu %zu\n", box_list[i].box_name, box_list[i].box_size,
                        box_list[i].n_publishers, box_list[i].n_subscribers);
                }
            }
            close(rx);
            break;
        }
        default:
            print_usage();
            break;
    }
    return 0;
}
