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
	int pid;	// identifier of process. determines coordinator election
	int next;	// pid of next process
	Head *headShm; // shared memory segment to identify the head of the election ring
	int semAddress;	// semaphore address
	bool isHead;	// boolean of whether current process is head of ring
	TIME lastHeartbeatReceivedTimestamp;// timestamp of last heartbeat received
	TIME lastHeartbeatSentTimestamp;	// timestamp of last heartbeat sent
	TIME lastSentDeathNoteTimestamp;	// timestamp of last death note sent
	int lastHeartbeatSender;// pid of last heartbeat sender (used to determine if that process is dead or not)
	int coordinatorPid;	// pid of coordinator process
	bool hasInitiatedElection;// boolean of whether current process initiated election
	bool hasSentVictory;// boolean of whether current process sent victory message

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
