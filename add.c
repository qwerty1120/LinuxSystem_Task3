#include "sync_header.h"
int daemon_init();
int main(int argc, char *argv[]){
    Get_Path();
    Stag_Setting();//초기 세팅
    if(argc<2){//경로 안들어옴
        printf("ERROR : <PATH> is not include\n");
        printf("Usage : add <PATH>: record path to staging area, path will track modification.\n");
        exit(1);
    }
    if(strlen(argv[1])>PATHMAX){//너무 김
        fprintf(stderr, "Input path must not exceed 4,096 bytes.\n");
        exit(1);
    }

    if(realpath(argv[1], FILEPATH)==NULL){//잘못된 경로
        fprintf(stderr, "ERROR : \"%s\" is wrong path.\n", argv[1]);
        exit(1);
    }

    if (strncmp(FILEPATH, EXEPATH, strlen(EXEPATH))
        || !strncmp(FILEPATH, REPOPATH, strlen(REPOPATH)))  {// 사용자 디렉토리 내부의 경로인지 확인
        fprintf(stderr, "ERROR: path must be in user directory\n - \'%s\' is not in user directory.\n", FILEPATH);
        exit(1);
    }

    //option 체크
    int optcnt = 0;
    char option;
    OPTION = 0;
    int optime = 0;
    while((option = getopt(argc - 1, argv + 1, "drt:")) != -1){
        if(option != 'd' && option != 'r' && option != 't'){
            if(optopt=='t') fprintf(stderr, "t option's [PERIOD] Empty\n");
            else fprintf(stderr, "ERROR : unknown option %c\nUsage : add [PATH] [OPTION]\n", optopt);//todo
            return -1;
        }

        optcnt++;
        if (option == 'd') {
            if (OPTION & OPT_D) {
                fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                return -1;
            }
            OPTION |= OPT_D;
        }
        else if (option == 'r') {
            if (OPTION & OPT_R) {
                fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                return -1;
            }
            OPTION |= OPT_R;
        }
        else if (option == 't') {
            if (OPTION & OPT_T) {
                fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                return -1;
            }
            if (optarg == NULL) {
                fprintf(stderr, "ERROR: <PERIOD> is null\n");
                return -1;
            }
            optime = atoi(optarg);//t option 시간
            OPTION |= OPT_T;
            optcnt++;
        }
        if((OPTION & OPT_R)&&(OPTION & OPT_D)) OPTION &= ~OPT_D;
    }
    if (argc - optcnt != 2) {// 옵션 처리 후 남은 인자 확인
        fprintf(stderr, "ERROR: argument error\n");
        return -1;
    }

    //dir인데 옵션 없는 경우 예외 처리
    struct Node * check = Find_Node(FILEPATH, Q->head);

    if(check->isdir == true){
        if((OPTION & (OPT_R|OPT_D)) == false){
            fprintf(stderr, "ERROR: \".%s\" is directory\n", FILEPATH);
            return -1;
        }
    }
    else {
        if((OPTION & (OPT_R|OPT_D)) == true){
            fprintf(stderr, "ERROR: \".%s\" is not a directory\n", FILEPATH);
            return -1;
        }
    }
    //명령어 중복 체크
    if(Check_Status(check, ADD_CMD, OPTION) == 0){
        printf("\".%s\" is already in monitoring.\n", FILEPATH+strlen(EXEPATH));
        exit(1);
    }
    if(daemon_init() < 0){ // todo 함수에 들어갈 argc, argv 만들기
        fprintf(stderr, "init failed\n");
        exit(1);
    }
    exit(0);
}
//todo new process를 위한 파일들 목록 만들기
char ** Monitor_File(struct Node *start, int command, int opt){
    struct Node *curr = start;

    struct list *file;//이 리스트로 bfs 큐 역할 시킬거임 옵션 D면은 그냥 큐 안넣으면 됌ㅁ!
    file = node_Init(file);

    int ret = 0;
    char **argv=(char**)malloc(sizeof(char*)), *argu;
    argv[0] = (char *) malloc(sizeof(char) * (strlen(curr->realpath)+1));//argv만들어서

    if(curr->mode != command){
        ret = 1;
    }
    if(curr->isdir == true){
        if(curr->child != NULL) {
            curr = curr->child;
            while (curr->prev != NULL) {
                if (opt & OPT_R)ret += Check_Status(curr, command, opt);//재귀적으로 들어가서 체크
                else if (curr->isdir == false) {
                    if (curr->mode != command) ret++;
                }
                curr = curr->prev;
            }
        }
    }

}

int daemon_init(){
    pid_t pid;
    int fd,maxfd;
    if((pid = fork()) == 0){
    }
    else if(pid != 0){
        exit(0);}
    pid = getpid();

    if((fd=open(STAGPATH, O_RDWR|O_APPEND)) < 0){
        fprintf(stderr, "open error for %s\n", STAGPATH);
        exit(1);
    }
    int n = pid, cnt=0;
    while(n > 0){
        n/=10;
        cnt++;
    }
    if(OPTION & OPT_D) snprintf(BUF, cnt+strlen(FILEPATH)+8, "ADD %d\"%s\"\n", pid,FILEPATH);//non recur dir
    else snprintf(BUF, cnt+strlen(FILEPATH)+8, "add %d\"%s\"\n", pid,FILEPATH);//regular file이랑 recur dir

    if(write(fd, BUF, strlen(BUF)) < 0){
        fprintf(stderr, "write error for %s\n", STAGPATH);
        exit(1);
    }
    printf("monitoring started (%s) : %d\n", FILEPATH, pid);

    setsid();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    maxfd = getdtablesize();
    for(fd =0; fd < maxfd; fd++) close(fd);
    umask(0);
    chdir("/");
    fd =open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
    int setime;
    if(!time) setime = 1;
    else if(time < 0) {
        fprintf(stderr, "ERROR : time period must be postive int\n");
        exit(0);
    }
//    else setime = time;
//    while(1){
//        for(int i=0;i<argc;i++){
//            //수정확인하는애
//              if(수정시간이 다르면){
//                  수정시간 업데이트해주고 해시해서 다르면 만들어주기
//              }
//        }
//        sleep(setime);
//    }
    return 0;
}
