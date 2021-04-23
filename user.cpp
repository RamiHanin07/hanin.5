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

struct pages{
    int address;
    int secondChance;
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
    pages pageTable[32];
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
    bool mesg_terminateNow;
    bool mesg_terminated;
    bool mesg_typeOfSystem;
    bool mesg_blocked;
    int mesg_unblockNS;
    int mesg_unblockSec;
    int mesg_rqIndex;
    int mesg_ptNumber;
    bool mesg_isItRead;
} message;

struct frame{
    int pid;
    int dirtyBit;
    int index;
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
    else if(signal == 1)
        cout << "Process Terminated" << endl;

    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    exit(signal);
}

int main(int argc, char* argv[]){
    // cout << "user.cpp" << endl;
    signal(SIGINT, signalHandler);
    signal(SIGALRM, signalHandler);
    struct simClock *clock;
    struct processes *pTable;
    int LEN = 18;
    int size = sizeof(pTable) * LEN;
    int loops = 0;
    int ptMax = 32000;
    int terminate;
    int totalTimeTilExpire = 5;
    alarm(totalTimeTilExpire);
    srand(getpid());



    //Shared Memory Creation for System Clock (Seconds)
    int sizeMem = 1024;
    key_t keyClock = 6666;

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
    key_t keyProc = 7777;

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

    // for(int i = 0; i < 18; i++){
    //     cout << pTable[i].pid << " ; pid " << i << endl;
    // }

    // cout << clock->nano << " user process nano " << endl;
    // cout << clock->sec << " user process sec " << endl;
    //Set Up Message Queue
    int msgid;
    int msgidTwo;
    key_t messageKey = ftok("pog", 67);
    key_t messageKeyTwo = ftok("home",68);
    msgid = msgget(messageKey, 0666|IPC_CREAT);
    msgidTwo = msgget(messageKeyTwo, 0666|IPC_CREAT);
    

    int ptNumber;
    int chanceForRead = 70;
    int chanceforOutOfBounds = 5;
    int isItOutOfBounds;
    int isItRead;
    int chanceToTerminate = 50;
    int didItTerminate;
    static int outOfOneHund = 100;
    terminate = false;
    while(terminate == false){
        // cout << "loops: " << loops << endl;
        
        strcpy(message.mesg_text, "Message Received");
        ptNumber = rand()%((ptMax - 1)+1);
        isItRead = rand()%((outOfOneHund - 1)+1);
        if(isItRead < chanceForRead){
            //1 for it being read, 0 for it being write.
            message.mesg_isItRead = 1;
        }else{
            message.mesg_isItRead = 0;
        }
        isItOutOfBounds = rand()%((outOfOneHund - 1)+1);
        if(isItOutOfBounds < chanceforOutOfBounds){
            while(ptNumber <= ptMax){
                ptNumber += 1000;
            }
        }
        message.mesg_pid = getpid();
        message.mesg_ptNumber = ptNumber;
        // cout << ptNumber << " user ptNumber" << endl;
        message.mesg_type = 1;
        msgsnd(msgid, &message, sizeof(message), 0);
        // cout << "after user message send" << endl;
        message.mesg_type = getpid();
        msgrcv(msgidTwo, &message, sizeof(message), message.mesg_type, 0);
        // cout << "after user message rcv" << endl;
        // cout << message.mesg_terminate << " ; message terminate" <<endl;
        if(loops > 5){
            didItTerminate = rand()%((outOfOneHund - 1)+1);
            if(didItTerminate < chanceToTerminate){
                // cout << "enter chance to terminate" << endl;
                terminate = true;
            }
        }
            // cout << terminate << " var terminate" << endl;
        if(message.mesg_terminateNow == true){
            // cout << "enter mesg term is true" << endl;
            message.mesg_terminateNow = terminate;
        }
        loops++;
    };
    // cout << terminate << " var terminate outside" << endl;
    // cout << "process dies" << endl;
    // cout << message.mesg_terminateNow << " this should be 1" << endl;
    message.mesg_terminated = 1;
    message.mesg_type = 1;
    msgsnd(msgid, &message, sizeof(message), 0);
    return 0;
}