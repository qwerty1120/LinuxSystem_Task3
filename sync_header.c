#include "sync_header.h"

#define true 1
#define false 0

#define HASH_MD5  33

#define PATHMAX 4096
#define STRMAX 255

typedef char bool;

int COMMAND_CNT=5;
int OPTION;
pid_t PID;

char EXEPATH[PATHMAX];
char LOGPATH[PATHMAX];
char BACKUPPATH[PATHMAX];
char FILEPATH[PATHMAX];
char inputBuf[PATHMAX*4];//버퍼
char BUF[PATHMAX*2];
char PIDBUF[STRMAX];

char BUF1[PATHMAX*2];

char *COMMAND_SET[] = {
        "add",
        "remove",
        "list",
        "help",
        "exit"
};

struct List *Q;
struct list *PID_LIST;

// 파일 해시하는 함수
int md5(char *target_path, char *hash_result) {
    FILE *fp;
    unsigned char hash[MD5_DIGEST_LENGTH];
    unsigned char buffer[SHRT_MAX];
    int bytes = 0;
    MD5_CTX md5;

    if ((fp = fopen(target_path, "rb")) == NULL) {
        printf("ERROR: fopen error for %s\n", target_path);
        return 1;
    }

    MD5_Init(&md5);

    while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
        MD5_Update(&md5, buffer, bytes);

    MD5_Final(hash, &md5);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(hash_result + (i * 2), "%02x", hash[i]);
    hash_result[HASH_MD5 - 1] = 0;//해시 길이만큼 끊어주기
    fclose(fp);

    return 0;
}

//필요한 repo path들 가져오기
void Get_Path(){
    getcwd(EXEPATH,PATHMAX);//현재 작업 경로

    snprintf(BACKUPPATH, strlen(getenv("HOME")) + 8, "%s/backup", getenv("HOME"));
    snprintf(LOGPATH, strlen(BACKUPPATH)+18, "%s/monitor_list.log", BACKUPPATH);
}

//특정 구분자가 있을 때 까지 읽기
int Read_Delim(int fd, char *buf, char delim){
    char charbuf;
    int i = 0, ret;
    while((ret = read(fd, &charbuf, sizeof(char))) > 0){
        buf[i++] = charbuf;
        if(charbuf == delim){//구분자 만나면
            buf[i-1] = '\0';//널문자 넣어서 끊어주기
            return ret;
        }
    }
    return ret;//파일 끝
}

//한 글자 읽는 데에 사용
int Read_One (int fd){
    char charbuf;
    return read(fd, &charbuf, sizeof(char));
}
//한 줄을 읽어서 리스트에 추가 필요성 결정
int Read_Line(int fd, char *buf){
    int check;
    if ((check = Read_Delim(fd, buf, ' ')) == 0) {//띄어쓰기까지 읽어서
        return 0;
    } else if (check < 0) {
        fprintf(stderr, "read error for %s\n", LOGPATH);//여기 들어오면 못읽은 거
        exit(1);
    }
    OPTION = 0;
    PID = atoi(buf);
    Read_Delim(fd, buf, ' ');
    Read_Delim(fd, buf, '\n');//구분자로 경로 읽어오기
    return 1;
}
//
void MonitorList_Init(){
    PID_LIST = node_Init(PID_LIST);
    int fd;
    int command;

    if((fd=open(LOGPATH, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", LOGPATH);
        exit(1);
    }

    while(Read_Line(fd, BUF) > 0){//스테이징 로그 읽어서
        struct node * new = (struct node *)malloc(sizeof(struct node));
        strcpy(new->path, BUF);
        new->pid = PID;
        new->prev = PID_LIST->tail->prev;
        PID_LIST->tail->prev = new;
        new->next = PID_LIST->tail;
        new->prev->next = new;
    }
    close(fd);
}
//경로 중복 체크
int Check_Path(char * path){
    struct node * curr = PID_LIST->head;
    while(curr->next != PID_LIST->tail){
        curr = curr->next;
        if(!strcmp(curr->path, path)) return 1;
    }
    return 0;
}
//list 초기화
struct list * node_Init(struct list *new){
    new = (struct list*)malloc(sizeof(struct list));
    new->head = (struct node*)malloc(sizeof(struct node));
    new->tail = (struct node*)malloc(sizeof(struct node));
    new->head->next=new->tail;
    new->tail->prev = new->head;
    new->tail->next = NULL;
    return new;
}
//list 초기화 세팅
struct List * List_Init(struct List * Q){
    Q = (struct List*)malloc(sizeof(struct List));
    Q->head = (struct Node*)malloc(sizeof(struct Node));
    Q->tail = (struct Node*)malloc(sizeof(struct Node));
    Q->head->child = (struct Node*)malloc(sizeof(struct Node));
    Q->head->parent = (struct Node*)malloc(sizeof(struct Node));
    Q->tail->next = NULL;
    Q->head->prev = NULL;
    Q->head->child = NULL;
    Q->head->parent = NULL;
    Q->head->next = Q->tail;
    Q->tail->prev = Q->head;
    strcpy(Q->head->realpath, EXEPATH);//루트 노드의 경로는 현재 작업 디렉토리
    Q->head->isdir = true;
    return Q;
}
//해당하는 리스트 끝에 노드 추가
void Insert_Node(struct Node *curr, char *path){
    struct Node * new = (struct Node *)malloc(sizeof(struct Node));
    strcpy(new->realpath, path);//노드에 경로 저장
    if(curr->child != NULL){
        new->parent = curr;
        curr->child->next = new;
        new->prev = curr->child;
        curr->child = new;
    }
    else {
        curr->child = new;
        new->parent = curr;
    }
}
//재귀적으로 디렉토리를 검사하면서 주어진 경로에 속하는 모든 파일을 노드로 삽입하는 함수
void Insert_Recur(struct Node *curr, char *path){
    int cnt;
    struct stat stbuf;
    struct dirent **namelist;
    char buf[PATHMAX];

    if(lstat(path, &stbuf) < 0) {
        fprintf(stderr, "lstat error for %s\n", path);
        exit(1);
    }
    if(S_ISDIR(stbuf.st_mode)) {//디렉토리면
        Insert_Node(curr, path);
        curr->child->isdir=true;
        curr->child->mod_time = stbuf.st_mtim;

        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {//스캔해서
            fprintf(stderr, "ERROR : scandir error for %s\n", path);
            exit(1);
        }
        for (int i = 0; i < cnt; i++) {//안에 있는 것들 검사해서
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
                continue;
            sprintf(buf, "%s/%s", path, namelist[i]->d_name);
            Insert_Recur(curr->child, buf);//다시 Insert_Recur 실행
        }
    }
    else if(S_ISREG(stbuf.st_mode)) {//파일이면
        Insert_Node(curr, path);//노드 추가하고
        curr->child->isdir = false;//디렉토리 아닌 것 저장
        curr->child->mod_time = stbuf.st_mtim;//수정 시간 등록
    }
}
//위의 함수들을 사용해 현재 작업 디렉토리에 대한 트리구조의 리스트를 만드는 함수
void List_Setting(){
    int cnt;
    struct dirent **namelist;
    char buf[PATHMAX*2];

    Q = List_Init(Q);//초기화
    MonitorList_Init();
    if ((cnt = scandir(EXEPATH, &namelist, NULL, alphasort)) == -1) {//현재 작업 디렉토리 스캔
        fprintf(stderr, "ERROR : scandir error for %s\n", EXEPATH);
        exit(1);
    }
    for (int i = 0; i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
        sprintf(buf, "%s/%s", EXEPATH, namelist[i]->d_name);
        Insert_Recur(Q->head, buf);//재귀적 실행
    }
}
//주어진 경로에 해당하는 노드를 찾아 그 위치를 반환하는 함수
struct Node * Find_Node(char *path, struct Node* start){
    struct Node * curr = start;
    while(1){
        if(strcmp(path, curr->realpath) && !strncmp(path, curr->realpath, strlen(curr->realpath))){//같지 않지만 앞부분의 경로가 같다면 같은 부모노드를 가졌을테니
            curr=curr->child;//내려감
        }
        else if(strncmp(path, curr->realpath, strlen(curr->realpath))){//같은 디렉토리에서 파일 이름 비교
            if(curr->prev != NULL){
                curr = curr->prev;
            }
            else {
                Insert_Node(curr->parent, path);
                return curr->parent->child;
            }
        }
        else if(!strcmp(path, curr->realpath)){//아예 같으면 찾음
            return curr;//반환
        }
    }
}
//주어진 리스트 마지막에 노드 삽입
void Insert_File(struct list * file, char * filepath){
    struct node * new = (struct node *)malloc(sizeof(struct node));
    strcpy(new->path, filepath);
    new->prev = file->tail->prev;
    file->tail->prev = new;
    new->next = file->tail;
    new->prev->next = new;
}
//주어진 리스트 첫 번째 노드 삭제
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
        Remove_File(file);
        return 1;
    }
    int check = 0;
    //첫번째 칸이아니라 좀 뒤에 있으면 가운데 있는애들 삭제된거임 todo
    while(curr->next != NULL){
        if(!strcmp(curr->path, filepath)) {check = 1; break;}
        curr = curr->next;
    }
    if(check){//remove 두번 되는거 여기서 확인 필요
        while(strcmp(file->head->next->path, filepath)){
            //write remove : todo
            int fd;
            time_t timer = time(NULL);
            struct tm *t = localtime(&timer);
            char tbuf[80];
            strftime(tbuf, 20,"%F %T", t);
            char buf[PATHMAX*2];
            sprintf(buf, "%s/%d.log", BACKUPPATH, pid);
            if((fd = open(buf, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0){
                exit(0);
            }
            sprintf(buf, "[%s][remove][%s]\n", tbuf, file->head->next->path);//todo 시간 받아서 로그에 쓰고 그걸로 만든 애도 디렉토리에 넣어줘야함 remove이상함 그리고 디렉토리일 때 안돌아감ㅋ
            write(fd, buf, strlen(buf));
            Remove_File(file);
        }
        Remove_File(file);
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
        free(namelist[i]);
    }
    free(namelist);
    strcpy(result, res);
}
//주어진 pid로 백업용 pid 디렉토리 만들기
void Make_Dir(char *filepath, int pid, struct tm *t){
    int fd1, fd2, len;
    struct stat statbuf;
    char tbuf[80];
    strftime(tbuf, 15,"%G%m%d%H%M%S", t);
    char buf[PATHMAX*2];
    sprintf(buf, "%s/%d%s_%s", BACKUPPATH, pid, filepath+strlen(EXEPATH), tbuf);
    for(int j=0;buf[j]!=0;j++){
        if(buf[j]=='/') {
            buf[j] = 0;
            if(access(buf,F_OK)){
                mkdir(buf,0777);
            }
            buf[j]='/';
        }
    }
    if ((fd1 = open(filepath, O_RDONLY)) < 0) {
        exit(1);
    }
    if ((fd2 = open(buf, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        exit(1);
    }
    char buffer[STRMAX];
    while ((len = read(fd1, buffer, sizeof(buffer))) > 0) {//내용 복사
        write(fd2, buffer, len);
    }
    close(fd1);close(fd2);
}
//파일을 monitoring 한다
void Monitoring(char * filepath, struct list* old, struct list* new, int pid, struct timespec mod_t){
    if(access(filepath, F_OK))return;
    Insert_File(new, filepath);
    int fd;
    time_t timer = time(NULL);
    struct tm *t = localtime(&timer);
    char tbuf[80];
    strftime(tbuf, 20,"%F %T", t);
    char buf[PATHMAX*2];
    sprintf(buf, "%s/%d.log", BACKUPPATH, pid);

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
                close(fd);
                Make_Dir(filepath, pid, t);
            }
        }
        //modified
    }
    else{
        //creat
        sprintf(buf, "[%s][create][%s]\n", tbuf, filepath);//todo 시간 받아서 로그에 쓰고 그걸로 만든 애도 디렉토리에 넣어줘야함
        write(fd, buf, strlen(buf));
        close(fd);
        Make_Dir(filepath, pid, t);
    }
    //monitoring
}
//옵션에 따라 경로를 비재귀 or 재귀적으로 모니터링
void Monitor_File(char * filepath, int pid, int opt, struct list* old_file){
    struct Node *curr = Find_Node(filepath, Q->head);
    struct list *q;//이 리스트로 bfs 큐 역할 시킬거임 옵션 D면은 그냥 큐 안넣으면 됌ㅁ!
    q = node_Init(q);
    struct list *new_file;
    new_file = node_Init(new_file);
    Insert_File(q, curr->realpath);
    while(Remove_File(q)){
        if(curr->isdir == true){
            if(curr->child != NULL) {
                curr = curr->child;
                while(1){
                    if(curr->prev != NULL)curr = curr->prev;
                    else break;
                }
                while (1) {
                    if (curr->isdir == true){//todo : .repo 지나치게하기~~
                        if(opt & OPT_R)Insert_File(q, curr->realpath);
                    }
                    else {//if(curr->pid == pid){ 여기 주석 풀면 겹치는거 없이 진행하는프로세스
                        Monitoring(curr->realpath, old_file, new_file, pid, curr->mod_time);
                        //monitoring
                    }
                    if(curr->next != NULL)curr = curr->next;
                    else break;
                }
            }
        }
        else {//if(curr->pid == pid){
            Monitoring(curr->realpath, old_file, new_file, pid, curr->mod_time);
            //monitoring
        }
        if(q->head->next != q->tail) curr = Find_Node(q->head->next->path, Q->head);
    }
    while(old_file->head->next != old_file->tail){
        int fd;
        time_t timer = time(NULL);
        struct tm *t = localtime(&timer);
        char tbuf[80];
        strftime(tbuf, 20,"%F %T", t);
        char buf[PATHMAX*2];
        sprintf(buf, "%s/%d.log", BACKUPPATH, pid);
        if((fd = open(buf, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0){
            exit(0);
        }
        sprintf(buf, "[%s][remove][%s]\n", tbuf, old_file->head->next->path);
        write(fd, buf, strlen(buf));
        Remove_File(old_file);
    }
    struct node *temp = new_file->head->next;
    while (temp != new_file->tail) {
        Insert_File(old_file, temp->path);
        temp = temp->next;
    }//new_file old_file바꿔줘야함 old_file은 비워주고
}
//pid 존재하는 지 체크
int Find_Pid(int pid){
    struct node *curr = PID_LIST->head;
    while(curr->next != PID_LIST->tail){
        curr = curr->next;
        if(curr->pid == pid)return 1;
    }return 0;
}
int daemon_init(int opt, int time, char* filepath){

    pid_t pid;
    int fd,maxfd;
    pid = getpid();
    printf("monitoring started (%s) : %d\n", FILEPATH, pid);
    int n = pid, cnt=0;
    while(n > 0){
        n/=10;
        cnt++;
    }
    //monitor에 적기
    if((fd = open(LOGPATH, O_CREAT|O_WRONLY|O_APPEND, 0666)) < 0){
        fprintf(stderr, "open error for %s\n", LOGPATH);
        exit(1);
    }
    sprintf(BUF, "%d : %s\n", pid, filepath);
    if(write(fd, BUF, strlen(BUF)) < 0){
        fprintf(stderr, "write error for %s\n", LOGPATH);
        exit(1);
    }
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
    List_Setting();
    while(1){
        Monitor_File(filepath, pid, opt, old_file);
        List_Setting();
        sleep(time);
        //monitor function
    }
    return 0;
}
//path remove
int RemoveDirch(char *path){
    struct dirent **namelist;
    struct stat statbuf;
    char *tmpPath = (char *) malloc(sizeof(char) * PATHMAX);
    int cnt,i;

    if (lstat(path, &statbuf) < 0) {
        return 1;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }
        for (i = 0; i < cnt; i++) {// 현재 디렉터리나 상위 디렉터리인 경우 넘어감
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            sprintf(tmpPath, "%s/%s", path, namelist[i]->d_name);
            RemoveDirch(tmpPath);
        }
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }// 디렉터리 내에 더 이상 항목이 없는 경우 디렉터리 삭제
        if(cnt<3)remove(path);
    }
    else remove(path);
    return 0;
}
void Remove_Log(int pid){
    char tmppath[PATHMAX*2];
    sprintf(tmppath, "%s/tmp", BACKUPPATH);
    int tmpfd, fd;
    if((tmpfd = open(tmppath, O_CREAT|O_TRUNC|O_APPEND|O_RDWR, 0666)) < 0){
        fprintf(stderr, "open error for %s", tmppath);
        exit(1);
    }
    if((fd = open(LOGPATH, O_RDONLY, 0666)) < 0){
        fprintf(stderr, "open error for %s\n", LOGPATH);
        exit(1);
    }
    //tmppath에 복사하면서 pid 같은건 생략 한줄 씩 읽기는 Readdelim 쓰면됌
    char buf[PATHMAX];
    int tmppid;
    while(Read_Line(fd, buf)){
        if(PID == pid){
            printf("monitoring ended (%s) : %d\n", buf, pid);
            continue;
        }
        char buf1[PATHMAX*2];
        sprintf(buf1, "%d : %s\n", PID, buf);
        write(tmpfd, buf1, strlen(buf1));
    }
    close(tmpfd);close(fd);
    if((tmpfd = open(tmppath, O_RDONLY, 0666)) < 0){
        fprintf(stderr, "open error for %s", tmppath);
        exit(1);
    }
    if((fd = open(LOGPATH, O_WRONLY|O_TRUNC|O_APPEND, 0666)) < 0){
        fprintf(stderr, "open error for %s\n", LOGPATH);
        exit(1);
    }
    while(Read_Delim(tmpfd, buf, '\n')){
        strcat(buf, "\n");
        write(fd, buf, strlen(buf));
    }
    close(tmpfd);close(fd);
    remove(tmppath);
}
void list_tree(int height, char *isLastDir, int pid) {//list명령어에서 tree 출력하기
    char *treePATH = (char*)malloc(sizeof(char)*PATHMAX);
    struct dirent **namelist;
    int i, count, lastIdx, firstidx = 0;
    struct stat statbuf;

    chdir(isLastDir);
    if ((count = scandir(".", &namelist, NULL, alphasort)) == -1) {
        return;
    }
    for (i = count - 1; i >= 0; i--) {//디렉터리 구성요소 체크
        if (!strcmp(".", namelist[i]->d_name) || !strcmp("..", namelist[i]->d_name)) {
            firstidx = i;
            continue;
        }

        lastIdx = i;
        break;
    }
    char buf[PATHMAX];
    for (i = 0; i < count; i++) {
        if (!strcmp(".", namelist[i]->d_name) || !strcmp("..", namelist[i]->d_name)) {//현재나 위쪽 디렉터리 넘기기
            free(namelist[i]);
            continue;
        }
        if (stat(namelist[i]->d_name, &statbuf) < 0) {
            free(namelist[i]);
            continue;
        }
        strcpy(buf, namelist[i]->d_name);
        if(S_ISREG(statbuf.st_mode)) {
            buf[strlen(namelist[i]->d_name) - 15] = '\0';
            if(i + 1 < count && !strncmp(buf, namelist[i+1]->d_name, strlen(buf)) && strlen(namelist[i]->d_name) == strlen(namelist[i+1]->d_name))continue;
        }//뒤에 있는거랑 이름 같으면 일단 넘기기
        for (int i = 1; i < height; i++) {
            if (isLastDir[i] == 0)   //마지막 아니면 잇기
                printf("│");
            else {
                printf(" ");
            }
            printf("   ");
        }
        if (i != lastIdx) {
            printf("├─ %s", buf); //밑에 자식 잇기
            isLastDir[height] = 0;
        } else {
            if(height)printf("└─ %s", buf);
            else printf("%s/%s", EXEPATH, buf);
            isLastDir[height] = 1;
        }
        if(S_ISREG(statbuf.st_mode)){
            printf("\n");
            sprintf(BUF, "%s/%d.log", BACKUPPATH,pid);

            int fd;
            if((fd = open(BUF, O_RDONLY, 0666)) < 0){
                fprintf(stderr, "open error for %s\n", buf);
                exit(1);
            }
            while(Print_Log(fd, buf, BUF1, pid)) {
                for (int i = 1; i < height + 1; i++) {
                    if (isLastDir[i] == 0)   //마지막 아니면 잇기
                        printf("│");
                    else {
                        printf(" ");
                    }
                    printf("   ");
                }
                printf("└─ %s\n", BUF1);
            }close(fd);
        }
        else if (S_ISDIR(statbuf.st_mode)) {   //디렉토리라면
            if(height)printf("/\n");
            else printf("\n");
            chdir(namelist[i]->d_name); //작업디렉토리 이동
            list_tree(height + 1, isLastDir, pid);  //깊이 1 증가
            chdir("..");  //돌아오기
        }
        free(namelist[i]);
    }
    free(namelist);
}
int Print_Log(int fd,char * name, char * output, int pid){
    char buf[PATHMAX*2],time[STRMAX], command[STRMAX], tmp[PATHMAX], exe[PATHMAX];
    getcwd(exe, PATHMAX);
    char tt[100];
    sprintf(tt, "%d", pid);
    if(strlen(exe) > strlen(BACKUPPATH)+strlen(tt)+2)sprintf(buf, "%s/%s/%s", EXEPATH,exe+strlen(BACKUPPATH)+strlen(tt)+2, name);
    else sprintf(buf, "%s/%s", EXEPATH, name);
    while(Read_One(fd)) {
        Read_Delim(fd, time, ']');
        Read_One(fd);
        Read_Delim(fd, command, ']');
        Read_One(fd);
        Read_Delim(fd, tmp, ']');
        Read_One(fd);
        if (!strcmp(tmp, buf)) {
            sprintf(output, "[%s][%s]", command, time);
            return 1;
        }
    }
    return 0;
}