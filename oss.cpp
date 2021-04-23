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
    int mesg_isItRead;
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

int frameTableAccess(frame *frameTable, int ptNum, int pid, int read);

void displayFrameTable(frame *frameTable);

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
    ofstream log("log.txt");
    log.close();
    int loops = 0;
    int i = 0;
    int interval = 0;
    int billion = 100000000;
    int maxSystemTimeSpent = 15;
    const int maxTimeBetweenNewProcsNS = 100;
    const int maxTimeBetweenNewProcsSecs = 1;



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

    for(int i = 0 ; i < 256; i++){
        frameTable[i].pid = -1;
        frameTable[i].dirtyBit = 0;
    }
    // for(int i = 0; i < 18; i++){
    //     cout << pTable[i].pid << " ; pid " << i << endl;
    // }


    //Generate child process
    char buffer[50] = "";
    char fileName[10] = "test";
    cout << "before fork" << endl;
    //strcpy(buffer, fileName);
    for(int i = 0; i < 2; i++){
        if(fork() == 0){
            cout << "enter fork" << endl;
            interval = rand()%((maxTimeBetweenNewProcsSecs - 1)+1);
            clock->sec+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };
            pTable[i].pid = getpid();
            pTable[i].timeStartedSec = clock->sec;
            pTable[i].timeStartedNS = clock->nano;
            log.open(fileName,ios::app);
            log << "OSS: Generating process with PID " << getpid() << " at time: " << clock->sec << " s : " << clock->nano << "ns \n";
            log.close();
            execl("./user", buffer);
        }
    }


    //Set UP Message Queues
    key_t messageKey = ftok("poggers", 65);
    msgid = msgget(messageKey, 0666|IPC_CREAT);

    while(loops < 10){
        if(msgrcv(msgid, &message, sizeof(message), 1, 0) == -1){
            perror("msgrcv");
            return 1;
        }

        interval = rand()%((maxSystemTimeSpent - 1)+1);
        clock->nano+= interval;
        while(clock->nano >= billion){
            clock->nano-= billion;
            clock->sec+= 1;
        };

        cout << message.mesg_text;
        cout << " : " << message.mesg_ptNumber << endl;
        if(message.mesg_isItRead == 1){
            log.open("log.txt", ios::app);
            log << "OSS: Process " << i << " requesting read of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
            log.close();
            frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead);
        }else
        {
            log.open("log.txt", ios::app);
            log << "OSS: Process " << i << " requesting write of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
            log.close();
            frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead);
        }
        
        loops++;
    }
    


    // displayFrameTable(frameTable);
    wait(NULL);
    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    
}

int frameTableAccess(frame *frameTable, int ptNum, int pid, int read){
    int frameIndex = ptNum / 1024;
    // cout << ptNum << " ; ptNum " << endl;
    // cout << frameIndex << " ; frameIndex " << endl;
    if(frameTable[frameIndex].pid == -1){
        frameTable[frameIndex].pid = pid;
    }
    else
    {
        //If the frameTable already is full, what do.
    }
    if(read == 0){
        frameTable[frameIndex].dirtyBit = 1;
    }
    // cout << frameTable[frameIndex].pid << " pid at frame table index " << frameIndex << endl;
}

void displayFrameTable(frame *frameTable){
    for(int i = 0; i < 256; i++){
        cout << "Index " << i << ": pid: " << frameTable[i].pid << " dirty bit: " << frameTable[i].dirtyBit << endl;
    }
}

