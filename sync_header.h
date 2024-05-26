#define OPENSSL_API_COMPAT 0x10100000L

#ifndef SYNC_HEADER_H
#define SYNC_HEADER_H
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
#include <signal.h>
#include <syslog.h>

#define true 1
#define false 0

#define OPT_D 0b0001
#define OPT_R 0b0010
#define OPT_T 0b0100

#define PATHMAX 4096
#define STRMAX 255

typedef char bool;
extern int COMMAND_CNT;
extern int OPTION;
extern pid_t PID;

extern char EXEPATH[PATHMAX];
extern char LOGPATH[PATHMAX];
extern char BACKUPPATH[PATHMAX];
extern char FILEPATH[PATHMAX];
extern char inputBuf[PATHMAX*4];
extern char BUF[PATHMAX*2];
extern char BUF1[PATHMAX*2];
extern char PIDBUF[STRMAX];

extern char *COMMAND_SET[];

struct Node{
    struct timespec mod_time;
    bool isdir;
    char realpath[PATHMAX];
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
    pid_t pid;
    struct node* next;
    struct node* prev;
};

struct list{
    struct node* head;
    struct node* tail;
};

extern struct list *PID_LIST;

extern struct List *Q;
//hashing
int md5(char *target_path, char *hash_result);
//Setting
void Get_Path();

struct list * node_Init(struct list *new);
struct List * List_Init(struct List * Q);

//read staging log functions
int Read_One (int fd);
int Read_Delim(int fd, char *buf, char delim);
int Read_Line(int fd, char *buf);

//linked list atomic functions
struct Node * Find_Node(char *path, struct Node* start);
void Insert_Node(struct Node* curr, char *path);
void Insert_Recur(struct Node *curr, char *path);
void List_Setting();

//add function
void Insert_File(struct list * file, char * filepath);
int Remove_File(struct list * file);
int isFile_Exist(char * filepath, struct list * file, int pid);
void Get_Backuppath(char * path, int pid, char * result);
void Make_Dir(char *filepath, int pid, struct tm *t);
void Monitoring(char * filepath, struct list* old, struct list* new, int pid, struct timespec mod_t);
void Monitor_File(char * filepath, int pid, int opt, struct list* old_file);
int Find_Pid(int pid);
void MonitorList_Init();
int Check_Path(char * path);
int daemon_init(int opt, int time, char *filepath);

//remove function
int RemoveDirch(char *path);
void Remove_Log(int pid);

//list function
void list_tree(int height, char* isLastDir, int pid);
int Print_Log(int fd,char * name, char * output, int pid);
#endif