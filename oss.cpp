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
    bool blocked;
    bool terminated;
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
vector<frame> frameTable;

void signalHandler(int signal);

int frameTableAccess(vector<frame> &frameTable, int ptNum, int pid, int read, int &interval, bool &blocked);

void displayFrameTable(vector<frame> &frameTable);

int checkBlockedQueue(simClock *clock, processes *blockedQueue, processes *pTable);

bool secondChance(simClock *clock, vector<frame> &frameTable);

void signalHandler(int signal){

    //Basic signal handler
    if(signal == 2)
        cout << "Interrupt Signal Received" <<endl;
    else if(signal == 20){
        cout << "Exceeded Time, Terminating Program" <<endl;
        displayFrameTable(frameTable);
    }

    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    exit(signal);
}

int main(int argc, char* argv[]){
    struct processes *pTable;
    struct simClock *clock;
    struct processes blockedQueue[18];
    bool blocked = false;
    const int blockRestartSecMax = 1;
    const int blockRestartNSMax = 1000;
    int blockRestartSec = rand()%((blockRestartSecMax - 1)+1);
    int blockRestartNS = rand()%((blockRestartNSMax - 1)+1);
    // struct frame frameTable[256];
    // cout << "Before vector" <<endl;
    // vector<frame> frameTable;
    // cout << "after vector" << endl;
    signal(SIGINT, signalHandler);
    signal(SIGALRM, signalHandler);
    int totalTimeTilExpire = 5;
    alarm(totalTimeTilExpire);
    int LEN = 18;
    int size = sizeof(pTable) * LEN;
    ofstream log("log.txt");
    log.close();
    int loops = 0;
    int i = 0;
    int interval = 0;
    int billion = 100000000;
    int maxSystemTimeSpent = 1000;
    const int maxTimeBetweenNewProcsNS = 100;
    const int maxTimeBetweenNewProcsSecs = 1;
    int overWritePid = 0;
    srand(time(NULL));



    int sizeMem = 1024;
    key_t keyClock = 6666;

    //Set UP Message Queues
    key_t messageKey = ftok("pog", 67);
    key_t messageKeyTwo = ftok("home", 68);
    msgid = msgget(messageKey, 0666|IPC_CREAT);
    msgidTwo = msgget(messageKeyTwo, 0666|IPC_CREAT); 


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

    for(int i  = 0; i < 18; i++){
        pTable[i].pid = -1;
        blockedQueue[i].pid = -1;
    }
    // cout << "before frameTable set" << endl;
    // for(int i = 0 ; i < 256; i++){
    //     frameTable.at(i).pid = -1;
    //     // frameTable[i].pid = -1;
    //     // frameTable[i].dirtyBit = 0;
    // }
    // cout << "after frameTable set" << endl;
    // for(int i = 0; i < 18; i++){
    //     cout << pTable[i].pid << " ; pid " << i << endl;
    // }


    //Generate child process
    char buffer[50] = "";
    char fileName[10] = "test";
    // cout << "before fork" << endl;
    //strcpy(buffer, fileName);
    for(int i = 0; i < 18; i++){
        interval = rand()%((maxTimeBetweenNewProcsNS - 1)+1);
        if(fork() == 0){
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };
            pTable[i].pid = getpid();
            pTable[i].timeStartedSec = clock->sec;
            pTable[i].timeStartedNS = clock->nano;
            log.open("log.txt",ios::app);
            log << "OSS: Generating process with PID " << getpid() << " at time: " << clock->sec << " s : " << clock->nano << "ns \n";
            log.close();
            execl("./user", buffer);
        }
    }

    sleep(1);    

    

    int totalProcesses = 0;
    int totalTerminated = 0;

    for(int i = 0; i < 18; i++){
        if(pTable[i].pid != -1){
            log.open("log.txt",ios::app);
            log << "OSS: Current PID: " << pTable[i].pid << endl;
            log.close();
            totalProcesses++;
        }
    }      

    int numBlocked = 0;
    while(totalTerminated < totalProcesses){
        overWritePid = 0;
        numBlocked = 0;
        secondChance(clock, frameTable);
        // cout << "before message receieve" << endl;
        for(int i = 0; i < 18; i++){
            if(pTable[i].blocked == true){
                numBlocked++;
            }
        }

        log.open("log.txt", ios::app);
        log << "Current Time : " << clock->sec << "s, " << clock->nano << "ns." << endl;
        log.close();
        // cout << numBlocked << " number of unblocked processes" << endl;
        // cout << ((totalProcesses - totalTerminated)) << " number of current alive processes" << endl;
        // log.open("log.txt",ios::app);
        // log << "OSS: Total Processes Terminated: " << totalTerminated << " and has mesg terminated value: " << message.mesg_terminated << endl;
        // log.close();

        if(message.mesg_terminated == true){
                // log.open("log.txt", ios::app);
                // log << "Entered terminated process loop " << endl;
                // log.close();
                totalTerminated++;
                // cout << message.mesg_pid << " ; terminated block pid" << endl;
                for(int i = 0; i < 18; i++){
                    // log.open("log.txt", ios::app);
                    // log << "OSS: Process " << message.mesg_pid << " message pid against pTable pid: " << pTable[i].pid <<endl;
                    // log.close();
                    if(pTable[i].pid == message.mesg_pid){
                        log.open("log.txt", ios::app);
                        log << "OSS: Process " << message.mesg_pid << " has been removed from process Table" <<endl;
                        log.close();
                        pTable[i].terminated = true;
                        pTable[i].pid = -1;
                    }
                }
                // cout << totalTerminated << " totalTerminated" << endl;
                interval = rand()%((maxSystemTimeSpent - 1)+1);
                clock->nano+= interval;
                while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;

            };
        }
        log.open("log.txt", ios::app);
        log << "numBlocked: " << numBlocked << ", current available processes: " << (totalProcesses - totalTerminated) << endl;
        log.close();
        if(numBlocked != (totalProcesses - totalTerminated)){
            if(msgrcv(msgid, &message, sizeof(message), 1, 0) == -1){
                perror("msgrcv");
                return 1;
            }

            int currentPTableIndex = -1;
            //Check all message variables
            for(int i = 0; i < 18; i++){
                if(pTable[i].pid == message.mesg_pid){
                    currentPTableIndex = i;
                    if(message.mesg_terminated == true){
                        pTable[i].terminated = true;
                        pTable[i].pid = -1;
                    }
                }
            }

            // cout << "after message receieve" << endl;

            // cout << msgrcv(msgid, &message, sizeof(message), 1, 0) << " message queue return " << endl;

            interval = rand()%((maxSystemTimeSpent - 1)+1);
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano -= billion;
                clock->sec += 1;
            };
            message.mesg_type = message.mesg_pid;
            // cout << message.mesg_ptNumber << " oss PtNUMBER" << endl;
            // cout << message.mesg_terminateNow << " OSS Terminate" << endl;
            if(message.mesg_ptNumber > 32000){
                // cout << "faulty process, terminating" << endl;
                // for(int i = 0; i < 18; i++){
                //     if(pTable[i].pid == message.mesg_pid){
                //         pTable[i].pid = -1;
                //     }
                // }
                message.mesg_terminateNow = true;
                // totalTerminated++;
                // msgsnd(msgidTwo, &message, sizeof(message), 0); Deprecated, there's a message send after the conditionals, don't need this one.
                log.open("log.txt", ios::app);
                log << "OSS: Process " << message.mesg_pid << " will terminate due to faulty memory access at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
                log << "OSS: Process " << message.mesg_pid << " has terminated status of " << message.mesg_terminated << endl;
                log.close();
            }
            else if(message.mesg_terminated == true){
                log.open("log.txt", ios::app);
                log << "OSS: Process " << message.mesg_pid << " has entered termination conditional, will be skipped" <<endl;
                log << "OSS: Process " << message.mesg_pid << " has status terminated" << endl;
                log.close();
            }
            else if(pTable[currentPTableIndex].terminated != true)
            {
                if(message.mesg_terminateNow == true){
                    log.open("log.txt", ios::app);
                    log << "OSS: Process " << message.mesg_pid << " has been told to terminate but has not " << endl;
                    log.close();
                }
                
                message.mesg_terminateNow = false;
                // cout << message.mesg_text;
                // cout << " : " << message.mesg_ptNumber << endl;
                if(message.mesg_isItRead == 1){
                    log.open("log.txt", ios::app);
                    log << "OSS: Process " << message.mesg_pid << " requesting read of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
                    log.close();
                    overWritePid = frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead, interval, blocked);
                    
                    
                }else
                {
                    log.open("log.txt", ios::app);
                    log << "OSS: Process " << message.mesg_pid << " requesting write of address " << message.mesg_ptNumber << " at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
                    log.close();
                    overWritePid = frameTableAccess(frameTable, message.mesg_ptNumber, message.mesg_pid, message.mesg_isItRead, interval, blocked);
                }

                if(blocked == true){
                    // cout << message.mesg_pid << " this pid is blocked" << endl;
                    //Process must go in blocked queue :(
                        for(int i = 0; i < 18; i++){
                            //If the space in the blocked Queue is open
                            if(blockedQueue[i].pid == -1){
                                //Set blocked queue pid to the newly blocked pid that pagefaulted.
                                for(int j = 0; j < 18; j++){
                                    if(message.mesg_pid == pTable[i].pid){
                                        pTable[i].blocked = true;
                                        log.open("log.txt", ios::app);
                                        log << "OSS: Process " << message.mesg_pid << " has been set to status blocked" << endl;
                                        log.close();
                                        j = 18;
                                    }
                                }
                                blockedQueue[i].pid = message.mesg_pid;
                                blockRestartSec = rand()%((blockRestartSecMax - 1)+1);
                                blockRestartNS = rand()%((blockRestartNSMax - 1)+1);
                                blockedQueue[i].blockRestartSec = clock->sec;
                                blockedQueue[i].blockRestartNS = clock->nano;
                                log.open("log.txt", ios::app);
                                log << "OSS: Process " << message.mesg_pid << " has entered blocked queue at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
                                log << "OSS: Process " << message.mesg_pid << " will exit blocked queue at time " << blockedQueue[i].blockRestartSec << "s, " << blockedQueue[i].blockRestartNS << "ns." << endl;
                                log.close();
                                i = 18;
                            }
                        }

                    message.mesg_blocked = true;
                    message.mesg_type = message.mesg_pid;
                    msgsnd(msgidTwo, &message, sizeof(message), 0);

                }
                
                blocked = false;
                
                // cout << interval << " ; interval" << endl;
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
            }
            interval = rand()%((maxSystemTimeSpent - 1)+1);
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };
            int tempType = message.mesg_pid;
            bool tempBlocked = message.mesg_blocked;
            int blockCheck = checkBlockedQueue(clock, blockedQueue, pTable);
            if(blockCheck != 0){
                log.open("log.txt", ios::app);
                log << "OSS: Block Check: " << blockCheck << " is supposed to be unblocked" << endl;
                log << "OSS: Former Process " << message.mesg_pid << " has status terminateNow of " << message.mesg_terminateNow << endl;
                log.close();

                interval = rand()%((maxSystemTimeSpent - 1)+1);
                clock->nano+= interval;
                while(clock->nano >= billion){
                    clock->nano-= billion;
                    clock->sec+= 1;
                };
                message.mesg_blocked = false;
                // cout << blockCheck << " unblocked pid" << endl;
                message.mesg_type = blockCheck;
                log.open("log.txt", ios::app);
                log << "OSS: Process: " << blockCheck << " has come unblocked at time " << clock->sec << "s, " << clock->nano << "ns." << endl;
                log.close();
                msgsnd(msgidTwo, &message, sizeof(message), 0);
                // if(message.mesg_terminated != true || message.mesg_type != tempPid){
                    message.mesg_blocked = tempBlocked;
                    message.mesg_type = tempType;
                    log.open("log.txt", ios::app);
                    log << "OSS: Current Process has been set to : " << message.mesg_pid << " with blocked status: " << message.mesg_blocked << endl;
                    log.close();
                // }
                
            }

            if(message.mesg_terminated != true){
                log.open("log.txt", ios::app);
                log << "OSS: Process " << message.mesg_pid << " is being sent a message" << endl;
                log.close();
                message.mesg_type = message.mesg_pid;
                msgsnd(msgidTwo, &message, sizeof(message), 0);
            }

        }
        else
        {
            log.open("log.txt", ios::app);
            log << "OSS: All Processes Blocked" << endl;
            log.close();
            interval = rand()%((maxSystemTimeSpent - 1)+1);
            clock->nano+= interval;
            while(clock->nano >= billion){
                clock->nano-= billion;
                clock->sec+= 1;
            };
            cout << clock->nano << " ns current" << endl;
            cout << clock->sec << " s current" << endl;
            int blockCheck = checkBlockedQueue(clock, blockedQueue, pTable);
            if(blockCheck != 0){
                cout << blockCheck << " pid coming unblocked" << endl;
                message.mesg_blocked = false;
                // cout << blockCheck << " unblocked pid" << endl;
                message.mesg_pid = blockCheck;
                log.open("log.txt", ios::app);
                log << "OSS: Process" << message.mesg_pid << " has come unblocked and is being sent a message to continue" << endl;
                log.close();
                msgsnd(msgidTwo, &message, sizeof(message), 0);
            }
        }
        loops++;
        // cout << loops << " oss loops" <<endl;
    }
    

    for(int i = 0; i < 18; i++){
        if(pTable[i].pid != -1){
            log.open("log.txt", ios::app);
            log << "OSS: Process: " << pTable[i].pid << " has not terminated " <<endl;
            log << message.mesg_pid << " ; terminated block pid" << endl;
            log.close();
            // log << "OSS: Process: " << message.mesg_pid << endl;
        }
    }
    displayFrameTable(frameTable);
    wait(NULL);
    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgidTwo, IPC_RMID,NULL);
    shmctl(shmidClock, IPC_RMID, NULL);
    shmctl(shmidProc, IPC_RMID, NULL);
    
}

int frameTableAccess(vector <frame> &frameTable, int ptNum, int pid, int read, int &interval, bool &blocked){
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
                blocked = true;
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
    return 0;
}

void displayFrameTable(vector<frame> &frameTable){
    cout << "            Process ID       Dirty Bit      SC" << endl;
    for(int i = 0; i < frameTable.size(); i++){
        cout << "Frame: " << i <<":   " << frameTable[i].pid << "            " << frameTable[i].dirtyBit << "              " << frameTable[i].secondChance << endl;
        // cout << "Index " << i << ": pid: " << frameTable[i].pid << " dirty bit: " << frameTable[i].dirtyBit << " second chance: " <<frameTable[i].secondChance << endl;
    }
}

int checkBlockedQueue(simClock *clock, processes blockedQueue[18], processes *pTable){
    ofstream log("log.txt", ios::app);
    log.close();
    cout << "enter checkBlockedQueue" << endl;
    int temp = 0;
    for(int i = 0; i < 18; i++){
        // cout << blockedQueue[i].pid << " blockedQueue Pid at index: " << i << endl;
        if(blockedQueue[i].pid != -1){
            // cout << "found blocked process" << endl;
            // cout << blockedQueue[i].blockRestartSec << " blocked process unblocked at sec" << endl;
            // cout << blockedQueue[i].blockRestartNS << " blocked queue unblocked at ns" << endl;
            // cout << clock->sec << " current seconds" << endl;
            // cout << clock->nano << " current ns" << endl;
            if((blockedQueue[i].blockRestartSec < clock->sec) || (blockedQueue[i].blockRestartSec == clock->sec && blockedQueue[i].blockRestartNS <= clock->nano)){
                // cout << "process should be unblocked" << endl;
                for(int j = 0; j < 18; j++){
                    if(blockedQueue[i].pid == pTable[i].pid){
                        pTable[i].blocked = false;
                    }
                }
                temp = blockedQueue[i].pid;
                blockedQueue[i].pid = -1;
                // cout << clock->nano << "time passed" <<endl;
                // cout << clock->sec << " time passed sec" <<endl;
                // cout << blockedQueue[i].blockRestartSec << " ; time to restart sec" <<endl;
                // cout << blockedQueue[i].blockRestartNS << " ; time to restart ns" <<endl;
                return temp;
            }else
            {
                // log.open("log.txt", ios::app);
                // log << "checkBlockedQueue: Process: " << blockedQueue[i].pid << " cannot be unblocked" << endl;
                // log << "checkBlockedQueue: Current Time is: " << clock->sec << "s, " << clock->nano << "ns." << endl;
                // log << "checkBlockedQueue: Time Required Til Unblock << " << blockedQueue[i].blockRestartSec << "s, " << blockedQueue[i].blockRestartNS << "ns" << endl;
                // log.close();
            }
        }
    }

    return 0;
}

bool secondChance(simClock *clock, vector<frame> &frameTable){
    ofstream log("log.txt", ios::app);
    log.close();
    int maxSize = frameTable.size();
    bool cleared = false;
    if(frameTable.size() >= 256){
        log.open("log.txt", ios::app);
        log << "Second Chance: Too Many Frames" << endl;
        log.close();
        //Too many frames, time to start replacing
        for(int index = 0 ; index < maxSize; index++){
            //If this frame hasn't been used recently.
            if(frameTable[index].secondChance == 0){
                //Remove this element from the array
                log.open("log.txt", ios::app);
                log << "erased index " << index << endl;
                log.close();
                frameTable.erase(frameTable.begin() + index);
                cleared = true;
                index = maxSize;
                //Exit loop
            }
            //If this specific frame has been used, make it unused.
            else if(frameTable[index].secondChance == 1)
            {
                frameTable[index].secondChance = 0;
            }
        }
        if(cleared != true){
        for(int index = 0; index < maxSize; index++){
            //Go to the first element that is 0, which technically should be the first element of the vector, and erase it.
            if(frameTable[index].secondChance == 0){
                log.open("log.txt", ios::app);
                log << "erased index after first passthrough: " << index << endl;
                log.close();
                frameTable.erase(frameTable.begin() + index);
                cleared = true;
                index = maxSize;
            }
        }
    }
    }
    return cleared;
}

