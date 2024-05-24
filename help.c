#include "sync_header.h"
int main(int argc, char *argv[]){
    if(argc == 1){//다른 명령어 안들어온 경우
        printf("Usage:\n");
        printf("\t> add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n");
        printf("\t  -d : add new daemon process of <PATH> if <PATH> is directory\n");
        printf("\t  -r : add new daemon process of <PATH> recursive if <PATH> is directory\n");
        printf("\t  -t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n");
        printf("\t> remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n");
        printf("\t> list [DAEMON_PID] : show daemon process list or dir tree\n");
        printf("\t> help [COMMAND] : show commands for program\n");
        printf("\t> exit : exit program\n");
    }
    else if(argc == 2){//명령어가 들어온 경우
        if(!strcmp(argv[1], COMMAND_SET[0])){
            printf("Usage: add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n");
            printf("\t-d : add new daemon process of <PATH> if <PATH> is directory\n");
            printf("\t-r : add new daemon process of <PATH> recursive if <PATH> is directory\n");
            printf("\t-t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n");
        }
        else if(!strcmp(argv[1], COMMAND_SET[1])){
            printf("\tUsage: remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n");
        }
        else if(!strcmp(argv[1], COMMAND_SET[2])){
            printf("\tUsage: list [DAEMON_PID] : show daemon process list or dir tree\n");
        }
        else if(!strcmp(argv[1], COMMAND_SET[3])){
            printf("\tUsage: help [COMMAND] : show commands for program\n");
        }
        else if(!strcmp(argv[1], COMMAND_SET[4])){
            printf("\tUsage: exit : exit program\n");
        }
        else {
            printf("\"%s\" is wrong command\n", argv[1]);
        }
    }
    else {//인자가 너무 많을 경우
        printf("too many argument\nUsage:\n\thelp: show commands for program\n\thelp [COMMAND] : show commands for program\n");
    }
    exit(0);
}