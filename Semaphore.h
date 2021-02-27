#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <cstdio>

union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    unsigned short *array; /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

int getSem(const char *path, int projID, int size);
void semLock(int address);
void semUnlock(int address);

#endif /* SEMAPHORE_H_ */
