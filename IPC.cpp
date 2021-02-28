#include "IPC.h"

/*
 * Gets address of shared memory segment
 *
 * param path: the path that's used by ftok to generate unique key
 * param projID: an integer used by ftok to generate unique key
 * param size: size of shared memory segment in bytes
 *
 * returns: address of shared memory segment
 */

Head* getShm(const char *path, int projID, int size) {	// returns shm address
	int key = ftok(path, projID);
	printf("Key is %d\n", key);
	int id = shmget(key, size, IPC_CREAT | 0666);
	printf("SHM ID %d\n", id);
	Head *address = (Head*) shmat(id, NULL, 0);
	return address;
}

/*
 * Reads from the provided shared memory
 *
 * param semAddress: semaphore address used to ensure mutex
 * param shm: shared memory pointer
 *
 * returns: value of shared memory
 */

Head readFromShm(int semAddress, Head *shm) {
	Head val;
	semLock(semAddress);// locks semaphore before reading from shm to ensure mutual exclusion
	val = *shm;
	semUnlock(semAddress);	// unlocks semaphore after reading from shm

	return val;
}

/*
 * Writes to shared memory
 *
 * param semAddr: semaphore address used to ensure mutex
 * param src: data required to be written in shm
 * param dst: shm pointer
 *
 * returns: void
 */

void writeToShm(int semAddr, Head src, Head *dst) {
	semLock(semAddr);// locks semaphore before writing to shm to ensure mutual exclusion
	*dst = src;
	semUnlock(semAddr);	// locks semaphore after writing to shm
}

/*
 * Sends message using message queues
 *
 * param receiver: pid of the receiving process
 * param msg: pointer to Message object that's going to be sent
 *
 * returns: void
 */

void sendMessage(int receiver, Message *msg) {
	int msgqid = msgget(receiver, 0600 | IPC_CREAT);// using message queues, create it if it doesn't exists
	if (msgqid < 0) {
		printf("failed to open message queue. return value = %d\n", msgqid);
	}
	int retVal = msgsnd(msgqid, msg, sizeof(msg->mtext), 0);
//	printf("return val from send is %d\n", retVal);
//	printf("message sent\n");
}

/*
 * Receives message using message queues
 *
 * param receiver: pid of the receiving process
 * param msgtype: type of message required to be received (0 for first message, any positive number will give the first message in the queue with that type)
 * param msg: pointer to received msg
 *
 * returns: number of bytes copied to msg (-1 if no message is received)
 */

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
