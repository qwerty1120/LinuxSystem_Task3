#include "sync_header.h"
int main(int argc, char *argv[]){
    Get_Path();
    List_Setting();
    if(argc<2){//pid 안들어옴
        int fd, cnt = 0;
        if((fd = open(LOGPATH, O_RDONLY)) < 0){
            fprintf(stderr, "open error for %s\n", LOGPATH);
            exit(1);
        }
        while(Read_Delim(fd, BUF, '\n')){
            cnt++;
            printf("%s\n", BUF);
        }
        if(!cnt){
            fprintf(stderr, "The daemon process does not exist\n");
            exit(1);
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
            char buf[PATHMAX*2];
            sprintf(buf, "%s/%d", BACKUPPATH, pid);
            printf("%d\n", pid);
            list_tree(0, buf, pid);
            chdir(EXEPATH);
        }
    }
    else{
        fprintf(stderr, "too many argument here\n");
        exit(1);
    }
}