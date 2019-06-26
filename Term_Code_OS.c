//
//  main.c
//  CPUScheduling - multilevel feedback Q 까지 넣었음.
//
//  Created by 임혜수 on 08/05/2019.
//  Copyright © 2019 HyesuLim. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_NUM_PROCESS 20
#define NUM_PROCESS 10
#define TIMEQUANTUM 3

#define FOREGROUND 1
#define BACKGROUND 2

#define FCFS 1
#define PRM_SJF 2
#define NONPRM_SJF 3
#define PRM_PRIORITY 4
#define NONPRM_PRIORITY 5
#define RR 6
#define PRM_FCFS 7
#define RR_PRI 8
#define RR_SJF 9
#define PRM_PRIORITY_MFQ 10

int myTime = 0;
int cpuIdleTime = 0;
int ellapsedTime = 0;

int selectedReadyQ = 0;

// process
typedef struct process* processPtr;
typedef struct process {
    int pid;
    int CPUburst;
    int IOburst;
    int CPUremainingTime;
    int IOremainingTime;
    int arrivalTime;
    int priority;
    int IOstartTime;
    int waitingTime;
    int turnaroundTime;
    int responseTime;
}process;

// evaluation 을 위한 배열 선언.
double avgWaitingTime[10] = {0,};
double avgTurnaroundTime[10] = {0,};
double avgResponseTime[10] = {0, };
double utilization[10] = {0, };


// jobQ, backupQ, readyQ, tempQ, waitingQ, runQ 가 있음.
// backupQ 는 create 된 process 정보를 저장하기 위한 용도.
// tempQ 는 RR 에서 ready -> run -> temp -> ready 를 위한 용도.
// waitingQ 는 IO 를 실행하고 있는 프로세스들이 있는 곳.
// runQ 는 크기가 1 로 run 상태에 있는 process 를 의미함.

// -------------------- job Q START --------------------
processPtr jobQ[MAX_NUM_PROCESS];
int curProcNumJobQ = 0;

void init_jobQ(){
    curProcNumJobQ = 0;
    for (int i=0; i<MAX_NUM_PROCESS; i++){
        jobQ[i] = NULL;
    }
}

void insert_jobQ(processPtr newProcess) {
    if(curProcNumJobQ < MAX_NUM_PROCESS){
        jobQ[curProcNumJobQ] = newProcess;
        curProcNumJobQ++;
    }
    else{
        printf("---- ERROR : Job Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_jobQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumJobQ; i++){
        if(jobQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_jobQ(processPtr process){
    if(curProcNumJobQ > 0){
        // 삭제하고자 하는 process 의 pid 가 jobQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_jobQ(process->pid);
        if(index != -1){
            processPtr outProc = jobQ[index];
            // jobQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumJobQ; i++){
                jobQ[i] = jobQ[i+1];
            }
            jobQ[curProcNumJobQ-1] = NULL;
            curProcNumJobQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Job Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Job Queue is EMPTY ----\n");
        return NULL;
    }
}

void print_jobQ(){
    printf("\n==== PRINT job Queue ====\n\n");
    for(int i=0; i<curProcNumJobQ; i++){
        printf("jobQ    pid %d\n", jobQ[i]->pid);
    }
}

void clear_jobQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(jobQ[i]);
        jobQ[i] = NULL;
    }
    curProcNumJobQ = 0;
}
// -------------------- job Q END --------------------

// -------------------- backup Q START --------------------
processPtr backupQ[MAX_NUM_PROCESS];
int curProcNumBackupQ = 0;

// clone processes from jobQ to backupQ
void clone_to_backupQ(processPtr jobQ[]){
    int i=0;
    
    // initialize backupQ
    for(i=0;i<MAX_NUM_PROCESS; i++){
        backupQ[i] = NULL;
    }
    
    // clone processes
    for(i=0; i<curProcNumJobQ; i++){
        backupQ[i] = (processPtr) malloc(sizeof(struct process));
        memcpy(backupQ[i], jobQ[i], sizeof(struct process));
    }
    curProcNumBackupQ = i;
    
    return;
}

// load processes from backupQ to jobQ
void load_from_backupQ(processPtr jobQ[]){
    int i=0;
    for(i=0; i<curProcNumBackupQ; i++){
        jobQ[i] = backupQ[i];
    }
    curProcNumJobQ = i;
    for(;i<MAX_NUM_PROCESS; i++){
        jobQ[i] = NULL;
    }
    return;
}

void print_backupQ(){
    printf("\n==== PRINT Backup Queue ====\n\n");
    for(int i=0; i<curProcNumBackupQ; i++){
        printf("backupQ pid %d\n", backupQ[i]->pid);
    }
}
void clear_backupQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(backupQ[i]);
        backupQ[i] = NULL;
    }
    curProcNumBackupQ = 0;
}
// -------------------- backup Q END --------------------

// -------------------- ready Q START --------------------
processPtr readyQ[MAX_NUM_PROCESS];
int curProcNumReadyQ = 0;

void init_readyQ(){
    curProcNumReadyQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        readyQ[i] = NULL;
    }
    return;
}

void insert_readyQ(processPtr newProcess) {
    if(curProcNumReadyQ < MAX_NUM_PROCESS){
        readyQ[curProcNumReadyQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(readyQ[curProcNumReadyQ], newProcess, sizeof(struct process));
        curProcNumReadyQ++;
    }
    else{
        printf("---- ERROR : Ready Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_readyQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumReadyQ; i++){
        if(readyQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_readyQ(processPtr process){
    if(curProcNumReadyQ > 0){
        // 삭제하고자 하는 process 의 pid 가 readyQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_readyQ(process->pid);
        if(index != -1){
            processPtr outProc = readyQ[index];
            // readyQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumReadyQ; i++){
                readyQ[i] = readyQ[i+1];
            }
            readyQ[curProcNumReadyQ-1] = NULL;
            curProcNumReadyQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Ready Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Ready Queue is EMPTY ----\n");
        return NULL;
    }
}

void print_readyQ(){
    printf("\n==== PRINT Ready Queue ====\n\n");
    for(int i=0; i<curProcNumReadyQ; i++){
        printf("readyQ  pid %d\n", readyQ[i]->pid);
    }
}
void clear_readyQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(readyQ[i]);
        readyQ[i] = NULL;
    }
    curProcNumReadyQ = 0;
}

// -------------------- ready Q END --------------------

// -------------------- foreground Q START --------------------
processPtr foreQ[MAX_NUM_PROCESS];
int curProcNumForeQ = 0;

void init_foreQ(){
    curProcNumForeQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        foreQ[i] = NULL;
    }
    return;
}

void insert_foreQ(processPtr newProcess) {
    if(curProcNumForeQ < MAX_NUM_PROCESS){
        foreQ[curProcNumForeQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(foreQ[curProcNumForeQ], newProcess, sizeof(struct process));
        curProcNumForeQ++;
    }
    else{
        printf("---- ERROR : Fore Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_foreQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumForeQ; i++){
        if(foreQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_foreQ(processPtr process){
    if(curProcNumForeQ > 0){
        // 삭제하고자 하는 process 의 pid 가 foreQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_foreQ(process->pid);
        if(index != -1){
            processPtr outProc = foreQ[index];
            // foreQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumForeQ; i++){
                foreQ[i] = foreQ[i+1];
            }
            foreQ[curProcNumForeQ-1] = NULL;
            curProcNumForeQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Fore Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Fore Queue is EMPTY ----\n");
        return NULL;
    }
}

// load processes from foreQ to readyQ
void load_from_foreQ(processPtr readyQ[]){
    int i=0;
    for(i=0; i<curProcNumForeQ; i++){
        readyQ[i] = (processPtr) malloc(sizeof(struct process));
        memcpy(readyQ[i], foreQ[i], sizeof(struct process));
    }
    curProcNumReadyQ = i;
    return;
}

void print_foreQ(){
    printf("\n==== PRINT Fore Queue ====\n\n");
    for(int i=0; i<curProcNumForeQ; i++){
        printf("foreQ  pid %d\n", foreQ[i]->pid);
    }
}
void clear_foreQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(foreQ[i]);
        foreQ[i] = NULL;
    }
    curProcNumForeQ = 0;
}

// -------------------- foreground Q END --------------------

// -------------------- background Q START --------------------
processPtr backQ[MAX_NUM_PROCESS];
int curProcNumBackQ = 0;

void init_backQ(){
    curProcNumBackQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        backQ[i] = NULL;
    }
    return;
}

void insert_backQ(processPtr newProcess) {
    if(curProcNumBackQ < MAX_NUM_PROCESS){
        backQ[curProcNumBackQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(backQ[curProcNumBackQ], newProcess, sizeof(struct process));
        curProcNumBackQ++;
    }
    else{
        printf("---- ERROR : Back Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_backQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumBackQ; i++){
        if(backQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_backQ(processPtr process){
    if(curProcNumBackQ > 0){
        // 삭제하고자 하는 process 의 pid 가 foreQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_backQ(process->pid);
        if(index != -1){
            processPtr outProc = backQ[index];
            // foreQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumBackQ; i++){
                backQ[i] = backQ[i+1];
            }
            backQ[curProcNumBackQ-1] = NULL;
            curProcNumBackQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Back Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Back Queue is EMPTY ----\n");
        return NULL;
    }
}

// load processes from backQ to readyQ
void load_from_backQ(processPtr readyQ[]){
    int i=0;
    for(i=0; i<curProcNumBackQ; i++){
        readyQ[i] = (processPtr) malloc(sizeof(struct process));
        memcpy(readyQ[i], backQ[i], sizeof(struct process));
    }
    curProcNumReadyQ = i;
    return;
}

void print_backQ(){
    printf("\n==== PRINT Back Queue ====\n\n");
    for(int i=0; i<curProcNumBackQ; i++){
        printf("backQ  pid %d\n", backQ[i]->pid);
    }
}
void clear_backQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(backQ[i]);
        backQ[i] = NULL;
    }
    curProcNumBackQ = 0;
}

// -------------------- background Q END --------------------

// -------------------- temp Q START --------------------
processPtr tempQ[MAX_NUM_PROCESS];
int curProcNumTempQ = 0;

void init_tempQ(){
    curProcNumTempQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        tempQ[i] = NULL;
    }
    return;
}

// load processes from tmepQ to readyQ
void load_from_tempQ(processPtr readyQ[]){
    int i=0;
    for(i=0; i<curProcNumTempQ; i++){
        readyQ[i] = tempQ[i];
    }
    curProcNumReadyQ = i;
    for(;i<MAX_NUM_PROCESS; i++){
        readyQ[i] = NULL;
    }
    return;
}


void insert_tempQ(processPtr newProcess) {
    if(curProcNumTempQ < MAX_NUM_PROCESS){
        tempQ[curProcNumTempQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(tempQ[curProcNumTempQ], newProcess, sizeof(struct process));
        curProcNumTempQ++;
    }
    else{
        printf("---- ERROR : Temp Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_tempQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumTempQ; i++){
        if(tempQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_tempQ(processPtr process){
    if(curProcNumTempQ > 0){
        // 삭제하고자 하는 process 의 pid 가 readyQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_tempQ(process->pid);
        if(index != -1){
            processPtr outProc = tempQ[index];
            // readyQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumTempQ; i++){
                tempQ[i] = tempQ[i+1];
            }
            tempQ[curProcNumTempQ-1] = NULL;
            curProcNumTempQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Temp Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Temp Queue is EMPTY ----\n");
        return NULL;
    }
}

void print_tempQ(){
    printf("\n==== PRINT Temp Queue ====\n\n");
    for(int i=0; i<curProcNumTempQ; i++){
        printf("tempQ  pid %d\n", tempQ[i]->pid);
    }
}

void clear_tempQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(tempQ[i]);
        tempQ[i] = NULL;
    }
    curProcNumTempQ = 0;
}
// -------------------- temp Q END --------------------

// -------------------- waiting Q START --------------------
processPtr waitingQ[MAX_NUM_PROCESS];
int curProcNumWaitingQ = 0;

void init_waitingQ(){
    curProcNumWaitingQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        waitingQ[i] = NULL;
    }
    return;
}

void insert_waitingQ(processPtr newProcess) {
    if(curProcNumWaitingQ < MAX_NUM_PROCESS){
        waitingQ[curProcNumWaitingQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(waitingQ[curProcNumWaitingQ], newProcess, sizeof(struct process));
        curProcNumWaitingQ++;
    }
    else{
        printf("---- ERROR : Waiting Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_waitingQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumWaitingQ; i++){
        if(waitingQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_waitingQ(processPtr process){
    if(curProcNumWaitingQ > 0){
        // 삭제하고자 하는 process 의 pid 가 waitingQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_waitingQ(process->pid);
        if(index != -1){
            processPtr outProc = waitingQ[index];
            // readyQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumWaitingQ; i++){
                waitingQ[i] = waitingQ[i+1];
            }
            waitingQ[curProcNumWaitingQ-1] = NULL;
            curProcNumWaitingQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Waiting Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Waiting Queue is EMPTY ----\n");
        return NULL;
    }
}

void print_waitingQ(){
    printf("\n==== PRINT Waiting Queue ====\n\n");
    for(int i=0; i<curProcNumWaitingQ; i++){
        printf("waitingQ    pid %d\n", waitingQ[i]->pid);
    }
}

void clear_waitingQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(waitingQ[i]);
        waitingQ[i] = NULL;
    }
    curProcNumWaitingQ = 0;
}

// -------------------- waiting Q END --------------------

// -------------------- run Q START --------------------
processPtr runQ[1]; // only one process can in the run state.
int curProcNumRunQ = 0;

void init_runQ(){
    curProcNumRunQ = 0;
    runQ[0] = NULL;
    return;
}

void insert_runQ(processPtr newProcess) {
    if(curProcNumRunQ < 1){
        runQ[curProcNumRunQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(runQ[curProcNumRunQ], newProcess, sizeof(struct process));
        curProcNumRunQ++;
    }
    else{
        printf("---- ERROR : Run Queue is FULL ----\n");
        return;
    }
}

void print_runQ(){
    printf("\n==== PRINT Run Queue ====\n\n");
    for(int i=0; i<curProcNumRunQ; i++){
        printf("runQ    pid %d\n", runQ[i]->pid);
    }
}

void clear_runQ() {
    int i;
    for(i = 0; i < 1; i++) {
        free(runQ[i]);
        runQ[i] = NULL;
    }
    curProcNumRunQ = 0;
}


// -------------------- run Q END --------------------

// -------------------- terminated Q START --------------------
processPtr terminatedQ[MAX_NUM_PROCESS];
int curProcNumTerminatedQ = 0;

void init_terminatedQ(){
    curProcNumTerminatedQ = 0;
    for(int i=0; i<MAX_NUM_PROCESS; i++){
        terminatedQ[i] = NULL;
    }
    return;
}

void insert_terminatedQ(processPtr newProcess) {
    if(curProcNumTerminatedQ < MAX_NUM_PROCESS){
        terminatedQ[curProcNumTerminatedQ] = (processPtr) malloc(sizeof(struct process));
        memcpy(terminatedQ[curProcNumTerminatedQ], newProcess, sizeof(struct process));
        curProcNumTerminatedQ++;
    }
    else{
        printf("---- ERROR : Terminated Queue is FULL ----\n");
        return;
    }
}

int getProcIndex_terminatedQ(int pid){
    int index = -1;
    for(int i=0; i<curProcNumTerminatedQ; i++){
        if(terminatedQ[i]->pid == pid){
            index = i;
            break;
        }
    }
    return index;
}

processPtr delete_terminatedQ(processPtr process){
    if(curProcNumTerminatedQ > 0){
        // 삭제하고자 하는 process 의 pid 가 terminatedQ 에 있는지 확인하고 index 가져옴.
        int index = getProcIndex_terminatedQ(process->pid);
        if(index != -1){
            processPtr outProc = terminatedQ[index];
            // readyQ 가 중간에 비었기 때문에 다시 정렬해줌.
            for(int i=index; i<curProcNumTerminatedQ; i++){
                terminatedQ[i] = terminatedQ[i+1];
            }
            terminatedQ[curProcNumTerminatedQ-1] = NULL;
            curProcNumTerminatedQ--;
            return outProc;
        }
        else{
            printf("---- ERROR : The process is not in Terminated Queue ----\n");
            return NULL;
        }
    }
    else{
        printf("---- ERROR : Terminated Queue is EMPTY ----\n");
        return NULL;
    }
}

void print_terminatedQ(){
    printf("\n==== PRINT Terminated Queue ====\n\n");
    for(int i=0; i<curProcNumTerminatedQ; i++){
        printf("terminatedQ    pid %d\n", terminatedQ[i]->pid);
    }
}

void clear_terminatedQ() {
    int i;
    for(i = 0; i < MAX_NUM_PROCESS; i++) {
        free(terminatedQ[i]);
        terminatedQ[i] = NULL;
    }
    curProcNumTerminatedQ = 0;
}

// -------------------- terminated Q END --------------------

// CPU scheduling algorithm 시작.

// -------------------- Algorithm 1 FCFS START --------------------
void FCFS_algo(int curTime){
    // 먼저 도착한 process 부터 readyQ 에서 뽑아 내서 "run" 상태로 만들어야 한다. -> runProcess
    if(curProcNumReadyQ == 0){
        // nothing to schedule..
        return;
    }
    else{
        // if there is some processes to schedule in the ready Queue
        // arrival time 이 가장 빠른 process 를 찾아 readyQ 에서 delete 하고, runQ 로 넣는다.
        
        processPtr firstArrivedProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            firstArrivedProc[i] = NULL;
        }
        
        // 가장 처음에 도착한 arrival time 찾기.
        int firstArrivedTime = readyQ[0]->arrivalTime;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(firstArrivedTime > readyQ[i]->arrivalTime){
                firstArrivedTime = readyQ[i]->arrivalTime;
            }
        }
        
        // arrival Time 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        // arrival Time 이 같은 process 에 대해서 어느 하나의 프로세스가 이미 실행중이라면 굳이 preemption 하지 않음. 진행 중인  프로세스를 마저 진행함.
        int j = 0; // j 는 firstArrivedTime 에 도착한 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->arrivalTime == firstArrivedTime){
                firstArrivedProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(firstArrivedProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){ //firstArrivedProc 에 process 가 있다면
            int minPid = MAX_NUM_PROCESS;
            // Pid 가 작은 프로세스를 찾는다.
            for(int i=0; i<j; i++){
                if(minPid > firstArrivedProc[i]->pid){
                    minPid = firstArrivedProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                // minimum pid 를 가진 process 에 대하여
                if(minPid == firstArrivedProc[i]->pid){
                    if(curProcNumRunQ!=0) // cpu 가 idle 하지 않다면 do nothing
                        break;
                    else{ // cpu 가 idle 하다면 해당 프로세스를 run.
                        insert_runQ(firstArrivedProc[i]);
                        delete_readyQ(firstArrivedProc[i]);
                        break;
                    }
                    // 하나의 프로세스만 run 시키면 되므로 i++ 안해줘도 되고, 바로 break.
                }
                else{
                    i++;
                }
            }
        }
    }
}

// -------------------- Algorithm 1 FCFS END --------------------

// -------------------- Algorithm 2 preemptive SJF START --------------------
void prmSJF_algo(){
    // readyQ 가 바뀔 때마다 cpuscheduling을 하게 됨. runQ 에 있는 프로세스도 고려해서 스케줄링해야함. readyQ 에서 스케줄링 먼저하고 runQ랑 다시 비교할 것.
    // CPUburst + IOburst = totalBurst 가 가장 짧은 process 를 찾는다.
    // totalBurst 가 같은 process 가 있는 경우, pid가 작은 process 부터 run 한다.
    if(curProcNumReadyQ==0){
        // nothing to CPUschedule
        return;
    }
    else{
        // there is some processes in readyQ to schedule
        
        processPtr minTotalBurstProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minTotalBurstProc[i] = NULL;
        }
        
        // find minimal totalBurst
        int minTotalBurst = readyQ[0]->CPUburst + readyQ[0]->IOburst;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minTotalBurst > readyQ[i]->CPUburst + readyQ[i]->IOburst)
                minTotalBurst = readyQ[i]->CPUburst + readyQ[i]->IOburst;
        }
        
        // totalbursttime 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 mintotalburst 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->CPUburst + readyQ[i]->IOburst == minTotalBurst){
                minTotalBurstProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minTotalBurstProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minTotalBurstProc[i]->pid){
                    minPid = minTotalBurstProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minTotalBurstProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 minTotalBurstProc[i] 와 runQ[curProcNumRunQ-1] 의 totalburst 를 비교.
                        if(minTotalBurst >= runQ[curProcNumRunQ-1]->CPUburst + runQ[curProcNumRunQ-1]->IOburst){
                            // preemption 적용 못함.
                            break;
                        }
                        else{ // preemption 적용 함.
                            if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                                // 바로 terminate
                                insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                                printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                                init_runQ();
                                // 다시 스케줄링 해야 함.
                                prmSJF_algo();
                            }
                            else{
                                insert_readyQ(runQ[curProcNumRunQ-1]);
                                init_runQ();
                                insert_runQ(minTotalBurstProc[i]);
                                delete_readyQ(minTotalBurstProc[i]);
                            }
                            
                        }
                        break;
                    }
                    
                    else{ // cpu 가 idle 하다면, minTotalBurstProc[i] 를 run 시켜야 함.
                        insert_runQ(minTotalBurstProc[i]);
                        delete_readyQ(minTotalBurstProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
}

// -------------------- Algorithm 2 preemptive SJF END --------------------

// -------------------- Algorithm 3 nonpreemptive SJF START --------------------
void nonprmSJF_algo(){
    // CPUburst + IOburst = totalBurst 가 가장 짧은 process 를 찾는다.
    // totalBurst 가 같은 process 가 있는 경우, pid가 작은 process 부터 run 한다.
    if(curProcNumReadyQ==0){
        // nothing to CPUschedule
        return;
    }
    else{
        // there is some processes in readyQ to schedule
        
        processPtr minTotalBurstProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minTotalBurstProc[i] = NULL;
        }
        
        // find minimal totalBurst
        int minTotalBurst = readyQ[0]->CPUburst + readyQ[0]->IOburst;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minTotalBurst > readyQ[i]->CPUburst + readyQ[i]->IOburst)
                minTotalBurst = readyQ[i]->CPUburst + readyQ[i]->IOburst;
        }
        
        // totalbursttime 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 mintotalburst 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->CPUburst + readyQ[i]->IOburst == minTotalBurst){
                minTotalBurstProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minTotalBurstProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minTotalBurstProc[i]->pid){
                    minPid = minTotalBurstProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minTotalBurstProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 do nothing
                        break;
                    }
                    
                    else{ // cpu 가 idle 하다면, minTotalBurstProc[i] 를 run 시켜야 함.
                        insert_runQ(minTotalBurstProc[i]);
                        delete_readyQ(minTotalBurstProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
}

// -------------------- Algorithm 3 nonpreemptive SJF END --------------------

// -------------------- Algorithm 4 preemptive priority START --------------------
void prmPriority_algo(){
    // priority 값이 가장 낮은 것이 가장 우선순위가 높다고 한다. -> 가장 높은 priority 를 찾으면 된다.
    // priority 가 같은 경우 pid 가 낮은 process 부터 적용.
    // 만약 같은 priority 인데 이미 어느 한 process 가 이미 실행 중이라면 굳이 preemption 안함.
    if(curProcNumReadyQ==0){
        // nothing to CPUschedule
        return;
    }
    else{
        // there is some processes in readyQ to schedule
        
        processPtr minPriorityProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minPriorityProc[i] = NULL;
        }
        
        // find minPriority
        int minPriority = readyQ[0]->priority;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minPriority > readyQ[i]->priority)
                minPriority = readyQ[i]->priority;
        }
        
        // priority 가 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 minpriority 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->priority == minPriority){
                minPriorityProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minPriorityProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minPriorityProc[i]->pid){
                    minPid = minPriorityProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minPriorityProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 minPriorityProc[i] 와 runQ[curProcNumRunQ-1] 의 priority 를 비교.
                        if(minPriority >= runQ[curProcNumRunQ-1]->priority){
                            // preemption 적용 못함.
                            break;
                        }
                        else{ // preemption 적용 함.
                            if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                                // 바로 terminate
                                insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                                printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                                init_runQ();
                                // 다시 스케줄링 해야 함.
                                prmPriority_algo();
                            }
                            else{
                                insert_readyQ(runQ[curProcNumRunQ-1]);
                                init_runQ();
                                insert_runQ(minPriorityProc[i]);
                                delete_readyQ(minPriorityProc[i]);
                            }
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면, minPriorityProc[i] 를 run 시켜야 함.
                        insert_runQ(minPriorityProc[i]);
                        delete_readyQ(minPriorityProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
    
    
}

// -------------------- Algorithm 4 preemptive priority END --------------------

// -------------------- Algorithm 5 nonpreemptive priority START --------------------
void nonprmPriority_algo(){
    if(curProcNumReadyQ==0){
        // nothing to CPUschedule
        return;
    }
    else{
        // there is some processes in readyQ to schedule
        
        processPtr minPriorityProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minPriorityProc[i] = NULL;
        }
        
        // find minPriority
        int minPriority = readyQ[0]->priority;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minPriority > readyQ[i]->priority)
                minPriority = readyQ[i]->priority;
        }
        
        // priority 가 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 minpriority 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->priority == minPriority){
                minPriorityProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minPriorityProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minPriorityProc[i]->pid){
                    minPid = minPriorityProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minPriorityProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 do nothing.
                        break;
                    }
                    
                    else{ // cpu 가 idle 하다면, minPriorityProc[i] 를 run 시켜야 함.
                        insert_runQ(minPriorityProc[i]);
                        delete_readyQ(minPriorityProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
}
// -------------------- Algorithm 5 nonpreemptive priority END --------------------

// -------------------- Algorithm 6 RR START --------------------
void RR_algo(){
    // timequantum : 3
    // fcfs 랑 유사하지만 preemption이 추가 되었음.
    // 각각의 프로세스는 time quantum 을 갖고 이 시간이 지나면 preempted 되고, ready queue 끝에 들어간다.
    
    // run->wait 인 경우. wait->ready 인 경우.
    // run->wait->temp 로 하고, temp->ready로 가게 한다면, 문제 해결.
    // 언제 wait->temp 되어야 하나?
    // RR에서는,  IO complete 일 때 insertTempQ 로 한다면, 될 것 같다.
    
    // ready -> run -> temp -> ready 순으로 프로세스는 Q 를 옮겨다닌다.
    // ellapsedTime == timequnatum 이면 run -> temp
    // ready가 empty 이면 temp -> ready
    
    if(curProcNumReadyQ == 0 && curProcNumTempQ == 0){ // readyQ , tempQ 모두 비어있을 때
        if(curProcNumRunQ!=0 && ellapsedTime >= TIMEQUANTUM){
            // run process 의 time 이 초과되면 다시 cpuscheduling을 해야 함...
            ellapsedTime = 0;
            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
            init_runQ();
            RR_algo();
        }
        // 그렇지 않으면 nothing to schedule.
        return;
    }
    else{
        if(curProcNumReadyQ == 0 && curProcNumTempQ != 0) {
            // tempQ 에 있는 process 를 readyQ 에 넣고 스케줄링 해야 한다.
            init_readyQ();
            load_from_tempQ(readyQ);
            init_tempQ();
        }
        // if there is some processes to schedule in the ready Queue
        // arrival time 이 가장 빠른 process 를 찾아 readyQ 에서 delete 하고, runQ 로 넣는다.
        
        processPtr firstArrivedProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            firstArrivedProc[i] = NULL;
        }
        
        // 가장 처음에 도착한 arrival time 찾기.
        int firstArrivedTime = readyQ[0]->arrivalTime;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(firstArrivedTime > readyQ[i]->arrivalTime){
                firstArrivedTime = readyQ[i]->arrivalTime;
            }
        }
        
        // arrival Time 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 firstArrivedTime 에 도착한 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->arrivalTime == firstArrivedTime){
                firstArrivedProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(firstArrivedProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > firstArrivedProc[i]->pid){
                    minPid = firstArrivedProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == firstArrivedProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면
                        // 만약 RunQ가 terminate 되어야 한다면?
                        if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                            // 바로 terminate
                            insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                            printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                            init_runQ();
                            RR_algo();
                        }
                        if(ellapsedTime >= TIMEQUANTUM){ // Timequantum 지나면 다시 cpuscheduling을 해야 함...
                            ellapsedTime = 0;
                            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
                            init_runQ();
                            RR_algo();
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면 readyQ 에 잇는 process 를 run 함.
                        ellapsedTime = 0;
                        insert_runQ(firstArrivedProc[i]);
                        delete_readyQ(firstArrivedProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
    return;
}

// -------------------- Algorithm 6 RR END --------------------


// -------------------- Algorithm 7 preemptive FCFS START --------------------
void prmFCFS_algo(int curTime){
    // 먼저 도착한 process 부터 readyQ 에서 뽑아 내서 "run" 상태로 만들어야 한다. -> runProcess
    if(curProcNumReadyQ == 0){
        // nothing to schedule..
        return;
    }
    else{
        // if there is some processes to schedule in the ready Queue
        // arrival time 이 가장 빠른 process 를 찾아 readyQ 에서 delete 하고, runQ 로 넣는다.
        
        processPtr firstArrivedProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            firstArrivedProc[i] = NULL;
        }
        
        // 가장 처음에 도착한 arrival time 찾기.
        int firstArrivedTime = readyQ[0]->arrivalTime;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(firstArrivedTime > readyQ[i]->arrivalTime){
                firstArrivedTime = readyQ[i]->arrivalTime;
            }
        }
        
        // arrival Time 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        // arrival Time 이 같은 process 에 대해서 어느 하나의 프로세스가 이미 실행중이라면 굳이 preemption 하지 않음. 진행 중인  프로세스를 마저 진행함.
        int j = 0; // j 는 firstArrivedTime 에 도착한 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->arrivalTime == firstArrivedTime){
                firstArrivedProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(firstArrivedProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){ //firstArrivedProc 에 process 가 있다면
            int minPid = MAX_NUM_PROCESS;
            // Pid 가 작은 프로세스를 찾는다.
            for(int i=0; i<j; i++){
                if(minPid > firstArrivedProc[i]->pid){
                    minPid = firstArrivedProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                // minimum pid 를 가진 process 에 대하여
                if(minPid == firstArrivedProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 firstArrivedProc[i] 와 runQ 의 arrivaltime 비교
                        if(firstArrivedTime >= runQ[curProcNumRunQ-1]->arrivalTime){ // preemption 적용 못함.
                            break;
                        }
                        else{ // preemption 적용 함.
                            if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                                // 바로 terminate
                                insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                                printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                                init_runQ();
                                prmFCFS_algo(curTime);
                            }
                            else{
                                insert_readyQ(runQ[curProcNumRunQ-1]);
                                init_runQ();
                                insert_runQ(firstArrivedProc[i]);
                                delete_readyQ(firstArrivedProc[i]);
                            }
                            
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면 해당 프로세스를 run.
                        insert_runQ(firstArrivedProc[i]);
                        delete_readyQ(firstArrivedProc[i]);
                        break;
                    }
                    // 하나의 프로세스만 run 시키면 되므로 i++ 안해줘도 되고, 바로 break.
                }
                else{
                    i++;
                }
            }
        }
    }
}

// -------------------- Algorithm 7 preemptive FCFS END --------------------

// -------------------- Algorithm 8 RR_priority START --------------------
void RR_priority_algo(){
    
    if(curProcNumReadyQ == 0 && curProcNumTempQ == 0){ // readyQ , tempQ 모두 비어있을 때
        if(curProcNumRunQ!=0 && ellapsedTime >= TIMEQUANTUM){
            // run process 의 time 이 초과되면 다시 cpuscheduling을 해야 함...
            ellapsedTime = 0;
            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
            init_runQ();
            RR_priority_algo();
        }
        // 그렇지 않으면 nothing to schedule.
        return;
    }
    else{
        if(curProcNumReadyQ == 0 && curProcNumTempQ != 0) {
            // tempQ 에 있는 process 를 readyQ 에 넣고 스케줄링 해야 한다.
            init_readyQ();
            load_from_tempQ(readyQ);
            init_tempQ();
        }
        // if there is some processes to schedule in the ready Queue
        // arrival time 이 가장 빠른 process 를 찾아 readyQ 에서 delete 하고, runQ 로 넣는다.
        
        processPtr minPriorityProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minPriorityProc[i] = NULL;
        }
        
        // find minPriority
        int minPriority = readyQ[0]->priority;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minPriority > readyQ[i]->priority)
                minPriority = readyQ[i]->priority;
        }
        
        // priority 가 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 minpriority 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->priority == minPriority){
                minPriorityProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minPriorityProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minPriorityProc[i]->pid){
                    minPid = minPriorityProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minPriorityProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면
                        // 만약 RunQ가 terminate 되어야 한다면?
                        if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                            // 바로 terminate
                            insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                            printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                            init_runQ();
                            RR_priority_algo();
                        }
                        if(ellapsedTime >= TIMEQUANTUM){ // Timequantum 지나면 다시 cpuscheduling을 해야 함...
                            ellapsedTime = 0;
                            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
                            init_runQ();
                            RR_priority_algo();
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면 readyQ 에 잇는 process 를 run 함.
                        ellapsedTime = 0;
                        insert_runQ(minPriorityProc[i]);
                        delete_readyQ(minPriorityProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
    return;
}

// -------------------- Algorithm 8 RR_priority END --------------------

// -------------------- Algorithm 9 RR_SJF START --------------------
void RR_SJF_algo(){
    
    if(curProcNumReadyQ == 0 && curProcNumTempQ == 0){ // readyQ , tempQ 모두 비어있을 때
        if(curProcNumRunQ!=0 && ellapsedTime >= TIMEQUANTUM){
            // run process 의 time 이 초과되면 다시 cpuscheduling을 해야 함...
            ellapsedTime = 0;
            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
            init_runQ();
            RR_SJF_algo();
        }
        // 그렇지 않으면 nothing to schedule.
        return;
    }
    else{
        if(curProcNumReadyQ == 0 && curProcNumTempQ != 0) {
            // tempQ 에 있는 process 를 readyQ 에 넣고 스케줄링 해야 한다.
            init_readyQ();
            load_from_tempQ(readyQ);
            init_tempQ();
        }
        // if there is some processes to schedule in the ready Queue
        // arrival time 이 가장 빠른 process 를 찾아 readyQ 에서 delete 하고, runQ 로 넣는다.
        
        processPtr minTotalBurstProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minTotalBurstProc[i] = NULL;
        }
        
        // find minimal totalBurst
        int minTotalBurst = readyQ[0]->CPUburst + readyQ[0]->IOburst;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minTotalBurst > readyQ[i]->CPUburst + readyQ[i]->IOburst)
                minTotalBurst = readyQ[i]->CPUburst + readyQ[i]->IOburst;
        }
        
        // totalbursttime 이 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 mintotalburst 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->CPUburst + readyQ[i]->IOburst == minTotalBurst){
                minTotalBurstProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minTotalBurstProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minTotalBurstProc[i]->pid){
                    minPid = minTotalBurstProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minTotalBurstProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면
                        // 만약 RunQ가 terminate 되어야 한다면?
                        if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
                            // 바로 terminate
                            insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
                            printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
                            init_runQ();
                            RR_SJF_algo();
                        }
                        if(ellapsedTime >= TIMEQUANTUM){ // Timequantum 지나면 다시 cpuscheduling을 해야 함...
                            ellapsedTime = 0;
                            insert_tempQ(runQ[curProcNumRunQ-1]); // 다른 프로세스에게 우선권을 주기 위해 직전에 실행한 process 는 tempQ에 넣음.
                            init_runQ();
                            RR_SJF_algo();
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면 readyQ 에 잇는 process 를 run 함.
                        ellapsedTime = 0;
                        insert_runQ(minTotalBurstProc[i]);
                        delete_readyQ(minTotalBurstProc[i]);
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
    return;
}

// -------------------- Algorithm 9 RR_SJF END --------------------


void jobScheduling(int curTime){
    // curTime 에 arrive 하는 process 가 있다면 JobQ 에서 readyQ 로 넣어준다.
    for(int i=0; i<curProcNumJobQ;){
        if(jobQ[i]->arrivalTime == curTime){
            // 현재 시간이 arrivalTime 과 같다면 readyQ 에 insert. jobQ 에서 delete.
            // jobQ에서 삭제된 process index 를 채우기 위해 한칸 씩 이동했으므로 i 를 증가할 필요 없다.
            insert_readyQ(jobQ[i]);
            delete_jobQ(jobQ[i]);
        }
        else{
            // i 번째 프로세스가 readyQ 에 들어가지 않은 경우 다음 프로세스를 탐색하기 위해 i 를 1 증가함.
            i++;
        }
    }
}

// job scheduling for multilevel feedback Q
void jobScheduling_MFQ(int curTime){
    int criteria = (NUM_PROCESS+6)/2;
    for(int i=0; i<curProcNumJobQ;){
        if(jobQ[i]->arrivalTime == curTime){
            if(jobQ[i]->priority<=criteria){ // priority <= criteria 면 더 우선순위가 높은 Q에 넣어줌.
                insert_foreQ(jobQ[i]);
                delete_jobQ(jobQ[i]);
            }
            else{ // priority > criteria 면 더 우선순위가 낮은 Q에 넣어줌.
                insert_backQ(jobQ[i]);
                delete_jobQ(jobQ[i]);
            }
        }
        else{
            i++;
        }
    }
    return;
}

int selectReadyQ_MFQ(){
    int criteria = rand() % 1;
    if(curProcNumForeQ == 0 && curProcNumBackQ ==0) // 모두 비어있는 경우
        return -1;
    else if (curProcNumForeQ == 0 && curProcNumBackQ !=0)
        return BACKGROUND;
    else if (curProcNumForeQ != 0 && curProcNumBackQ ==0)
        return FOREGROUND;
    else{ // 둘 다 있는 경우
        if(criteria<=0.8){ // select foreQ
            return FOREGROUND;
        }
        return BACKGROUND;
    }
}

int selectReadyQ_MFQ_process(processPtr myprocess){
    int criteria = (NUM_PROCESS+6)/2;
    if(myprocess->priority<=criteria)
        return FOREGROUND;
    else
        return BACKGROUND;
}

void aging_MFQ(){
    for(int i=0; i<curProcNumBackQ;){
        if(backQ[i]->responseTime > 10){
            insert_foreQ(backQ[i]);
            delete_backQ(backQ[i]);
        }
        else{
            i++;
        }
    }
}

void prmPriority_MFQ_algo(){
    selectedReadyQ = selectReadyQ_MFQ(); // 어떤 readyQ 에서 가져올 것인지 선택.
    
    aging_MFQ();
    
    if(selectedReadyQ == FOREGROUND){
        init_readyQ();
        load_from_foreQ(readyQ);
    }
    else if(selectedReadyQ == BACKGROUND){
        init_readyQ();
        load_from_backQ(readyQ);
    }
    else{ // foreQ, readyQ 가 모두 비어있는 경우..
        init_readyQ();
    }
    
    // same as prm priority!!!
    if(curProcNumReadyQ==0){
        // nothing to CPUschedule
        return;
    }
    else{
        // there is some processes in readyQ to schedule
        
        processPtr minPriorityProc[MAX_NUM_PROCESS];
        // initializing
        for(int i=0; i<MAX_NUM_PROCESS; i++){
            minPriorityProc[i] = NULL;
        }
        
        // find minPriority
        int minPriority = readyQ[0]->priority;
        for(int i=0; i<curProcNumReadyQ; i++){
            if(minPriority > readyQ[i]->priority)
                minPriority = readyQ[i]->priority;
        }
        
        // priority 가 같을 경우, pid 가 작은 process 부터 runQ에 넣을 것임.
        int j = 0; // j 는 minpriority 를 가진 process 의 수
        for(int i=0; i<curProcNumReadyQ; i++){
            if(readyQ[i]->priority == minPriority){
                minPriorityProc[j] = (processPtr)malloc(sizeof(struct process));
                memcpy(minPriorityProc[j], readyQ[i], sizeof(struct process));
                j++;
            }
        }
        
        if(j!=0){
            int minPid = MAX_NUM_PROCESS;
            for(int i=0; i<j; i++){
                if(minPid > minPriorityProc[i]->pid){
                    minPid = minPriorityProc[i]->pid;
                }
            }
            
            for(int i=0; i<j;){
                if(minPid == minPriorityProc[i]->pid){
                    if(curProcNumRunQ!=0){ // cpu 가 idle 하지 않다면 minPriorityProc[i] 와 runQ[curProcNumRunQ-1] 의 priority 를 비교.
                        if(minPriority >= runQ[curProcNumRunQ-1]->priority){
                            // preemption 적용 못함.
                            break;
                        }
                        else{ // preemption 적용 함.
                            insert_readyQ(runQ[curProcNumRunQ-1]);
                            if(selectReadyQ_MFQ_process(runQ[curProcNumRunQ-1]) == FOREGROUND){
                                insert_foreQ(runQ[curProcNumRunQ-1]);
                            }
                            else if(selectReadyQ_MFQ_process(runQ[curProcNumRunQ-1]) == BACKGROUND){
                                insert_backQ(runQ[curProcNumRunQ-1]);
                            }
                            init_runQ();
                            insert_runQ(minPriorityProc[i]);
                            delete_readyQ(minPriorityProc[i]);
                            if(selectedReadyQ == FOREGROUND){
                                delete_foreQ(minPriorityProc[i]);
                            }
                            else if(selectedReadyQ == BACKGROUND){
                                delete_backQ(minPriorityProc[i]);
                            }
                        }
                        break;
                    }
                    else{ // cpu 가 idle 하다면, minPriorityProc[i] 를 run 시켜야 함.
                        insert_runQ(minPriorityProc[i]);
                        delete_readyQ(minPriorityProc[i]);
                        if(selectedReadyQ == FOREGROUND){
                            delete_foreQ(minPriorityProc[i]);
                        }
                        else if(selectedReadyQ == BACKGROUND){
                            delete_backQ(minPriorityProc[i]);
                        }
                        break;
                    }
                }
                else{
                    i++;
                }
            }
        }
    }
}


void CPUScheduling(int algo, int curTime){
    switch (algo) {
        case FCFS:
            FCFS_algo(curTime);
            break;
            
        case PRM_SJF:
            prmSJF_algo();
            break;
            
        case NONPRM_SJF:
            nonprmSJF_algo();
            break;
            
        case PRM_PRIORITY:
            prmPriority_algo();
            break;
            
        case NONPRM_PRIORITY:
            nonprmPriority_algo();
            break;
            
        case RR:
            RR_algo();
            break;
            
        case PRM_FCFS:
            prmFCFS_algo(curTime);
            break;
            
        case RR_PRI:
            RR_priority_algo();
            break;
            
        case RR_SJF:
            RR_SJF_algo();
            break;
            
        case PRM_PRIORITY_MFQ:
            prmPriority_MFQ_algo();
            break;
            
            
        default:
            printf("---- ERROR : Selected algorithm does not exist ----\n");
            return;
    }
}


// 도착한 프로세스가 1개 있는 상황. cpuburst 는 끝남. IO start 안함. run -> ready 로 가야 함.
// 이때, ready 로 간 경우에는

bool IOstart(){
    // IO remainingTime > 0 인 runningprocess 에 대해서 check 해야 함.
    bool needCPUschedule = false;
    
    // runQ -> waitingQ 이므로
    if(curProcNumRunQ!=0){
        if(runQ[curProcNumRunQ-1]->IOremainingTime>0 && runQ[curProcNumRunQ-1]->IOstartTime <= myTime &&
           runQ[curProcNumRunQ-1]->CPUburst > runQ[curProcNumRunQ-1]->CPUremainingTime){ // 실행해야 할 IO 가 있다면
            // IO start 되었고, cpu를 1개라도 실행한 경우 waitingQ 에 넣어야 함.
            insert_waitingQ(runQ[curProcNumRunQ-1]);
            init_runQ(); // runQ 가 비었기 때문에 다시 cpuScheduling 을 해야함.
            needCPUschedule = true;
        }
    }
    return needCPUschedule;
}

bool IOcompletion(int algo){
    bool needCPUschedule = false;
    // waitingQ -> readyQ
    if(curProcNumWaitingQ!=0){
        for(int i=0; i<curProcNumWaitingQ;){
            if(waitingQ[i]->IOremainingTime == 0){
                insert_readyQ(waitingQ[i]);
                if(algo == PRM_PRIORITY_MFQ){
                    if(selectReadyQ_MFQ_process(waitingQ[i]) == FOREGROUND){
                        insert_foreQ(waitingQ[i]);
                    }
                    else if(selectReadyQ_MFQ_process(waitingQ[i]) == BACKGROUND){
                        insert_backQ(waitingQ[i]);
                    }
                }
                delete_waitingQ(waitingQ[i]);
                needCPUschedule = true; // readyQ 가 바뀌었기 때문에 다시 cpuScheduling을 해야 한다.
            }
            else{
                i++;
            }
        }
    }
    
    return needCPUschedule;
}

bool terminate(int algo){
    bool isTerminated = false;
    // runQ -> terminatedQ 이기 때문에
    if(curProcNumRunQ != 0){
        if(runQ[curProcNumRunQ-1]->CPUremainingTime == 0 && runQ[curProcNumRunQ-1]->IOremainingTime == 0){
            // 실행을 다 끝내면, terminated.
            isTerminated = true;
            insert_terminatedQ(runQ[curProcNumRunQ-1]); // add the process to terminatedQ
            printf("%2d : pid %d - terminated\n", myTime, runQ[curProcNumRunQ-1]->pid);
            init_runQ(); // delete the running process from runQ
            // runQ 가 비었으니까 다시 cpuSchedule, schedule 된 runQ 가 IO 시작했다면 다시 schedule.
            // 시간이 흐르지 않았으므로 IO completion을 살필 필요는 없음.
            do{
                CPUScheduling(algo, myTime);
            }while(IOstart());
            
            if(curProcNumRunQ!=0)
                printf("     pid %d - run\n", runQ[curProcNumRunQ-1]->pid);
            else{
                printf("     CPU idle\n");
                cpuIdleTime++;
            }
        }
        else{
            printf("%2d : pid %d - run\n", myTime, runQ[curProcNumRunQ-1]->pid);
        }
    }
    else{
        printf("%2d : CPU idle\n", myTime);
        cpuIdleTime++;
    }
    return isTerminated;
}




void simulation(int algo){
    
    // 알고리즘 시작..!
    init_jobQ(); // job Q 를 초기화함.
    load_from_backupQ(jobQ); // backupQ 에서 load 함. => jobQ 준비.
    init_readyQ(); // ready Q 를 초기화함.
    init_runQ();
    init_waitingQ();
    init_terminatedQ();
    init_foreQ();
    init_backQ();
    
    // 시간 시작.
    myTime = 0;
    cpuIdleTime = 0;
    ellapsedTime = 0;
    // jobQ에 아무것도 없을 때까지 반복. jobQ 에 있는 process의 arrival time 을 확인하여, readyQ 에 넣는다
    
    // myTime 을 증가시키기 전에 확인해야 하는 것.
    // 1. JobScheduling : jobQ -> readyQ
    // 2. CPUScheduling : readyQ -> runQ
    // 3. IO starts? : runQ -> waitngQ
    // 4. IO completion? : waitingQ -> readyQ
    // 5. Time expired or preemption? : runQ -> readyQ
    // 6. terminated? : runQ -> terminatedQ
    
    // 모든 프로세스가 terminate 될 때 까지 반복.
    while (curProcNumTerminatedQ != NUM_PROCESS) {
        // 1. jobScheduling : 현재 myTime 에 도착한 process가 있다면 readyQ에 넣어준다.
        if(algo == PRM_PRIORITY_MFQ)
            jobScheduling_MFQ(myTime);
        else
            jobScheduling(myTime);
        
        // 2. CPUScheduling
        CPUScheduling(algo, myTime);
        
        // 3. IO starts?
        if(IOstart())
            continue;
        
        // 4. IO completion?
        if(IOcompletion(algo))
            continue;
        
        // 5. time expired? or preemption? runQ->readyQ
        // 각각의 알고리즘마다 다를듯
        
        // 6. terminated : runQ 에 있는 프로세스의 burst time 이 모두 끝나면 terminated..
        terminate(algo);
        // 아직 시간이 흐르기 전이므로, IO completion 될 것은 없음.
        
        // 시간 증가
        
        if(curProcNumRunQ!=0 && runQ[curProcNumRunQ-1]->CPUremainingTime > 0){
            runQ[curProcNumRunQ-1]->CPUremainingTime--;
            runQ[curProcNumRunQ-1]->turnaroundTime++;
            ellapsedTime++;
        }
        
        myTime++;
        
        if(curProcNumWaitingQ!=0){
            for(int i=0; i<curProcNumWaitingQ; i++){
                waitingQ[i]->IOremainingTime--;
                waitingQ[i]->turnaroundTime++;
                printf("     pid %d - waiting \n", waitingQ[i]->pid);
            }
        }
        
        if(curProcNumReadyQ!=0){
            for(int i=0; i<curProcNumReadyQ; i++){
                readyQ[i]->waitingTime++;
                readyQ[i]->turnaroundTime++;
                if(readyQ[i]->CPUburst==readyQ[i]->CPUremainingTime && readyQ[i]->IOburst==readyQ[i]->IOremainingTime)
                    readyQ[i]->responseTime++;
            }
        }
        
        if(curProcNumForeQ!=0){
            for(int i=0; i<curProcNumForeQ; i++){
                foreQ[i]->waitingTime++;
                foreQ[i]->turnaroundTime++;
                if(foreQ[i]->CPUburst==foreQ[i]->CPUremainingTime && foreQ[i]->IOburst==foreQ[i]->IOremainingTime)
                    foreQ[i]->responseTime++;
            }
        }
        
        if(curProcNumBackQ!=0){
            for(int i=0; i<curProcNumBackQ; i++){
                backQ[i]->waitingTime++;
                backQ[i]->turnaroundTime++;
                if(backQ[i]->CPUburst==backQ[i]->CPUremainingTime && backQ[i]->IOburst==backQ[i]->IOremainingTime)
                    backQ[i]->responseTime++;
            }
        }
        
        if(curProcNumTempQ!=0){
            for(int i=0; i<curProcNumTempQ; i++){
                tempQ[i]->waitingTime++;
                tempQ[i]->turnaroundTime++;
            }
        }
        
        //print_foreQ();
        //print_backQ();
        
        // waiting time : arrival time 부터 끝날 때 까지 readyQ 에 있었던 시간.
        // response time : arrival time 이후 (readyQ에 들어온 이후) 처음 실행될때 까지 걸린 시간.
        // turnaround time : arrival time 부터 terminate 되기 전까지.
        
    }
}


void evaluate(int algo){
    printf("\n=== EVALUATE ===\n\n");
    
    double tmp = 0;
    int i = 0;
    
    printf("pid\twaitingTime\tturnaroundTime\tresponseTime\n");
    
    for(i=0; i<NUM_PROCESS; i++){
        printf("%2d\t\t%2d\t\t\t%2d\t\t\t%2d\n", terminatedQ[i]->pid, terminatedQ[i]->waitingTime, terminatedQ[i]->turnaroundTime, terminatedQ[i]->responseTime);
    }
    
    printf("\n");
    
    tmp = 0;
    for(i=0; i<NUM_PROCESS; i++){
        tmp += terminatedQ[i]->waitingTime;
    }
    avgWaitingTime[algo-1] = (double)tmp/NUM_PROCESS;
    tmp = 0;
    
    for(i=0; i<NUM_PROCESS; i++){
        tmp += terminatedQ[i]->turnaroundTime;
    }
    avgTurnaroundTime[algo-1] = (double)tmp/NUM_PROCESS;
    tmp = 0;
    
    for(i=0; i<NUM_PROCESS; i++){
        tmp += terminatedQ[i]->responseTime;
    }
    avgResponseTime[algo-1] = (double)tmp/NUM_PROCESS;
    tmp = 0;
    
    utilization[algo-1] = (double)(myTime-cpuIdleTime)/myTime;
    
    printf("Average Waiting Time : %.3f\n", avgWaitingTime[algo-1]);
    printf("Average Turnaround Time : %.3f\n", avgTurnaroundTime[algo-1]);
    printf("Average Response Time : %.3f\n", avgResponseTime[algo-1]);
    printf("Utilization : %.2f%%\n", utilization[algo-1]*100);
    return;
}


void selectingAlgo(int algo){
    switch (algo) {
        case FCFS:
            printf("=== FCFS algoritnm ===\n\n");
            break;
            
        case PRM_SJF:
            printf("=== preemptive SJF algoritnm ===\n\n");
            break;
            
        case NONPRM_SJF:
            printf("=== nonpreemptive SJF algoritnm ===\n\n");
            break;
            
        case PRM_PRIORITY:
            printf("=== preemptive priority algoritnm ===\n\n");
            break;
            
        case NONPRM_PRIORITY:
            printf("=== nonpreemptive priority algoritnm ===\n\n");
            break;
            
        case RR:
            printf("=== Round Robin algoritnm ===\n\n");
            break;
            
        case PRM_FCFS:
            printf("=== preemptive FCFS algoritnm ===\n\n");
            break;
            
        case RR_PRI:
            printf("=== RR priority algoritnm ===\n\n");
            break;
            
        case RR_SJF:
            printf("=== RR SJF algoritnm ===\n\n");
            break;
            
        case PRM_PRIORITY_MFQ:
            printf("=== preemptive priority MFQ algoritnm ===\n\n");
            break;
            
        default:
            printf("---- ERROR : Selected algorithm does not exist ----\n");
            return;
    }
    
    simulation(algo);
    
}


// -------------------- creating process START --------------------
processPtr createProcess(int pid, int CPUburst, int IOburst, int arrivalTime, int priority, int IOstartTime){
    processPtr newProcess = (processPtr)malloc(sizeof(struct process));
    newProcess->pid = pid;
    newProcess->CPUburst = CPUburst;
    newProcess->IOburst = IOburst;
    newProcess->CPUremainingTime = CPUburst;
    newProcess->IOremainingTime = IOburst;
    newProcess->arrivalTime = arrivalTime;
    newProcess->priority = priority;
    newProcess->IOstartTime = IOstartTime;
    newProcess->waitingTime = 0;
    newProcess->turnaroundTime = 0;
    newProcess->responseTime = 0;
    
    // insert processes into jobQ
    insert_jobQ(newProcess);
    
    return newProcess;
}

void createProcesses(int numProcess){
    for(int i=0; i<numProcess; i++){
        // int pid, int CPUburst, int IOburst, int arrivalTime, int priority, int IO start Time
        // CPUburst : 5~15, IOburst : 3~10, arrivalTime : 0~numprocess+10, priority : 1~numprocess+5, IOstartTime : arrivalTime + 1~4
        int CPUburst = rand() % 11 + 5;
        int IOburst = rand() % 8 + 3;
        int arrivalTime = rand() % (numProcess+11);
        int priority = rand() % (numProcess+5) + 1;
        int IOstartTime = rand() % 4 + 1 + arrivalTime;
        printf("pid %d, CPUburst %2d, IOburst %2d, arrivalTime %2d, priority %2d, IOstartTime %2d\n", i+1, CPUburst, IOburst, arrivalTime, priority, IOstartTime);
        createProcess(i+1, CPUburst, IOburst, arrivalTime, priority , IOstartTime);
    }
}
// -------------------- creating process END --------------------


int main(int argc, const char * argv[]) {
    // process 생성 - run 상태. JobQ 에 넣기.
    // 생성된 process 정보 저장.. How? (알고리즘이 선택되면 JQ 에서 RQ 로 프로세스가 빠지기 때문에 빠지기 전 정보를 따로 저장해 놓아야 한다.)
    // FCFS, preemtive_SJF, nonpreemptive_SJF, preemptive_priority, nonpreemptive_priority, RR 로 simulate
    // 알고리즘이 선택되면, JQ 에 있던 프로세스의 arrival time 에 따라 readyQ 에 넣어준다.
    // JQ 에 있던 process 가 arrival time에 따라 redayQ 로 들어가는 함수를 만들자. (job scheduler)
    // 알고리즘이 선택되는 함수를 만들어야 한다. startSimulation 이 여기서는 그 함수..
    
    // readyQ 에 있는 프로세스는 각각의 알고리즘에 따라 CPU scheduling 을 하고 실행한다.
    // 각 프로세스마다 waiting time, turnaround time, response time 을 계산해야 한다. run 될때와 run 이 아닐 때 +1 되도록 해야 함.
    // run 하고 있는 프로세스는 time 1 이 지날 때마다 Gantt Chart 를 출력해야 한다.
    // cpu 사용 시간 / 전체 시간 으로 cpu utilization 을 계산한다.
    // 각각의 알고리즘으로 시뮬레이션 한 결과를 출력. (avgWaitingTime, avgTurnaroundTime, avgResponseTime, CPUutilization, GanttChart)
    
    srand((unsigned int)time(NULL));
    createProcesses(NUM_PROCESS); // process 생성 - jobQ 에 넣기.
    //print_jobQ(); // jobQ 에 잘 생성되었는지 확인
    
    /*
     createProcess(1, 6, 5, 8, 3, 12);
     createProcess(2, 8, 7, 2, 2, 3);
     createProcess(3, 8, 10, 5, 7, 9);
     */
    
    clone_to_backupQ(jobQ); // backupQ 에 clone 하기
    //print_backupQ(); // backupQ 에 잘 clone 되었나 확인.
    printf("\n");
    
    selectingAlgo(FCFS); // selecting algorithm as FCFS. and simulate!!!!
    evaluate(FCFS);
    printf("\nFCFS DONE\n\n");
    
    selectingAlgo(PRM_FCFS); // selecting algorithm as FCFS. and simulate!!!!
    evaluate(PRM_FCFS);
    printf("\nPRM_FCFS DONE\n\n");
    
    selectingAlgo(PRM_SJF);
    evaluate(PRM_SJF);
    printf("\nPRM_SJF DONE\n\n");
    
    selectingAlgo(NONPRM_SJF);
    evaluate(NONPRM_SJF);
    printf("\nNONPRM_SJF DONE\n\n");
    
    selectingAlgo(PRM_PRIORITY);
    evaluate(PRM_PRIORITY);
    printf("\nPRM_PRIORITY DONE\n\n");
    
    selectingAlgo(NONPRM_PRIORITY);
    evaluate(NONPRM_PRIORITY);
    printf("\nNONPRM_PRIORITY DONE\n\n");
    
    selectingAlgo(RR);
    evaluate(RR);
    printf("\nRR DONE\n\n");
    
    selectingAlgo(RR_PRI);
    evaluate(RR_PRI);
    printf("\nRR_PRI DONE\n\n");
    
    selectingAlgo(RR_SJF);
    evaluate(RR_SJF);
    printf("\nRR_SJF DONE\n\n");
    
    selectingAlgo(PRM_PRIORITY_MFQ);
    evaluate(PRM_PRIORITY_MFQ);
    printf("\nPRM_PRIORITY_MFQ DONE\n\n");
    
    clear_jobQ();
    clear_runQ();
    clear_tempQ();
    clear_readyQ();
    clear_backupQ();
    clear_foreQ();
    clear_backQ();
    
    return 0;
}
