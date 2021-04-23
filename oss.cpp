#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <csignal>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <string>
#include <time.h>

using namespace std;

struct simClock{
    int sec;
    int nano;
};

struct processes{
    int pid;
    int timeStartedSec;
    int timeStartedNS;
    int totalCPUTime;
    int totalTimeSystem;
    int lastBurst;
    int processPrio;
    bool typeOfSystem;
    int blockRestartSec;
    int blockRestartNS;
    bool unblocked;
    int timeInReadySec;
    int timeInReadyNS;
    int pageTable[32];
};

struct mesg_buffer{
    long mesg_type;
    char mesg_text[100];
    int mesg_pid;
    int mesg_totalCPUTime;
    int mesg_totalTimeSystem;
    int mesg_lastBurst;
    int mesg_processPrio;
    int mesg_timeQuant;
    int mesg_timeUsed;
    bool mesg_terminated;
    bool mesg_typeOfSystem;
    bool mesg_blocked;
    int mesg_unblockNS;
    int mesg_unblockSec;
    int mesg_rqIndex;
    int mesg_ptNumber;
} message;

struct frame{
    int pid;
    int dirtyBit;
};

int shmidClock;
int shmidProc;
int msgid;
int msgidTwo;

void signalHandler(int signal);

void signalHandler(int signal){

    //Basic signal handler
    if(signal == 2)
        cout << "Interrupt Signal Received" <<endl;
    else if(signal == 20)
        cout << "Exceeded Time, Terminating Program" <<endl;

    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    exit(signal);
}

int main(int argc, char* argv[]){
    struct processes *pTable;
    struct simClock *clock;
    struct frame frameTable[256];
    signal(SIGINT, signalHandler);
    signal(SIGALRM, signalHandler);
    int LEN = 18;
    int size = sizeof(pTable) * LEN;



    int sizeMem = 1024;
    key_t keyClock = 786575;


    //Shared Memory Creation for System Clock (Seconds)
    shmidClock = shmget(keyClock, sizeof(struct simClock), 0644|IPC_CREAT);
    if (shmidClock == -1) {
        perror("Shared memory");
        return 1;
    }

    clock = (simClock*)shmat(shmidClock,NULL,0);
    if(clock == (void*) -1){
        perror("Shared memory attach");
        return 1;
    }

    //Shared Memory Creation for Process Table 
    key_t keyProc = 76589;

    shmidProc = shmget(keyProc, sizeof(struct processes), 0644|IPC_CREAT);
    if (shmidProc == -1) {
        perror("Shared memory");
        return 1;
    }

    pTable = (processes*)shmat(shmidProc,NULL,0);
    if(pTable == (void*) -1){
        perror("Shared memory attach");
        return 1;
    }

    for(int i  = 0; i < 18; i++){
        pTable[i].pid = -1;
    }

    // for(int i = 0; i < 18; i++){
    //     cout << pTable[i].pid << " ; pid " << i << endl;
    // }

    clock->sec += 5;
    clock->nano += 500;

    //Generate child process
    char buffer[50] = "";
    char fileName[10] = "test";
    cout << "before fork" << endl;
    //strcpy(buffer, fileName);
    if(fork() == 0){
        cout << "enter fork" << endl;
        execl("./user", buffer);
    }


    //Set UP Message Queues
    key_t messageKey = ftok("poggers", 65);
    msgid = msgget(messageKey, 0666|IPC_CREAT);

    int loops = 0;

    while(loops < 5){
        if(msgrcv(msgid, &message, sizeof(message), 1, 0) == -1){
            perror("msgrcv");
            return 1;
        }

        cout << message.mesg_text;
        cout << " : " << message.mesg_ptNumber << endl;
        loops++;
    }
    


    wait(NULL);
    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    
}