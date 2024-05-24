#include "sync_header.h"
int main(int argc, char *argv[]){
    Get_Path();
    if(argc<2){//pid 안들어옴
        int fd;
        if((fd = open(LOGPATH, O_RDONLY)) < 0){
            fprintf(stderr, "open error for %s\n", LOGPATH);
            exit(1);
        }
        while(Read_Delim(fd, BUF, '\n')){
            printf("%s\n", BUF);
        }
    }
    else if(argc == 2){
        pid_t pid;
        if((pid = atoi(argv[1])) == 0){
            fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
            exit(1);
        }
        if(!Find_Pid(pid)) {
            fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
            exit(1);
        }
        else {
            //tree 구조 print
        }
    }
    else{
        fprintf(stderr, "too many argument here\n");
        exit(1);
    }
}