#include "Semaphore.h"

int getSem(const char* path, int projID, int size) {
	int key = ftok(path, projID);
	int address = semget(key, size, IPC_CREAT | 0666);
	semctl(address, 0, IPC_RMID, NULL);
	address = semget(key, size, IPC_CREAT | 0666);
	Semun semun;
	semun.val = 1;
	semctl(address, 0, SETVAL, semun);
    return address;
}

void semLock(int semAddr)
{
    sembuf p_op;
    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    if (semop(semAddr, &p_op, 1) == -1)
        perror("Error in semLock()");
}

void semUnlock(int semAddr)
{
    struct sembuf p_op;
    p_op.sem_num = 0;
    p_op.sem_op = 1;
    p_op.sem_flg = !IPC_NOWAIT;
    if (semop(semAddr, &p_op, 1) == -1)
        perror("Error in semUnlock()");
}
