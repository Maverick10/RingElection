#include "IPC.h"

Head* getShm(const char *path, int projID, int size) {
	int key = ftok(path, projID);
	printf("Key is %d\n", key);
	int id = shmget(key, size, IPC_CREAT | 0666);
	std::cout << "SHM ID " << id << std::endl;
	Head *address = (Head*) shmat(id, NULL, 0);
	// std::cout << "number of attached processes to shared memory " << buf.shm_nattch << std::endl;
	return address;
}

Head readFromShm(int semAddress, Head *shm) {
	Head val;
	semLock(semAddress);
	// std::cout << "SEM LOCKED " << shm << std::endl;
	val = *shm;
	// std::cout << "BEFORE SEM UNLOCKED " << std::endl;

	semUnlock(semAddress);
	// std::cout << "SEM UNLOCKED " << std::endl;

	return val;
}

void writeToShm(int semAddr, Head src, Head *dst) {
	semLock(semAddr);
	*dst = src;
	semUnlock(semAddr);
}

void sendMessage(int receiver, Message *msg) {
	int msgqid = msgget(receiver, 0600 | IPC_CREAT);
	if (msgqid < 0) {
		printf("failed to open message queue. return value = %d\n", msgqid);
	}
	int retVal = msgsnd(msgqid, msg, sizeof(msg->mtext), 0);
	printf("return val from send is %d\n", retVal);
	printf("message sent\n");
}

int receiveMessage(int receiver, int msgtype, Message *msg) {
	int msgqid = msgget(receiver, 0600 | IPC_CREAT);
	if (msgqid < 0) {
		printf("failed to open message queue. return value = %d\n", msgqid);
		return -1;
	}
	int retVal = msgrcv(msgqid, msg, sizeof(msg->mtext), msgtype, IPC_NOWAIT | MSG_NOERROR);
//	while (retVal >= 0)
//		retVal = msgrcv(msgqid, msg, sizeof(msg->mtext), msgtype, IPC_NOWAIT);
//	printf("errno is %d\n", errno);
	return retVal;
}
