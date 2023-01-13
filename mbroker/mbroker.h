#define BOX_NAME_SIZE 32
#define MAX_BOXES 256
#define PIPE_NAME_SIZE 256
#define MX_MSG_SIZE 1024


typedef struct Boxes {
    char* name;
    int num_active_subs;
    int pub_activity; 
} Box;


typedef struct Sessions {
    int num_active_sessions;
    char *active_sessions[256];
    char* pipe_name;
    Box active_box[256];
    int num_active_box;
} Session;


char* format_msg(char* msg, char msg[]);


