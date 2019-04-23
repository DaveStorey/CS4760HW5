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
	int request;
	int granted;
} message;

int main(int argc, char * argv[]){
	srand(getpid()*time(0));
	int msgid, msgid1, msgid2, resourcesHeld = 0;
	unsigned int terminates;
	char* ptr;
	pid_t pid = getpid();
	time_t when, when2;
	struct clock * shmPTR;
	signal(SIGINT, intHandler);
	unsigned long shmID;
	unsigned long key = strtoul(argv[0], &ptr, 10);
	unsigned long life = strtoul(argv[1], &ptr, 10);
	unsigned long msgKey = strtoul(argv[2], &ptr, 10);
	unsigned long msgKey1 = strtoul(argv[3], &ptr, 10);
	unsigned long msgKey2 = strtoul(argv[4], &ptr, 10);
	unsigned long logicalNum = strtoul(argv[5], &ptr, 10);
	time(&when);
	shmID = shmget(key, sizeof(struct clock), 0);
	shmPTR = (struct clock *) shmat(shmID, (void *)0, 0);
	msgid = msgget(msgKey, 0777 | IPC_CREAT);
	msgid1 = msgget(msgKey1, 0777 | IPC_CREAT);
	msgid2 = msgget(msgKey2, 0777 | IPC_CREAT);
	terminates = rand() % 100;
	message.mesg_type = 1;
	printf("In child %li.\n", logicalNum);
	while (terminates > 20){
		message.processNum = logicalNum;
		message.request = (rand() % 7) - 3;
		while ((message.request < 0) && (message.request < (0 - resourcesHeld))){
			message.request = (rand() % 7) - 3;
		}
		message.granted = 0;
		msgsnd(msgid, &message, sizeof(message), 0);
		msgrcv(msgid1, &message, sizeof(message), logicalNum, 0);
		printf("Child %li granted %d resources.\n", logicalNum, message.granted);
		resourcesHeld = resourcesHeld + message.granted;
		terminates = rand() % 100;
		time(&when2);
		if ((when2 - when) > 1){
			terminates = 1;
		}
	}
	message.processNum = logicalNum;
	message.request = 0 - resourcesHeld;
	msgsnd(msgid2, &message, sizeof(message), 0);
	shmdt(shmPTR);
	printf("Child %li dying.\n", logicalNum);
	return 0;
}
