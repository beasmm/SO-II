#define BOX_NAME_SIZE 32
#define MAX_BOXES 256
#define PIPE_NAME_SIZE 256
#define MAX_MSG_SIZE 1024

// Data structures
typedef struct Boxes {
    char* name;
    int num_active_subs;
    int pub_activity; 
} Box;

typedef char pipename_t[PIPE_NAME_SIZE];

typedef struct Sessions {
    int num_active_sessions;
    pipename_t* active_sessions;
    char* pipe_name;
    Box active_box[MAX_BOXES];
    int num_active_box;
} Session;