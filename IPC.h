#ifndef IPC_H_
#define IPC_H_

#include <sys/shm.h>
#include <iostream> // for NULL value
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "Semaphore.h"
#include "Head.h"
#include "Message.h"

Head* getShm(const char *path, int projID, int size);
Head readFromShm(int semAddress, Head *shm);
void writeToShm(int semAddr, Head src, Head *dst);
void sendMessage(int receiver, Message *msg);
int receiveMessage(int receiver, int msgtype, Message *msg);

#endif
