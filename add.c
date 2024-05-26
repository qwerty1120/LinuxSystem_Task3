#include "sync_header.h"
int main(int argc, char *argv[]){
    Get_Path();
    List_Setting();//초기 세팅
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
    if (strncmp(FILEPATH, getenv("HOME"), strlen(getenv("HOME")))
        || !strncmp(FILEPATH, BACKUPPATH, strlen(BACKUPPATH))
        || !strcmp(FILEPATH, getenv("HOME")))  {// 사용자 디렉토리 내부의 경로인지 확인
        fprintf(stderr, "ERROR: path must be in user directory\n - \'%s\' is not in user directory.\n", FILEPATH);
        exit(1);
    }
    //option 체크
    int optcnt = 0;
    char option;
    OPTION = 0;
    int optime = 1;
    while((option = getopt(argc - 1, argv + 1, "drt:")) != -1){
        if(option != 'd' && option != 'r' && option != 't'){//잘못된 옵션 처리
            if(optopt=='t') fprintf(stderr, "t option's [PERIOD] Empty\n");
            else fprintf(stderr, "ERROR : unknown option %c\nUsage : add [PATH] [OPTION]\n", optopt);//todo
            return -1;
        }

        optcnt++;
        if (option == 'd') {
            if (OPTION & OPT_D) {//중복체크
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
    if(Check_Path(FILEPATH)){
        printf("\".%s\" is already in monitoring.\n", FILEPATH+strlen(EXEPATH));
        exit(1);
    }
    int ch;
    if((ch = fork()) == 0)
        if(daemon_init(OPTION, optime, filepath) < 0){
            fprintf(stderr, "init failed\n");
            exit(1);
        }
    else if(ch == 1)exit(0);
}