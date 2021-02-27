#include "Process.h"

Process::Process() {
	this->pid = (rand() % MAXPID) + 1;
	this->isHead = 0;
	this->lastHeartbeatSender = -1;
	this->lastHeartbeatReceivedTimestamp = this->lastHeartbeatSentTimestamp =
			this->lastSentDeathNoteTimestamp = 0;
	printf("PID = %d\n", this->pid);
	this->initShm();

	this->enterRing();
//	printf("entered ring\n");
	this->initiateElection();
	this->lifeLoop();
	puts("");
}

Process::~Process() {
}

int Process::getPid() {
	return this->pid;
}

void Process::initShm() {
	this->headShm = getShm(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID,
			sizeof(Head));
//	std::cout << "HEAD SHM " << headShm << std::endl;
	this->semAddress = getSem(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID, 1);
	puts("");
}

void Process::enterRing() {
	Head head = readFromShm(semAddress, this->headShm);	// head is not to be confused with coordinator, they are two different concepts. although they can be the same process
	if (head.id == 0) {	// shm is clean, this is the first process
		this->appointAsHead(this->pid);
	} else {
		if (this->pingProcess(head.id)) {
			printf("Process %d: Process %d is alive and well\n", this->pid,
					head.id);
			this->sendChangeNext(head.id, this->pid); // this->next = head.next, head.id.next = this->pid
			this->next = head.next;
		} else if (this->pingProcess(head.next)) { // head is dead, must send a message to ring that this node is dead
			printf("Process %d: Process %d is alive and well\n", this->pid,
					head.next);
			this->appointAsHead(head.next);
			this->sendProcessDeath(this->pid, getCurTime(), head.id);
		} else {
			printf("Process %d: All dead\n", this->pid);
			this->appointAsHead(this->pid);
		}
	}
	puts("");
}

void Process::listenToQueue() {

	Message *msg = new Message();
	int val = receiveMessage(this->pid, 0, msg);
	if (val >= 0) {	// received a message
//		printf("Process %d: Received %ld\n%s\n", this->pid, msg->mtype,
//				msg->mtext);
//		printf("msgrcv val is %d\n", val);
		if (msg->mtype == MSGTYPE_PINGREQUEST)
			this->receivePingRequest(msg);
		else if (msg->mtype == MSGTYPE_CHANGENEXT)
			this->receiveChangeNext(msg);
		else if (msg->mtype == MSGTYPE_HEARTBEAT)
			this->receiveHeartbeat(msg);
		else if (msg->mtype == MSGTYPE_PROCESSDEATH)
			this->receiveProcessDeath(msg);
		else if (msg->mtype == MSGTYPE_ELECTION)
			this->receiveElection(msg);
		else if (msg->mtype == MSGTYPE_VICTORY)
			this->receiveVictory(msg);
		puts("");
	}
	delete msg;

}

bool Process::pingProcess(int pid) {
	printf("Process %d: Pinging pid %d\n", this->pid, pid);
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PINGREQUEST;
	TIME msgTime = getCurTime();
	sprintf(msg->mtext, "%d %lld", this->pid, msgTime);
	printf("sent message is %ld %s\n", msg->mtype, msg->mtext);
	sendMessage(pid, msg);

	while (getCurTime() - msgTime <= PING_TIMEOUT) {// wait till you receive a reply
		int val = receiveMessage(this->pid, MSGTYPE_PINGREPLY, msg);
		if (val >= 0) {	// reply received
			printf("Process %d: Received %ld\n%s\n", this->pid, msg->mtype,
					msg->mtext);
			printf("msgrcv val is %d\n", val);
			int pingReplySender;
			sscanf(msg->mtext, "%d", &pingReplySender);
			if (pingReplySender == pid)	// reply was successful
				return 1;
		}
	}
	puts("");
	return 0;
}

void Process::appointAsHead(int next) {
	puts("wrote to shm");
//	std::cout << "Shm is clean, wrote to shm\n";
	Head entry(this->pid, next);
	this->semAddress = getSem(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID, 1);

	writeToShm(semAddress, entry, this->headShm);
	this->isHead = 1;
	this->next = next;
	puts("");
}

void Process::lifeLoop() {
	while (1) {
		this->listenToQueue();
//		sleep(1);	// debugging
		this->sendHeartbeat();
		if (this->isPrevProcessDead()) {

			TIME timestamp = getCurTime();
			if (timestamp - this->lastSentDeathNoteTimestamp
					> DEATHNOTE_SEND_GAP) {
				if (this->next != this->lastHeartbeatSender) {
					sendProcessDeath(this->pid, timestamp,
							this->lastHeartbeatSender);
					this->lastSentDeathNoteTimestamp = timestamp;
				} else {	// corner case, loop only contained two processes

					appointAsHead(this->pid);
				}
			}
		}
	}
}

bool Process::isPrevProcessDead() {
	return getCurTime() - this->lastHeartbeatReceivedTimestamp
			> HEARTBEAT_TIMEOUT;
}

void Process::initiateElection() {
	printf("Process %d: initiated election\n", this->pid);
	this->hasInitiatedElection = 1;
	this->sendElection(this->pid, this->pid);
	puts("");
}

void Process::sendChangeNext(int from, int to) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_CHANGENEXT;
	sprintf(msg->mtext, "%d %lld %d", this->pid, getCurTime(), to);
	sendMessage(from, msg);
	delete msg;
}

void Process::sendHeartbeat() {
	TIME curTime = getCurTime();
	if (curTime - this->lastHeartbeatSentTimestamp > HEARTBEAT_SEND_GAP) {
		Message *msg = new Message();
		msg->mtype = MSGTYPE_HEARTBEAT;
		sprintf(msg->mtext, "%d %lld", this->pid, curTime);
		this->lastHeartbeatSentTimestamp = curTime;
		sendMessage(this->next, msg);
		delete msg;
	}
}

void Process::sendProcessDeath(int originalSender, TIME t, int deadProcess) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PROCESSDEATH;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, t, deadProcess,
			originalSender);
	sendMessage(this->next, msg);
	delete msg;
}

void Process::sendElection(int initiator, int curWinner) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_ELECTION;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, getCurTime(), initiator,
			curWinner);
	printf("Process %d: sent election message to Process %d\n", this->pid,
			this->next);
	sendMessage(this->next, msg);
	puts("");
}

void Process::sendVictory(int originalSender, int winner) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_VICTORY;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, getCurTime(),
			originalSender, winner);
	sendMessage(this->next, msg);
}

void Process::receivePingRequest(Message *msg) {
	int pingSender;
	sscanf(msg->mtext, "%d", &pingSender);
	Message *pingReply = new Message();
	pingReply->mtype = MSGTYPE_PINGREPLY;
	sprintf(pingReply->mtext, "%d %lld", this->pid, getCurTime());
	sendMessage(pingSender, pingReply);
	delete pingReply;
}

void Process::receiveChangeNext(Message *msg) {
	int newNext;
	sscanf(msg->mtext, "%d %d %d", &newNext, &newNext, &newNext);
	this->next = newNext;
	if (this->isHead)
		this->appointAsHead(this->next);
	printf("Process %d: Changed next to process %d\n", this->pid, this->next);
	puts("");
}

void Process::receiveHeartbeat(Message *msg) {
	int sender;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld", &sender, &timestamp);
	this->lastHeartbeatReceivedTimestamp = timestamp;
	this->lastHeartbeatSender = sender;
	printf("Process %d: Received heartbeat from %d at %lld\n", this->pid,
			sender, timestamp);
	puts("");
}

void Process::receiveProcessDeath(Message *msg) {
	int sender, deadProcess, originalSender;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &deadProcess,
			&originalSender);
	printf(
			"Process %d: Received message from %d at %lld that process %d died\n",
			this->pid, sender, timestamp, deadProcess);
	if (this->next == deadProcess) { // next process died, stop message
		int oldNext = this->next;
		this->next = originalSender;

		if (oldNext == coordinatorPid)
			this->initiateElection();
		printf("Process %d: changed next to %d\n", this->pid, this->next);
	} else if (this->pid != originalSender
			&& getCurTime() - timestamp <= MSG_TIMEOUT) // relay message to next process, stop message if it took a full lap
														// also timeout was added so that if for any reasons the message was not stopped by the original process
														// or the preceding process to the dead one, this would stop it
		this->sendProcessDeath(originalSender, timestamp, deadProcess);
	puts("");
}

void Process::receiveElection(Message *msg) {
	int sender, initiator, curWinner;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &initiator,
			&curWinner);
	printf(
			"Process %d: Received election message, initiator is process %d, curWinner is process %d\n",
			this->pid, initiator, curWinner);
	if (initiator == this->pid) { // election stops, winner is determined
		printf("Process %d: Process %d is the new coordinator\n", this->pid,
				curWinner);
		this->coordinatorPid = curWinner;
		sendVictory(this->pid, curWinner);
		// TODO: send victory
	} else {
		sendElection(initiator, max(curWinner, this->pid));
	}
	puts("");
}

void Process::receiveVictory(Message *msg) {
	int sender, originalSender, coordinator;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &originalSender,
			&coordinator);
	printf(
			"Process %d: Received victory message, original sender is process %d, coordinator is process %d\n",
			this->pid, originalSender, coordinator);
	if (originalSender != this->pid) { // relay to next process
		this->coordinatorPid = coordinator;
		sendVictory(originalSender, coordinator);
	}
	puts("");
}
