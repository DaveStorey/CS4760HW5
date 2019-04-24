#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/msg.h>

//Signal handler to catch ctrl-c
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
}message;

void scheduler(char* outfile, int total){
	unsigned int quantum = 500000;
	int alive = 0, totalSpawn = 0, msgid, msgid1, msgid2, shmID, timeFlag = 0, i = 0, resource = 20, totalFlag = 0, pid[10], status, requests[10];
	unsigned long increment, timeBetween;
	char * parameter[32], parameter1[32], parameter2[32], parameter3[32], parameter4[32], parameter5[32], parameter6[32];
	//Pointer for the shared memory timer
	struct clock * shmPTR;
	struct clock launchTime;
	//Time variables for the time out function
	time_t when, when2;
	//File pointers for input and output, respectively
	FILE * outPut;
	//Key variable for shared memory access.
	unsigned long key, msgKey, msgKey1, msgKey2;
	srand(time(0));
	timeBetween = (rand() % 100000000) + 1000000;
	key = rand();
	msgKey = ftok("child.c", 65);
	msgKey1 = ftok("child.c", 67);
	msgKey2 = ftok("child.c", 69);
	msgid = msgget(msgKey, 0777 | IPC_CREAT);
	msgid1 = msgget(msgKey1, 0777 | IPC_CREAT);
	msgid2 = msgget(msgKey2, 0777 | IPC_CREAT);
	message.mesg_type = 1;
	//Setting initial time for later check.
	time(&when);
	outPut = fopen(outfile, "w");
	//Check for file error.
	if (outPut == NULL){
		perror("Error");
		printf("Output file could not be created.\n");
		exit(EXIT_SUCCESS);
	}
	
	//Get and access shared memory, setting initial timer state to 0.
	shmID = shmget(key, sizeof(struct clock), IPC_CREAT | IPC_EXCL | 0777);
	shmPTR = (struct clock *) shmat(shmID, (void *) 0, 0);
	shmPTR[0].sec = 0;
	shmPTR[0].nano = 0;
	launchTime.sec = 0;
	launchTime.nano = timeBetween;
	//Initializing pids to -1 
	for(int k = 0; k < total; k++){
		pid[k] = -1;
	}
	//Call to signal handler for ctrl-c
	signal(SIGINT, intHandler);
	increment = (rand() % 5000000) + 25000000;
	//While loop keeps running until all children are dead, ctrl-c, or time is reached.
	while((i < 10) && (keepRunning == 1) && (timeFlag == 0)){
		//Incrementing the timer.
		time(&when2);
		/*if (when2 - when > 3){
			timeFlag = 1;
		}*/
		shmPTR[0].nano += increment;
		if(shmPTR[0].nano >= 1000000000){
			shmPTR[0].sec += 1;
			shmPTR[0].nano -= 1000000000;
		}
		//If statement to spawn child if timer has passed its birth time.
		if((shmPTR[0].sec > launchTime.sec) || ((shmPTR[0].sec == launchTime.sec) && (shmPTR[0].nano > launchTime.nano))){
			if((pid[i] = fork()) == 0){
			//Converting key, shmID and life to char* for passing to exec.
				sprintf(parameter1, "%li", key);
				sprintf(parameter2, "%d", quantum);
				sprintf(parameter3, "%li", msgKey);
				sprintf(parameter4, "%li", msgKey1);
				sprintf(parameter5, "%li", msgKey2);
				sprintf(parameter6, "%d", i+1);
				srand(getpid() * (time(0) / 3));
				char * args[] = {parameter1, parameter2, parameter3, parameter4, parameter5, parameter6, NULL};
				execvp("./child\0", args);
			}
			usleep(200000);
			launchTime.sec = shmPTR[0].sec;
			launchTime.nano = shmPTR[0].nano;
			launchTime.nano += timeBetween;
			if(launchTime.nano >= 1000000000){
				launchTime.sec += 1;
				launchTime.nano -= 1000000000;
			}
			alive++;
			totalSpawn++;
			i++;
		}
		if (msgrcv(msgid2, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			resource = resource - message.request;
			printf("Parent receiving %d resource from dying child,%li. %d remain.\n", message.request, message.processNum, resource);
			alive--;
			pid[message.processNum-1] = -1;
			requests[message.processNum-1] = 0;
			for (int k = 0; k < totalSpawn; k++){
				if ((requests[k] > 0) && (requests[k] < resource)){
					message.mesg_type = k+1;
					message.granted = requests[k];
					resource = resource - requests[k];
					message.request = 0;
					printf("Parent granting child %li %d resources, %d remain.\n", message.processNum, message.request, resource);
					msgsnd(msgid1, &message, sizeof(message), 0);
				}
			}
		}
		if (msgrcv(msgid, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			if(resource - message.request > 0){
				resource = resource - message.request;
				message.granted = message.request;
				message.mesg_type = message.processNum;
				printf("Parent granting child %li %d resources, %d remain.\n", message.processNum, message.request, resource);
				message.request = 0;
				msgsnd(msgid1, &message, sizeof(message), 0);
			}
			else{
				requests[message.processNum-1] = message.request;
			}
		}
	}
	while(alive > 0 && keepRunning == 1){
		if (msgrcv(msgid2, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			resource = resource - message.request;
			printf("Parent receiving %d resource from dying child %li.\n", message.request, message.processNum);
			requests[message.processNum-1] = 0;
			alive--;
			for (int k = 0; k < totalSpawn; k++){
				if ((requests[k] > 0) && (requests[k] < resource)){
					message.mesg_type = k+1;
					message.granted = requests[k];
					resource = resource - requests[k];
					message.request = 0;
					printf("Parent granting child %li %d resources, %d remain.\n", message.processNum, message.request, resource);
					msgsnd(msgid1, &message, sizeof(message), 0);
				}
			}
		}
		else if (msgrcv(msgid, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			if(resource - message.request > 0){
				resource = resource - message.request;
				message.granted = message.request;
				message.mesg_type = message.processNum;
				printf("Parent granting child %li %d resources, %d remain.\n", message.processNum, message.request, resource);
				message.request = 0;
				msgsnd(msgid1, &message, sizeof(message), 0);
			}
		}
	}	
	if(timeFlag == 1){
		printf("Program has reached its allotted time, exiting.\n");
		//fprintf(outPut, "Scheduler terminated at %li nanoseconds due to time limit.\n",  shmPTR[0].timeClock);
	}
	if(totalFlag == 1){
		printf("Program has reached its allotted children, exiting.\n");
		//fprintf(outPut, "Scheduler terminated at %li nanoseconds due to process limit.\n",  shmPTR[0].timeClock);
	}
	if(keepRunning == 0){
		printf("Terminated due to ctrl-c signal.\n");
		//fprintf(outPut, "Scheduler terminated at %li nanoseconds due to ctrl-c signal.\n",  shmPTR[0].timeClock);
	}
	shmdt(shmPTR);
	shmctl(shmID, IPC_RMID, NULL);
	msgctl(msgid, IPC_RMID, NULL);
	msgctl(msgid1, IPC_RMID, NULL);
	msgctl(msgid2, IPC_RMID, NULL);
	wait(NULL);
	fclose(outPut);
	sleep(1);
}
