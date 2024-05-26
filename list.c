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
        while(Read_Delim(fd, BUF, '\n')){//한줄 씩 읽어서 출력
            cnt++;
            printf("%s\n", BUF);
        }
        if(!cnt){//아무것도 없음
            fprintf(stderr, "The daemon process does not exist\n");
            exit(1);
        }
    }
    else if(argc == 2){
        pid_t pid;
        if((pid = atoi(argv[1])) == 0){//pid 정수인지 확인
            fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
            exit(1);
        }
        if(!Find_Pid(pid)) {//존재 확인
            fprintf(stderr, "ERROR : \"%s\" is wrong pid.\n", argv[1]);
            exit(1);
        }
        else {
            //tree 구조 print
            char buf[PATHMAX*2];
            sprintf(buf, "%s/%d", BACKUPPATH, pid);
            list_tree(0, buf, pid);//트리 구조 출력
            chdir(EXEPATH);//작업 디렉토리 다시 돌아오기
        }
    }
    else{
        fprintf(stderr, "too many argument here\n");
        exit(1);
    }
}