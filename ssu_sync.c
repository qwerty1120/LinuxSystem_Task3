#include "sync_header.h"

void Init();
int Command_Check(char *command);
int Input_Line();
int Prompt();

int main(){//warning not exist argc and argv
    Init();
    while(Prompt());//프롬프트 실행
}
//명령어 구분
int Command_Check(char *command){
    for(int i=0;i<COMMAND_CNT;i++){
        if(!strcmp(command,COMMAND_SET[i])) return i;
    }
    return -1;
}
//프로세스 실행
int Prompt(char *path){
    printf("20232898> ");

    int cnt;
    if(!(cnt = Input_Line())){
        return 1;
    }
    pid_t pid;
    char *command=strtok(inputBuf," ");//입력받은 거 짤라서 명령어로 저장

    if(Command_Check(command)<0){//명령어인지 확인
        printf("Usage:\n");
        printf("\t> add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n");
        printf("\t  -d : add new daemon process of <PATH> if <PATH> is directory\n");
        printf("\t  -r : add new daemon process of <PATH> recursive if <PATH> is directory\n");
        printf("\t  -t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n");
        printf("\t> remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n");
        printf("\t> list [DAEMON_PID] : show daemon process list or dir tree\n");
        printf("\t> help [COMMAND] : show commands for program\n");
        printf("\t> exit : exit program\n");
        return -1;
    }

    if(!strcmp(COMMAND_SET[4],command)){//exit이면 종료
        return 0;
    }

    char **argv=(char**)malloc(sizeof(char*)*(cnt+1)), *argu;
    argv[0] = (char *) malloc(sizeof(char) * (strlen(command)+1));//argv만들어서

    strcpy(argv[0], command);

    for(int i=1;(argu = strtok(NULL, " ")) != NULL;i++) {
        argv[i] = (char *) malloc(sizeof(char) * (strlen(argu)+1));
        strcpy(argv[i], argu);
    }
    argv[cnt]=NULL;

    if((pid=fork())==0){
        sprintf(BUF, "./%s", argv[0]);
        BUF[strlen(argv[0])+2]=0;
        execv(BUF,argv);//실행할 때 전달
        return 0;
    }
    else if(pid>0){
        wait(NULL);//프로세스 끝날 때까지 기다리기
    }
    else{
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    for(int i=0;i<cnt;i++){
        free(argv[i]);
    }
    free(argv);
    return 1;
}
//라인을 입력 받아서 인자 개수를 리턴하는 함수
int Input_Line(){
    int cnt=0;
    for(int i=0;;i++){
        inputBuf[i]=getchar();
        if(inputBuf[i]==' ')cnt++;
        if(inputBuf[i]=='\n') {
            cnt++;
            inputBuf[i]='\0';
            break;
        }
    }
    if(inputBuf[0] == 0) return 0;//개행만 입력됐을 경우
    return cnt;
}
//세팅
void Init(){
    int fd;
    umask(0000);
    getcwd(EXEPATH,PATHMAX);
    snprintf(BACKUPPATH, strlen(getenv("HOME")) + 8, "%s/backup", getenv("HOME"));
    snprintf(LOGPATH, strlen(BACKUPPATH)+18, "%s/monitor_list.log", BACKUPPATH);

    if (access(BACKUPPATH, F_OK))// 백업 디렉토리가 존재하지 않을 경우 생성
        mkdir(BACKUPPATH, 0777);

    if ((fd = open(LOGPATH, O_RDWR | O_CREAT, 0666)) < 0) {// 백업 로그 파일 열기
        fprintf(stderr, "open error for %s\n", LOGPATH);
        exit(1);
    }
    close(fd);
}