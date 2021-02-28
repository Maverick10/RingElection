#include "IPC.h"

Head* getShm(const char *path, int projID, int size) {	// returns shm address
	int key = ftok(path, projID);
	printf("Key is %d\n", key);
	int id = shmget(key, size, IPC_CREAT | 0666);
	printf("SHM ID %d\n", id);
	Head *address = (Head*) shmat(id, NULL, 0);
	return address;
}

Head readFromShm(int semAddress, Head *shm) {
	Head val;
	semLock(semAddress);// locks semaphore before reading from shm to ensure mutual exclusion
	val = *shm;
	semUnlock(semAddress);	// unlocks semaphore after reading from shm

	return val;
}

void writeToShm(int semAddr, Head src, Head *dst) {
	semLock(semAddr);// locks semaphore before writing to shm to ensure mutual exclusion
	*dst = src;
	semUnlock(semAddr);	// locks semaphore after writing to shm
}

void sendMessage(int receiver, Message *msg) {
	int msgqid = msgget(receiver, 0600 | IPC_CREAT);// using message queues, create it if it doesn't exists
	if (msgqid < 0) {
		printf("failed to open message queue. return value = %d\n", msgqid);
	}
	int retVal = msgsnd(msgqid, msg, sizeof(msg->mtext), 0);
//	printf("return val from send is %d\n", retVal);
//	printf("message sent\n");
}

int receiveMessage(int receiver, int msgtype, Message *msg) {
	int msgqid = msgget(receiver, 0600 | IPC_CREAT);// using message queues, create it if it doesn't exists
	if (msgqid < 0) {
		printf("failed to open message queue. return value = %d\n", msgqid);
		return -1;
	}
	int retVal = msgrcv(msgqid, msg, sizeof(msg->mtext), msgtype,
	IPC_NOWAIT | MSG_NOERROR);// with options to not wait for messages, and if there's an error not to give it back
//	while (retVal >= 0)
//		retVal = msgrcv(msgqid, msg, sizeof(msg->mtext), msgtype, IPC_NOWAIT);
//	printf("errno is %d\n", errno);
	return retVal;
}
