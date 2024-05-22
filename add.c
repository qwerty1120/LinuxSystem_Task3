#include "sync_header.h"
int daemon_init(int opt, int time, char *filepath);
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
    char filepath[PATHMAX];
    strcpy(filepath, FILEPATH);
    if (strncmp(FILEPATH, EXEPATH, strlen(EXEPATH))
        || !strncmp(FILEPATH, REPOPATH, strlen(REPOPATH)))  {// 사용자 디렉토리 내부의 경로인지 확인
        fprintf(stderr, "ERROR: path must be in user directory\n - \'%s\' is not in user directory.\n", FILEPATH);
        exit(1);
    }
    //option 체크
    int optcnt = 0;
    char option;
    OPTION = 0;
    int optime = 1;
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
    if(daemon_init(OPTION, optime, filepath) < 0){
        fprintf(stderr, "init failed\n");
        exit(1);
    }
    exit(0);
}
void Insert_File(struct list * file, char * filepath){
    struct node * new = (struct node *)malloc(sizeof(struct node));
    strcpy(new->path, filepath);
    new->prev = file->tail->prev;
    file->tail->prev = new;
    new->next = file->tail;
    new->prev->next = new;
}
int Remove_File(struct list * file){
    if(file->head->next == file->tail){
        return 0;//
    }
    file->head->next = file->head->next->next;
    file->head->next->prev = file->head;
    return 1;
}
//리스트에서 파일 유무 확인용
int isFile_Exist(char * filepath, struct list * file, int pid){
    struct node * curr = file->head->next;
    if(!strcmp(curr->path, filepath)){
        curr->next->prev = curr->prev;
        curr->prev->next = curr->next;
        free(curr);
        return 1;
    }
    int check = 0;
    //첫번째 칸이아니라 좀 뒤에 있으면 가운데 있는애들 삭제된거임 todo
    while(curr->next != NULL){
        if(!strcmp(curr->path, filepath)) {check = 1; break;}
        curr = curr->next;
    }
    if(check){
        curr = file->head->next;
        while(strcmp(curr->path, filepath)){
            //write remove : todo
            curr->next->prev = curr->prev;
            curr->prev->next = curr->next;
            free(curr);
            int fd;
            time_t timer = time(NULL);
            struct tm *t = localtime(&timer);
            char tbuf[80];
            strftime(tbuf, 20,"%F %T", t);
            char buf[PATHMAX*2];
            sprintf(buf, "%s/%d.log", LOGPATH, pid);
            if((fd = open(buf, O_WRONLY|O_APPEND, 0666)) < 0){
                exit(0);
            }
            sprintf(buf, "[%s][remove][%s]\n", tbuf, filepath);//todo 시간 받아서 로그에 쓰고 그걸로 만든 애도 디렉토리에 넣어줘야함 remove이상함 그리고 디렉토리일 때 안돌아감ㅋ
            write(fd, buf, strlen(buf));
        }
        return 1;
    }
    return 0;
}
//디렉 스캔해서 최신 백업 버전 찾아오기
void Get_Backuppath(char * path, int pid, char * result){
    char dirbuf[PATHMAX*2];
    char buf[PATHMAX*2], filename[STRMAX], res[PATHMAX*3];
    sprintf(buf, "%s/%d%s", BACKUPPATH, pid, path+strlen(EXEPATH));
    strcpy(dirbuf, buf);
    int cnt;
    for(int i=0;i<strlen(dirbuf);i++){
        if(dirbuf[i] == '/')cnt = i;
    }dirbuf[cnt] = 0;
    sprintf(filename, "%s", buf+cnt+1);
    struct dirent **namelist;
    if ((cnt = scandir(dirbuf, &namelist, NULL, alphasort)) == -1) {
        exit(1);
    }
    for (int i = 0; i < cnt; i++) {// 디렉터리 내의 모든 파일 및 디렉터리에 대해 재귀적으로 복구 수행
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
        if(!strncmp(namelist[i]->d_name, filename, strlen(filename))){
            sprintf(res,"%s/%s", dirbuf, namelist[i]->d_name);//앞부분 공통인 디렉토리가 있을수도? todo
        }
    }
    strcpy(result, res);
}
//monitoring 함수 따로 만들자
void Monitoring(char * filepath, struct list* old, struct list* new, int pid, struct timespec mod_t){
    Insert_File(new, filepath);
    int fd;
    time_t timer = time(NULL);
    struct tm *t = localtime(&timer);
    char tbuf[80];
    strftime(tbuf, 20,"%F %T", t);
    char buf[PATHMAX*2];
    sprintf(buf, "%s/%d/%d.log", BACKUPPATH, pid, pid);

    if((fd = open(buf, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0){
        exit(0);
    }
    if(isFile_Exist(filepath, old, pid)){
        struct stat statbuf;
        if(stat(filepath, &statbuf) < 0){
            exit(1);
        }
        if(statbuf.st_mtim.tv_nsec != mod_t.tv_nsec){//이게 잘안되면 최신 파일의 생성 시간을 보자 todo
            char buf[PATHMAX*2];
            Get_Backuppath(filepath, pid, buf);
            md5(buf, BUF);
            md5(filepath, BUF1);
            if(strcmp(BUF,BUF1)){
                sprintf(buf, "[%s][modify][%s]\n", tbuf, filepath);//todo 시간 받아서 로그에 쓰고 그걸로 만든 애도 디렉토리에 넣어줘야함
                write(fd, buf, strlen(buf));
            }
        }
        //modified
    }
    else{
        //creat
        sprintf(buf, "[%s][creat][%s]\n", tbuf, filepath);//todo 시간 받아서 로그에 쓰고 그걸로 만든 애도 디렉토리에 넣어줘야함
        write(fd, buf, strlen(buf));
    }
    close(fd);
    //monitoring
}
//todo new process를 위한 파일들 목록 만들기
void Monitor_File(char * filepath, int pid, int opt, struct list* old_file, struct list * new_file){
    struct Node *curr = Find_Node(filepath, Q->head);
    struct list *q;//이 리스트로 bfs 큐 역할 시킬거임 옵션 D면은 그냥 큐 안넣으면 됌ㅁ!
    q = node_Init(q);
    Insert_File(q, curr->realpath);
    while(Remove_File(q)){
        if(curr->isdir == true){
            if(curr->child != NULL) {
                curr = curr->child;
                while (curr->prev != NULL) {
                    if (curr->isdir == true && (opt & OPT_R)){
                        Insert_File(q, curr->realpath);
                    }
                    else if(curr->isdir == false && curr->pid == pid){
                        Monitoring(curr->realpath, old_file, new_file, pid, curr->mod_time);
                        //monitoring
                    }
                    curr = curr->prev;
                }
            }
        }
        else if(curr->pid == pid){
            Monitoring(curr->realpath, old_file, new_file, pid, curr->mod_time);
            //monitoring
        }
        struct list * tmp = old_file;
        old_file->head = new_file->head;
        old_file->tail = new_file->tail;
        new_file->head = tmp->head;
        new_file->tail = tmp->tail;
        //new_file old_file바꿔줘야함 old_file은 비워주고
    }
}
int daemon_init(int opt, int time, char* filepath){

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
    sprintf(BUF1, "%s/%d", BACKUPPATH, pid);
    if (mkdir(BUF1, 0777) < 0 && errno != EEXIST) {
        perror("mkdir error");
        exit(1);
    }
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
    if(time < 0) {
        fprintf(stderr, "ERROR : time period must be postive int\n");
        exit(0);
    }
    //monitoring
    struct list *old_file;//새로 생긴 애 체크용
    old_file = node_Init(old_file);
    struct list *new_file;//최신 버전 파일 목록
    new_file = node_Init(new_file);
    Stag_Setting();
    while(1){
        Monitor_File(filepath, pid, opt, old_file,new_file);
        Stag_Setting();
        sleep(time);
        //monitor function
    }
    return 0;
}