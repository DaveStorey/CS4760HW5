#include<stdio.h>
#include<fcntl.h>
#include<sys/ipc.h>
#include<string.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/msg.h>
#include <signal.h>
#include<time.h>

#define resSize 3

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

struct clock{
	unsigned long nano;
	unsigned int sec;
};

struct mesg_buffer{
	long mesg_type;
	unsigned long processNum;
	int request[resSize];
	int granted[resSize];
} message;

int main(int argc, char * argv[]){
	srand(getpid()*time(0));
	int msgid, msgid1, msgid2, resourcesHeld[resSize];
	for(int k = 0; k < resSize; k++){
		resourcesHeld[k] = 0;
	}
	unsigned int terminates;
	char* ptr;
	pid_t pid = getpid();
	struct clock * shmPTR;
	signal(SIGINT, intHandler);
	unsigned long shmID;
	unsigned long key = strtoul(argv[0], &ptr, 10);
	unsigned long life = strtoul(argv[1], &ptr, 10);
	unsigned long msgKey = strtoul(argv[2], &ptr, 10);
	unsigned long msgKey1 = strtoul(argv[3], &ptr, 10);
	unsigned long msgKey2 = strtoul(argv[4], &ptr, 10);
	unsigned long logicalNum = strtoul(argv[5], &ptr, 10);
	shmID = shmget(key, sizeof(struct clock), 0);
	shmPTR = (struct clock *) shmat(shmID, (void *)0, 0);
	msgid = msgget(msgKey, 0777 | IPC_CREAT);
	msgid1 = msgget(msgKey1, 0777 | IPC_CREAT);
	msgid2 = msgget(msgKey2, 0777 | IPC_CREAT);
	terminates = rand() % 100;
	message.mesg_type = 1;
	printf("In child %li.\n", logicalNum);
	while (terminates > 10){
		message.processNum = logicalNum;
		if (terminates > 30){
			for(int k = 0; k < resSize; k++){
				message.request[k] = (rand() % 4);
				//Ensuring process cannot request more resources than system has.
				while (message.request[k] + resourcesHeld[k] > resSize){
					message.request[k] = (rand() % 4);
				}
			}
		printf("Child %li requesting resources.\n", logicalNum);
		}
		else{
			for(int k = 0; k < resSize; k++){
				if (resourcesHeld[k] > 0){
					message.request[k] = 0 - (rand() % 4);
					//While loop prevents process releasing more resources than it has
					while (message.request[k] < (0 - resourcesHeld[k])){
						message.request[k] = 0 - (rand() % 4);
					}
				}
				else{
					message.request[k] = 0;
				}
			}
			printf("Child %li releasing resources.\n", logicalNum);
			//printf("Child %li requesting %d resources.\n", logicalNum, message.request[k]);
		}
		message.mesg_type = 1;
		msgsnd(msgid, &message, sizeof(message), 0);
		msgrcv(msgid1, &message, sizeof(message), logicalNum, 0);
		printf("Child %li receiving resources.\n", logicalNum);
		for(int k = 0; k < resSize; k++){
			resourcesHeld[k] = resourcesHeld[k] + message.granted[k];
			//printf("Child %li granted %d resources, holds %d.\n", logicalNum, message.granted[k], resourcesHeld[k]);
		}
		terminates = rand() % 100;
	}
	message.processNum = logicalNum;
	for(int k = 0; k < resSize; k++){
		message.request[k] = 0 - resourcesHeld[k];
	}
	msgsnd(msgid2, &message, sizeof(message), 0);
	shmdt(shmPTR);
	printf("Child %li dying.\n", logicalNum);
	return 0;
}
