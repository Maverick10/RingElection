#include "Semaphore.h"

/*
 * Gets address of semaphore
 *
 * param path: the path that's used by ftok to generate unique key
 * param projID: an integer used by ftok to generate unique key
 * param size: size of semaphore in bytes
 *
 * returns: address of semaphore
 */

int getSem(const char *path, int projID, int size) {
	int key = ftok(path, projID);
	int address = semget(key, size, IPC_CREAT | 0666);
	semctl(address, 0, IPC_RMID, NULL);
	address = semget(key, size, IPC_CREAT | 0666);
	Semun semun;
	semun.val = 1;
	semctl(address, 0, SETVAL, semun);
	return address;
}

/*
 * Locks semaphore
 *
 * param semAddr: semaphore address
 *
 * returns: void
 */

void semLock(int semAddr) {
	sembuf p_op;
	p_op.sem_num = 0;
	p_op.sem_op = -1;	// lock
	p_op.sem_flg = !IPC_NOWAIT;
	if (semop(semAddr, &p_op, 1) == -1)
		perror("Error in semLock()");
}

/*
 * Unlocks semaphore
 *
 * param semAddr: semaphore address
 *
 * returns: void
 */

void semUnlock(int semAddr) {
	struct sembuf p_op;
	p_op.sem_num = 0;
	p_op.sem_op = 1;	// unlock
	p_op.sem_flg = !IPC_NOWAIT;
	if (semop(semAddr, &p_op, 1) == -1)
		perror("Error in semUnlock()");
}
