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
	// variables
	int pid;	// determines coordinator election
	int next;
	Head *headShm; // shared memory segment to identify the head of the election ring
	int semAddress;
	bool isHead;
	TIME lastHeartbeatReceivedTimestamp;
	TIME lastHeartbeatSentTimestamp;
	TIME lastSentDeathNoteTimestamp;
	int lastHeartbeatSender;
	int coordinatorPid;
	bool hasInitiatedElection;
	bool hasSentVictory;

	// control methods
	void initShm();
	void enterRing();
	void listenToQueue();
	bool pingProcess(int pid);
	void appointAsHead(int next);
	void lifeLoop();
	bool isPrevProcessDead();
	void initiateElection();

	// ipc-related methods
	void sendChangeNext(int from, int to);
	void sendHeartbeat();
	void sendProcessDeath(int originalSender, TIME t, int deadProcess);
	void sendElection(int initiator, int curWinner);
	void sendVictory(int originalSender, int winner);
	void receivePingRequest(Message *msg);
	void receiveChangeNext(Message *msg);
	void receiveHeartbeat(Message *msg);
	void receiveProcessDeath(Message *msg);
	void receiveElection(Message *msg);
	void receiveVictory(Message *msg);

public:
	Process();
	~Process();
	int getPid();
};

#endif
