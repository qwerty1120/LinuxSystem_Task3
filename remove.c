#include "sync_header.h"

int main(int argc, char *argv[]){
    Get_Path();
    List_Setting();//초기 세팅
    int pid;
    if(argc<2){//pid 안들어옴
        printf("ERROR : <DAEMON_PID> is not include\n");
        printf("Usage: remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n");
        exit(1);
    }
    if((pid = atoi(argv[1])) == 0){
        fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
        exit(1);
    }
    if(Find_Pid(pid)) {
        if(kill(pid, SIGKILL)){
            fprintf(stderr, "kill error for %d\n", pid);
        }
    }
    else{
        fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
        exit(1);
    }
    char tmpbuf[PATHMAX*2];
    sprintf(tmpbuf, "%s/%d", BACKUPPATH, pid);
    RemoveDirch(tmpbuf);
    sprintf(tmpbuf, "%s/%d.log", BACKUPPATH, pid);
    remove(tmpbuf);
    //monitor.log 에서 지우는 거

}
void Remove_Log(){
    char tmppath[PATHMAX];
    sprintf(tmppath, "%s/tmp", BACKUPPATH);
    int tmpfd;
    if((tmpfd = open(tmppath, O_CREAT|O_TRUNC|O_APPEND|O_RDWR, 0666)) < 0){
        fprintf(stderr, "open error for %s", tmppath);
        exit(1);
    }
    //tmppath에 복사하면서 pid 같은건 생략 한줄 씩 읽기는 Readdelim 쓰면됌
}