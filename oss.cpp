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
#include <bits/stdc++.h>

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
    int index;
    int dirtyBit;
    int secondChance;
};

int shmidClock;
int shmidProc;
int msgid;
int msgidTwo;

void signalHandler(int signal);

int frameTableAccess(vector<frame> &frameTable, int ptNum, int pid, int read, int &interval);

void displayFrameTable(vector<frame> &frameTable);

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
    // struct frame frameTable[256];
    cout << "Before vector" <<endl;
    vector<frame> frameTable;
    cout << "after vector" << endl;
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
    int overWritePid = 0;
    srand(time(NULL));



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
    // cout << "before frameTable set" << endl;
    // for(int i = 0 ; i < 256; i++){
    //     frameTable.at(i).pid = -1;
    //     // frameTable[i].pid = -1;
    //     // frameTable[i].dirtyBit = 0;
    // }
    cout << "after frameTable set" << endl;
    // for(int i = 0; i < 18; i++){
    //     cout << pTable[i].pid << " ; pid " << i << endl;
    // }


    //Generate child process
    char buffer[50] = "";
    char fileName[10] = "test";
    cout << "before fork" << endl;
    //strcpy(buffer, fileName);
    for(int i = 0; i < 2; i++){
        interval = rand()%((maxTimeBetweenNewProcsNS - 1)+1);
        cout << interval << " interval" << endl;
        if(fork() == 0){
            cout << "enter fork" << endl;
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };

            cout << clock->sec << " main process sec" << endl;
            cout << clock->nano << " main process nano" << endl;
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
        overWritePid = 0;
        if(msgrcv(msgid, &message, sizeof(message), 1, 0) == -1){
            perror("msgrcv");
            return 1;
        }

        // cout << msgrcv(msgid, &message, sizeof(message), 1, 0) << " message queue return " << endl;

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
            log << "OSS: Process " << message.mesg_pid << " requesting read of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
            log.close();
            overWritePid = frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead, interval);
            
            
        }else
        {
            log.open("log.txt", ios::app);
            log << "OSS: Process " << message.mesg_pid << " requesting write of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
            log.close();
            overWritePid = frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead, interval);
        }
        
        cout << interval << " ; interval" << endl;
        clock->nano+= interval;
        while(clock->nano >= billion){
            clock->nano-= billion;
            clock->sec+= 1;
        };
        
        //If the frame was overwritten
        if(overWritePid != 0){
            interval = rand()%((maxSystemTimeSpent - 1)+1);
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };
            log.open("log.txt", ios::app);
            log << "OSS: Process: " << overWritePid << "'s frame in frame table was overwritten by process: " << message.mesg_pid << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
            log.close();
        }

        //Set page table of process

        for(int i = 0; i < 18; i++){
            if(pTable[i].pid == message.mesg_pid){
                // cout << "pid found at index " << i << endl;
                pTable[i].pageTable[message.mesg_ptNumber/1024].address = message.mesg_ptNumber;
                // cout << pTable[i].pageTable[message.mesg_ptNumber/1024] << " ptNumber stored in pageTable index: " << (message.mesg_ptNumber/1024) << endl;
            }
        }
        loops++;
    }
    


    displayFrameTable(frameTable);
    wait(NULL);
    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    
}

int frameTableAccess(vector <frame> &frameTable, int ptNum, int pid, int read, int &interval){
    int pageIndex = ptNum / 1024;
    int overWritePid;
    bool pageFault = true;
    ofstream log("log.txt", ios::app);
    log.close();
    interval = 0;
    int billion = 100000000;
    int maxSystemTimeSpent = 15;
    const int maxTimeBetweenNewProcsNS = 100;
    const int maxTimeBetweenNewProcsSecs = 1;
    // cout << ptNum << " ; ptNum " << endl;
    // cout << frameIndex << " ; frameIndex " << endl;
    
    //If frameTable hasn't exceeded its maximum amount
    if(frameTable.size() < 256){
        // +cout << "enter if" << endl;
        //If the frameTable isn't empty.
        if(frameTable.size() > 0){
            for(int i = 0; i < frameTable.size(); i++){
                if(frameTable[i].pid == pid && frameTable[i].index == pageIndex){
                    //Has been accessed recently, increment second chance!
                    frameTable[i].secondChance = 1;
                    if(read == 0){
                        frameTable[i].dirtyBit = 1;
                        log.open("log.txt", ios::app);
                        log << "OSS: Dirty bit of frame " << i << " set, adding additional time to the clock" << endl;
                        log.close();
                        interval = rand()%((maxSystemTimeSpent - 1)+1);
                    }
                    else
                    {
                        frameTable[i].dirtyBit = 0;
                    }
                    // cout << "page already exists in frametable" << endl;
                    pageFault = false;
                }
            }
            if(pageFault == true){
                // cout << "page fault!" << endl;
                frameTable.push_back(frame());
                frameTable.back().pid = pid;
                frameTable.back().index = pageIndex;
                if(read == 0){
                    frameTable.back().dirtyBit = 1;
                    log << "OSS: Dirty bit of frame " << frameTable.size() << " set, adding additional time to the clock" << endl;
                    log.close();
                    interval = rand()%((maxSystemTimeSpent - 1)+1);
                }
                else
                {
                    frameTable.back().dirtyBit = 0;
                }

                // cout << frameTable.back().pid << " new element pid" << endl;
                // cout << frameTable.back().index << " new element index" << endl;
            }
        }
        else
        {
            frameTable.push_back(frame());
            frameTable.back().pid = pid;
            frameTable.back().index = pageIndex;
        }
    }
    // if(read == 0){
    //     frameTable[frameIndex].dirtyBit = 1;
    // }
    // if(frameTable[frameIndex].pid == -1){
    //     frameTable[frameIndex].pid = pid;
    // }
    // else
    // {
    //     overWritePid = frameTable[frameIndex].pid;
    //     frameTable[frameIndex].pid = pid;
    //     return overWritePid;
    //     //If the frameTable already is full, what do.
    // }
    return 0;
    // // cout << frameTable[frameIndex].pid << " pid at frame table index " << frameIndex << endl;
}

void displayFrameTable(vector<frame> &frameTable){
    for(int i = 0; i < frameTable.size(); i++){
        cout << "Index " << i << ": pid: " << frameTable[i].pid << " dirty bit: " << frameTable[i].dirtyBit << " second chance: " <<frameTable[i].secondChance << endl;
    }
}

