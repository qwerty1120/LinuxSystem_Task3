#include "sync_header.h"

#define true 1
#define false 0

#define HASH_MD5  33

#define UNTRACKED 0
#define ADD_CMD 1
#define REM_CMD 2

#define STAG_MOD 1
#define CMT_MOD 2
#define LOG_MOD 3

#define PATHMAX 4096
#define STRMAX 255

#define max(i, j) ((i) > (j) ? (i) : (j))

typedef char bool;

int COMMAND_CNT=8;
int PLUS;
int MINUS;
int MODIF;
int FCNT;
int OPTION;
int PID;

char EXEPATH[PATHMAX];
char REPOPATH[PATHMAX];
char COMMITPATH[PATHMAX];
char LOGPATH[PATHMAX];
char BACKUPPATH[PATHMAX];
char STAGPATH[PATHMAX];
char FILEPATH[PATHMAX];
char inputBuf[PATHMAX*4];//버퍼
char BUF[PATHMAX*2];
char PIDBUF[STRMAX];

char BUF1[PATHMAX*2];

char *COMMAND_SET[] = {//명령어 셋
        "add",
        "remove",
        "status",
        "commit",
        "revert",
        "log",
        "help",
        "exit"
};

struct List *Q;
struct list *NEW;
struct list *MDF;
struct list *REM;

struct list *UNT;
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
    strcpy(REPOPATH,EXEPATH);
    strcat(REPOPATH,"/.repo");//레포 디렉 경로

    snprintf(COMMITPATH, strlen(REPOPATH)+13, "%s/.commit.log", REPOPATH);//커밋 로그 경로
    snprintf(STAGPATH, strlen(REPOPATH)+14, "%s/.staging.log", REPOPATH);//스테이징 로그 경로
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
char Read_One (int fd){
    char charbuf;
    read(fd, &charbuf, sizeof(char));
    return charbuf;
}
//한 줄을 읽어서 리스트에 추가 필요성 결정
int Read_Line(int fd, char *buf, int mode){
    int ret = -1;
    int check;
    if(mode == STAG_MOD) {//스테이징 구역 관리할 때
        if ((check = Read_Delim(fd, buf, ' ')) == 0) {//띄어쓰기까지 읽어서
            return 0;
        } else if (check < 0) {
            fprintf(stderr, "read error for %s\n", STAGPATH);//여기 들어오면 못읽은 거
            exit(1);
        }
        OPTION = 0;
        if (!strcmp(buf, COMMAND_SET[0])) {//명령어에 따른 값 할당
            ret = ADD_CMD;
            OPTION = OPT_R;
        }else if(!strcmp(buf, "ADD")){
            ret = ADD_CMD;
            OPTION = OPT_D;
        } else if (!strcmp(buf, COMMAND_SET[1])) {
            ret = REM_CMD;
        }
        Read_Delim(fd, PIDBUF, '\"');
        Read_Delim(fd, buf, '\"');//구분자로 경로 읽어오기
        Read_One(fd);
    }
    else if(mode == CMT_MOD || mode == LOG_MOD){
        if ((check = Read_Delim(fd, buf, ' ')) == 0) {
            return 0;
        } else if (check < 0) {
            fprintf(stderr, "read error for %s\n", COMMITPATH);
            exit(1);
        }
        ret = 1;
        Read_One(fd);

        Read_Delim(fd, buf, '\"');//commit name
        Read_One(fd);
        Read_Delim(fd, inputBuf, ' ');
        Read_Delim(fd, inputBuf, ':');//command
        if(mode == LOG_MOD) strcpy(BUF1, inputBuf);
        Read_Delim(fd, inputBuf, '\"');
        Read_Delim(fd, inputBuf, '\"');//realpath
        Read_One(fd);
    }
    return ret;
}
//문장 개수 세기
int countLines(int fd){
    lseek(fd, 0, SEEK_SET);//처음부터
    char charbuf;
    int ret = 0;
    while(read(fd, &charbuf, sizeof(char)) > 0){//0이면 파일 끝
        ret += charbuf == '\n';//개행만나면 줄 카운트 늘려주기
    }
    int a = lseek(fd, 0, SEEK_SET);//offset위치 받아놓기
    return ret;
}

void getOffsets(int fd, int *arr, int N){
    lseek(fd, 0, SEEK_SET);//offset초기화

    char charbuf;
    int i = 1;
    while(read(fd, &charbuf, sizeof(char)) > 0){
        if(charbuf == '\n') arr[i++] = lseek(fd, 0, SEEK_CUR) - 1;
    }

    lseek(fd, 0, SEEK_SET);
}

void LineByLine(int **dp, int i, char * str1, int fd, int *arr, int M){
    lseek(fd, 0, SEEK_SET);//처음부터 읽기

    for(int j = 1; j <= M; j++){
        dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
        int n = arr[j] - arr[j-1];
        char *str2 = malloc(sizeof(char) * n);
        lseek(fd, arr[j-1]+1, SEEK_SET);
        read(fd, str2, n-1); str2[n-1] = 0;
        if(!strcmp(str1, str2)) dp[i][j] = max(dp[i-1][j-1] + 1, dp[i][j]);
        free(str2);
    }//문장끼리 비교
    lseek(fd, 0, SEEK_SET);//오프셋다시 처음위치로
}


void doLCS(int **dp, int *arr1, int *arr2, int fd1, int fd2, int N, int M){
    lseek(fd1, 0, SEEK_SET);

    for(int i = 1; i <= N; i++){
        int n = arr1[i] - arr1[i-1];
        char *str1 = malloc(sizeof(char) * n);
        lseek(fd1, arr1[i-1]+1, SEEK_SET);
        read(fd1, str1, n-1); str1[n-1] = 0;
        LineByLine(dp, i, str1, fd2, arr2, M);
        free(str1);
    }//LCS이용해서 수정안된 문장 수 찾기

    lseek(fd1, 0, SEEK_SET);
}

int Count_Line(char *fname1, char *fname2){
    int fd1 , fd2;
    if((fd1 = open(fname1, O_RDONLY)) < 0){//파일 읽기 전용 오픈
        fprintf(stderr, "open error for %s\n", fname1);
        exit(1);
    }
    if((fd2 = open(fname2, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", fname2);
        exit(1);
    }

    int cnt1, cnt2;
    cnt1 = countLines(fd1);
    cnt2 = countLines(fd2);
    MINUS += cnt1;//파일 문장개수 넣어서 활용가능하게끔
    PLUS += cnt2;
    int **dp = malloc(sizeof(int *) * (cnt1+1));
    for(int i = 0; i <= cnt1; i++){
        dp[i] = malloc(sizeof(int) * (cnt2+1));
        for(int j = 0; j <= cnt2; j++) dp[i][j] = 0;
    }

    int *offs1, *offs2;
    offs1 = malloc(sizeof(int) * (cnt1+1));
    offs2 = malloc(sizeof(int) * (cnt2+1));
    offs1[0] = offs2[0] = -1;

    getOffsets(fd1, offs1, cnt1);
    getOffsets(fd2, offs2, cnt2);

    doLCS(dp, offs1, offs2, fd1, fd2, cnt1, cnt2);//LCS 진행
    int N = cnt1, M = cnt2;
    return dp[cnt1][cnt2];// LCS 개수 반환
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
//status에 필요한 list 초기화
void Status_Init(){
    NEW = node_Init(NEW);
    MDF = node_Init(MDF);
    REM = node_Init(REM);
    UNT = node_Init(UNT);
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
    Q->head->status = true;
    return Q;
}
//해당하는 리스트 끝에 노드 추가
void Insert_Node(struct Node *curr, char *path){
    struct Node * new = (struct Node *)malloc(sizeof(struct Node));
    strcpy(new->realpath, path);//노드에 경로 저장
    new->mode = UNTRACKED;//처음 세팅할 때는 untracked
    new->pid = 0;
    new->time = 1;
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
//command를 입력받아 주어진 노드와 그 노드에 속하는 노드들의 command를 변경
int Cmd_File_Switch(int command, struct Node *start){
    struct Node * curr = start;
    if(curr->mode == command){//같은 상태 변경할 필요없음
        return 0;
    }curr->mode = command;//모드 변경

    if(command == ADD_CMD){
        curr->pid = atoi(PIDBUF);
        while(curr->parent != NULL) {//add 위의 디렉토리가 remove라면 탐색할 때 못 볼 수도 있기 때문에
            curr->status = true;//status를 이용
            curr = curr->parent;//타고 올라가면서 다 변동
        }
    }
    else if(command == REM_CMD){//remove는 해당 노드만 꺼도 됌
        curr->pid = 0;
        curr->mode = REM_CMD;
        curr->status = false;
    }
    return 1;
}
//재귀적으로 디렉토리를 탐색하면서 command 변경
int Cmd_Recur_Switch(int command, struct Node *start, int opt){
    struct Node * curr = start;
    int ret;//ret이 최종적으로 0이면 cmd가 변경된 게 없다는 뜻 그러면 already...

    ret = Cmd_File_Switch(command, curr);

    if(curr->isdir == true){//디렉토리면
        curr=curr->child;
        while(curr != NULL){
            if(curr->isdir == true && (opt & OPT_R)) ret += Cmd_Recur_Switch(command, curr, opt);//재귀적으로 수행
            else if(curr->isdir == false)ret += Cmd_File_Switch(command, curr);//파일이면 이쪽
            if(curr->prev != NULL) curr = curr->prev;
            else break;
        }
    }
    return ret;
}
//staging list 세팅
void Stag_Setting(){
    int fd;
    int command;

    List_Setting();

    if((fd=open(STAGPATH, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", STAGPATH);
        exit(1);
    }

    while((command = Read_Line(fd, BUF, STAG_MOD)) > 0){//스테이징 로그 읽어서
        if(Cmd_Recur_Switch(command, Find_Node(BUF, Q->head), OPTION) < 0){//모드 변경 해주기
            printf("staging log error\n");
            exit(1);
        }
    }

    if((fd=open(COMMITPATH, O_RDONLY)) < 0){//커밋 로그 열어서
        fprintf(stderr, "open error for %s\n", COMMITPATH);
        exit(1);
    }

    while((command = Read_Line(fd, BUF, CMT_MOD)) > 0){// 커밋 모드로 설정함
        strcpy(Find_Node(inputBuf, Q->head)->backupname, BUF);//
    }
}
//add remove child와 parent 관계에 따른 예외 처리용
int Check_Status(struct Node *start, int command, int opt){
    int ret = 0;
    struct Node *curr = start;

    if(curr->mode != command){
        return 1;
    }
    if(curr->isdir == true){
        if(curr->child == NULL) return 0;
        curr = curr->child;
        while(curr->prev != NULL){
            if(opt & OPT_R)ret += Check_Status(curr, command, opt);//재귀적으로 들어가서 체크
            else if(curr->isdir == false){
                if(curr->mode != command) return 1;
            }
            curr = curr->prev;
        }
    }
    else {
        return 0;
    }
    return ret;
}
//commit path로 변화
char * Commit_Path(char * name, char * path){
    sprintf(BUF, "%s/%s%s", REPOPATH, name, path+strlen(EXEPATH));
    return BUF;
}

//커밋 파일만들고 내용 복사
void File_Commit(char *realpath, char * commitpath){
    struct stat statbuf;
    int fd1, fd2;
    int len;
    char buf[PATHMAX*4];

    if((fd1=open(realpath,O_RDONLY))<0){
        fprintf(stderr, "commit error for %s\n", realpath);
    }
    if((fd2=open(commitpath,O_CREAT|O_RDWR, 0777))<0){//커밋할 파일 열기
        fprintf(stderr, "commit error for %s\n", commitpath);
    }
    if (fstat(fd1, &statbuf) < 0) {//stat 가져와서
        fprintf(stderr, "stat error for %s\n", realpath);
        exit(1);
    }
    while ((len = read(fd1, buf, statbuf.st_size)) > 0) {//길이만큼 내용 복사
        if(write(fd2, buf, len) < 0){
            fprintf(stderr, "write error for %s\n", commitpath);
            exit(1);
        }
    }
}
//주어진 이름을 사용해 노드를 탐색하며 스테이징 구역에 있는 파일들을 커밋 경로로 백업
void Make_Commit(struct Node *start, char *name){
    struct Node * curr = start;
    if(curr->isdir == true && (curr->status == true || curr->mode == ADD_CMD)){//디렉토리면
        if(curr->child != NULL) {
            curr = curr->child;
            while (1) {//재귀적으로
                Make_Commit(curr, name);
                if (curr->prev != NULL) {
                    curr = curr->prev;
                } else break;
            }
        }
    }
    else if(curr->isdir == false && curr->mode == ADD_CMD){//스테이징 구역에 있는 파일
        if(!access(curr->realpath, F_OK)) {//접근 가능하면
            strcpy(BUF1, Commit_Path(name, curr->realpath));//커밋 경로만들어서
            for (size_t j = strlen(REPOPATH); BUF1[j] != 0; j++) {// 디렉터리가 없으면 생성
                if (BUF1[j] == '/') {
                    BUF1[j] = 0;
                    if (access(BUF1, F_OK)) {
                        mkdir(BUF1, 0777);
                    }
                    BUF1[j] = '/';
                }
            }
            File_Commit(curr->realpath, BUF1);//파일 만들어서 커밋
        }
    }
}
//파일의 최신 백업 파일로 가서 수정됐는지 삭제되었는지 새로운 파일인지 체크해서 정보 입력
void File_Status(struct Node *file){
    struct node * new = (struct node*)malloc(sizeof(struct node));
    strcpy(new->path, file->realpath);//경로 저장
    if(file->mode == UNTRACKED){//untracked인 애들 UNT에 연결
        if(strncmp(file->realpath, REPOPATH, strlen(REPOPATH))) {
            UNT->tail->next = new;
            new->prev = UNT->tail;
            UNT->tail = UNT->tail->next;
            UNT->tail->next = NULL;
        }
    }
    else if(file->mode == ADD_CMD){
        if(access(file->realpath, F_OK) && strcmp(file->backupname, "")) {//존재하지 않는 파일이고 커밋기록이 있다면
            if(!access(Commit_Path(file->backupname, file->realpath), F_OK)) {//커밋 버전을 들어가서
                int fd;
                if((fd = open(Commit_Path(file->backupname, file->realpath), O_RDONLY)) < 0){
                    fprintf(stderr, "open error for %s\n", Commit_Path(file->backupname, file->realpath));
                    exit(1);
                }
                MINUS += countLines(fd);//줄 개수 카운트해서 지워진 것에 추가
                close(fd);
                FCNT++;
                REM->tail->next = new;
                new->prev = REM->tail;
                REM->tail = REM->tail->next;
                REM->tail->next = NULL;
            }
        }
        else if(!access(file->realpath, F_OK)){
            if(!strcmp(file->backupname, "")){//커밋 기록이 없다
                int fd;
                if((fd = open(file->realpath, O_RDONLY)) < 0){
                    fprintf(stderr, "open error for %s\n", file->realpath);
                    exit(1);
                }
                PLUS += countLines(fd);//줄 개수 세서 추가
                close(fd);
                FCNT++;//파일 개수 카운트
                NEW->tail->next = new;
                new->prev = NEW->tail;
                NEW->tail = NEW->tail->next;
                NEW->tail->next = NULL;
            }
            else if(!access(file->realpath, F_OK)){
                MODIF += Count_Line(Commit_Path(file->backupname, file->realpath), file->realpath);//공통 줄 개수 카운트
                md5(Commit_Path(file->backupname, file->realpath), inputBuf);
                md5(file->realpath, BUF1);
                if(strncmp(BUF1, inputBuf, MD5_DIGEST_LENGTH) != 0){//내용이 다르면
                    FCNT++;
                    MDF->tail->next = new;
                    new->prev = MDF->tail;
                    MDF->tail = MDF->tail->next;
                    MDF->tail->next = NULL;
                }
            }
        }
    }
}
//BFS로 노드를 탐색하면서 파일의 status 정보를 업데이트
void Status_Check(struct Node *start){
    struct Node * curr = start;
    if(curr->isdir == true){//디렉토리면
        if(curr->child != NULL) {
            curr = curr->child;
            while (1) {
                if(curr->isdir == false && curr->mode != REM_CMD){
                    File_Status(curr);//상태확인
                }
                if (curr->prev != NULL) {
                    curr = curr->prev;
                } else break;
            }
            curr = start->child;
            while(1){
                if(curr->isdir == true){
                    Status_Check(curr);
                }
                if (curr->prev != NULL) {
                    curr = curr->prev;
                } else break;
            }
        }
    }
}
//노드의 status 정보를 가져와 각 정보마다 분류해 출력
void Print_Status(){
    int check = 0;
    Status_Init();
    Status_Check(Q->head);
    if(NEW->head->next != NULL || MDF->head->next != NULL || REM->head->next != NULL){
        printf("Changes to be committed: \n");
        if(MDF->head->next != NULL){//수정된 애들
            struct node *curr = MDF->head->next;
            printf("  Modified: \n");
            while(1){
                printf("\t\".%s\"\n", curr->path + strlen(EXEPATH));
                if(curr->next == NULL) break;
                curr=curr->next;
            }
            free(curr);
            printf("\n");
        }
        if(REM->head->next != NULL){//삭제된 애들
            struct node *curr = REM->head->next;
            printf("  Removed: \n");
            while(1){
                printf("\t\".%s\"\n", curr->path + strlen(EXEPATH));
                if(curr->next == NULL) break;
                curr=curr->next;
            }
            free(curr);
            printf("\n");
        }
        if(NEW->head->next != NULL){//새로 추가된 애들
            struct node *curr = NEW->head->next;
            printf("  New file: \n");
            while(1){
                printf("\t\".%s\"\n", curr->path + strlen(EXEPATH));
                if(curr->next == NULL) break;
                curr=curr->next;
            }
            free(curr);
            printf("\n");
        }
    }
    else check = 1;
    if(UNT->head->next != NULL){//untracked 애들
        printf("Untraked files: \n");
        struct node *curr = UNT->head->next;
        printf("  New file: \n");
        while(1){
            printf("\t\".%s\"\n", curr->path + strlen(EXEPATH));
            if(curr->next == NULL) break;
            curr=curr->next;
        }
        free(curr);
        printf("\n");
    }
    else if(check) printf("Nothing to commit\n");//변경된 사항이없고 untracked도 없는 경우
}
//커밋된 사항들을 각 파일의 최신 버전의 백업본과 비교해 달라진 점을 출력
void Print_Commit(char *name){
    int fd;

    if((fd=open(COMMITPATH, O_RDWR|O_APPEND)) < 0){//커밋 로그 파일 열어서
        fprintf(stderr, "open error for %s\n", COMMITPATH);
        exit(1);
    }
    printf("commit to \"%s\"\n", name);
    if(MINUS+PLUS+MODIF){//수정된 것이 존재하면
        printf("%d files changed, %d insertions(+), %d deletions(-)\n",FCNT, PLUS-MODIF, MINUS-MODIF);//출력
    }
    if(MDF->head->next != NULL){//수정된 것들
        struct node *curr = MDF->head->next;
        while(1){
            printf("\tmodified: \".%s\"\n", curr->path + strlen(EXEPATH));
            sprintf(BUF, "commit: \"%s\" - modified: \"%s\"\n", name, curr->path);
            write(fd, BUF, strlen(BUF));
            if(curr->next == NULL) break;
            curr=curr->next;
        }
        free(curr);
        printf("\n");
    }
    if(REM->head->next != NULL){//지워진 것들
        struct node *curr = REM->head->next;
        while(1){
            printf("\tremoved: \".%s\"\n", curr->path + strlen(EXEPATH));
            sprintf(BUF, "commit: \"%s\" - removed: \"%s\"\n", name, curr->path);
            write(fd, BUF, strlen(BUF));
            if(curr->next == NULL) break;
            curr=curr->next;
        }
        free(curr);
        printf("\n");
    }
    if(NEW->head->next != NULL){//새로 추가된 것들
        struct node *curr = NEW->head->next;
        while(1){
            printf("\tnew file: \".%s\"\n", curr->path + strlen(EXEPATH));
            sprintf(BUF, "commit: \"%s\" - new file: \"%s\"\n", name, curr->path);
            write(fd, BUF, strlen(BUF));
            if(curr->next == NULL) break;
            curr=curr->next;
        }
        free(curr);
        printf("\n");
    }
    close(fd);
}
//커밋 관련 함수를 종합해놓은 커밋
void Commit(char *name){
    Status_Init();
    Status_Check(Q->head);
    if(NEW->head->next != NULL || MDF->head->next != NULL || REM->head->next != NULL){
        Make_Commit(Q->head, name);
        Print_Commit(name);
    }
    else {
        printf("Nothing to commit\n");
    }
}
//주어진 name의 커밋 로그를 출력, name이 “”라면 전체 로그 출력
int Print_Log(char *name){
    int fd;
    int command = 0, cnt, ret = 1;
    char backupname[STRMAX]={0};

    if((fd=open(COMMITPATH, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", COMMITPATH);
        exit(1);
    }

    if(strcmp(name, "") != 0){
        strcpy(backupname, name);
        printf("commit: \"%s\"\n", name);
        command = true;
    }

    while((cnt = Read_Line(fd, BUF, LOG_MOD)) > 0){
        if(!strcmp(backupname, BUF)){
            printf("\t- %s: \"%s\"\n", BUF1, inputBuf);
            ret = 0;
        }
        else if(!command){//""이면 backupname을 변형하지 않음
            printf("commit: \"%s\"\n", BUF);
            strcpy(backupname, BUF);
            printf("\t- %s: \"%s\"\n", BUF1, inputBuf);
            ret = 0;
        }
    }
    return ret;
}
