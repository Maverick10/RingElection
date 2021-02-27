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
	bool isHead;
	TIME lastHeartbeatReceivedTimestamp;
	TIME lastHeartbeatSentTimestamp;
	int lastHeartbeatSender;

	void initShm();
	void enterRing();
	void listenToQueue();
	bool pingProcess(int pid);
	void appointAsHead(int next);
	void lifeLoop();

	void sendChangeNext(int first, int mid, int last);
	void sendHeartbeat();
	void receivePingRequest(Message *msg);
	void receiveChangeNext(Message *msg);
	void receiveHeartbeat(Message *msg);

public:
	Process();
	~Process();
	int getPid();
};

#endif
