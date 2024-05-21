#define OPENSSL_API_COMPAT 0x10100000L

#ifndef REPO_HEADER_H
#define REPO_HEADER_H
#include <stdio.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <wait.h>
#include <limits.h>
#include <wait.h>
#define true 1

#define false 0

#define UNTRACKTED 0
#define ADD_CMD 1
#define REM_CMD 2

#define OPT_D 0b0001
#define OPT_R 0b0010
#define OPT_T 0b0100

#define PATHMAX 4096
#define STRMAX 255

typedef char bool;
extern int COMMAND_CNT;
extern int PLUS;
extern int MINUS;
extern int MODIF;
extern int FCNT;
extern int OPTION;
extern pid_t PID;

extern char EXEPATH[PATHMAX];
extern char REPOPATH[PATHMAX];
extern char COMMITPATH[PATHMAX];
extern char STAGPATH[PATHMAX];
extern char FILEPATH[PATHMAX];
extern char inputBuf[PATHMAX*4];
extern char BUF[PATHMAX*2];
extern char BUF1[PATHMAX*2];

extern char *COMMAND_SET[];

struct Node{
    int mode;
    int status;//status가 true 이면 mode 와 상관없이 child를 볼 필요가 있다. dir가 remove 된 후 안의 파일이 add 된 경우 status가 true가 된다.
    int time;
    struct timespec mod_time;
    pid_t pid;
    bool isdir;
    char realpath[PATHMAX];
    char backupname[STRMAX];
    struct Node *next;
    struct Node *prev;
    struct Node *child;
    struct Node *parent;
};

struct List{
    struct Node *head;
    struct Node *tail;
};

struct node{
    char path[PATHMAX];
    int line;
    int mod;
    struct node* next;
    struct node* prev;
};

struct list{
    struct node* head;
    struct node* tail;
};

extern struct list *NEW;
extern struct list *MDF;
extern struct list *REM;
extern struct list *UNT;

extern struct List *Q;
//hashing
int md5(char *target_path, char *hash_result);
//Setting
void Get_Path();

struct list * node_Init(struct list *new);
struct List * List_Init(struct List * Q);
void Status_Init();

void Stag_Setting();

//read staging log functions
char Read_One (int fd);
int Read_Delim(int fd, char *buf, char delim);
int Read_Line(int fd, char *buf, int mode);

//linked list atomic functions
struct Node * Find_Node(char *path, struct Node* start);
void Insert_Node(struct Node* curr, char *path);
void Insert_Recur(struct Node *curr, char *path);
void List_Setting();
//status check
int Cmd_File_Switch(int command, struct Node *start);
int Cmd_Recur_Switch(int command, struct Node *start, int opt);
int Check_Status(struct Node *start, int command, int opt);

void File_Status(struct Node *file);
void Status_Check(struct Node *start);
void Print_Status();
//commit check
char * Commit_Path(char * name, char * path);
void Make_Commit(struct Node *start, char *name);
void Print_Commit(char *name);
void Commit(char *name);
//log function
int Print_Log(char *name);
//counting modified lines functions
int countLines(int fd);
void getOffsets(int fd, int *arr, int N);
void LineByLine(int **dp, int i, char * str1, int fd, int *arr, int M);
void doLCS(int **dp, int *arr1, int *arr2, int fd1, int fd2, int N, int M);
int Count_Line(char *fname1, char *fname2);

#endif //REPO_HEADER_H