#ifndef PROCESS_H_
#define PROCESS_H_

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "IPC.h"
#include "Config.h"
#include "Head.h"
#include "Message.h"
#include "MessageType.h"

class Process {
	int pid;
	int next;
	Head *headShm; // shared memory segment to identify the head of the election ring
	int semAddress;
	void initShm();
	void enterRing();
	void listenToQueue();
	bool pingProcess(int pid);

public:
	Process();
//	Process(bool b); // debugging
	~Process();
	int getPid();
};

#endif
