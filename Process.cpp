#include "Process.h"

Process::Process() {
	this->pid = (rand() % MAXPID) + 1;
	this->isHead = 0;
	this->lastHeartbeatSender = -1;
	this->lastHeartbeatReceivedTimestamp = this->lastHeartbeatSentTimestamp =
			this->lastSentDeathNoteTimestamp = 0;
	std::cout << "PID = " << pid << std::endl;
	this->initShm();

	this->enterRing();
	printf("entered ring\n");
	this->lifeLoop();

}

Process::~Process() {
}

void Process::initShm() {
	this->headShm = getShm(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID,
			sizeof(Head));
	std::cout << "HEAD SHM " << headShm << std::endl;
	this->semAddress = getSem(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID, 1);
}

int Process::getPid() {
	return this->pid;
}

void Process::appointAsHead(int next) {
	std::cout << "Shm is clean, wrote to shm\n";
	Head entry(this->pid, next);
	this->semAddress = getSem(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID, 1);

	writeToShm(semAddress, entry, this->headShm);
	this->isHead = 1;
	this->next = next;
}

void Process::sendChangeNext(int first, int mid, int last) { // mid.next = last, first.next = mid
	Message *msgToMid = new Message();
	msgToMid->mtype = MSGTYPE_CHANGENEXT;
	sprintf(msgToMid->mtext, "%d %lld %d", this->pid, getCurTime(), last);
	sendMessage(mid, msgToMid);
	Message *msgToFirst = new Message();
	msgToFirst->mtype = MSGTYPE_CHANGENEXT;
	sprintf(msgToFirst->mtext, "%d %lld %d", this->pid, getCurTime(), mid);
	sendMessage(first, msgToFirst);
	delete msgToFirst;
	delete msgToMid;
}

void Process::enterRing() {
	Head head = readFromShm(semAddress, this->headShm);	// head is not to be confused with coordinator, they are two different concepts. although they can be the same process
	if (head.id == 0) {	// shm is clean, this is the first process
		this->appointAsHead(this->pid);
	} else {
		if (this->pingProcess(head.id)) {
			printf("Process %d is alive and well\n", head.id);
			this->sendChangeNext(head.id, this->pid, head.next); // this->next = head.next, head.id.next = this->pid
		} else if (this->pingProcess(head.next)) { // head is dead, must send a message to ring that this node is dead
			printf("Process %d is alive and well\n", head.next);
			this->appointAsHead(head.next);
			this->sendProcessDeath(this->pid, getCurTime(), head.id);
		} else {
			puts("All dead");
			this->appointAsHead(this->pid);
		}
	}
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

bool Process::isPrevProcessDead() {
	return getCurTime() - this->lastHeartbeatReceivedTimestamp
			> HEARTBEAT_TIMEOUT;
}

void Process::sendProcessDeath(int originalSender, TIME t, int deadProcess) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PROCESSDEATH;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, t, deadProcess,
			originalSender);
	sendMessage(this->next, msg);
	delete msg;
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
				sendProcessDeath(this->pid, timestamp,
						this->lastHeartbeatSender);
				lastSentDeathNoteTimestamp = timestamp;
			}
		}
	}
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
	printf("Changed next to process %d\n", this->next);
}

void Process::receiveHeartbeat(Message *msg) {
	int sender;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld", &sender, &timestamp);
	this->lastHeartbeatReceivedTimestamp = timestamp;
	this->lastHeartbeatSender = sender;
	printf("Received heartbeat from %d at %lld\n", sender, timestamp);
}

void Process::receiveProcessDeath(Message *msg) {
	int sender, deadProcess, originalSender;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &deadProcess,
			&originalSender);
	printf("%d Received message from %d at %lld that process %d died\n",
			this->pid, sender, timestamp, deadProcess);
	if (this->next == deadProcess) 	// next process died, stop message
		this->next = originalSender;
	else if (this->pid != originalSender
			&& getCurTime() - timestamp <= MSG_TIMEOUT) // relay message to next process, stop message if it took a full lap
														// also timeout was added so that if for any reasons the message was not stopped by the original process
														// or the preceding process to the dead one, this would stop it
		this->sendProcessDeath(originalSender, timestamp, deadProcess);
}

void Process::listenToQueue() {

	Message *msg = new Message();
	int val = receiveMessage(this->pid, 0, msg);
	if (val >= 0) {	// received a message
		printf("Received %ld\n%s\n", msg->mtype, msg->mtext);
		printf("msgrcv val is %d\n", val);
		if (msg->mtype == MSGTYPE_PINGREQUEST)
			this->receivePingRequest(msg);
		else if (msg->mtype == MSGTYPE_CHANGENEXT)
			this->receiveChangeNext(msg);
		else if (msg->mtype == MSGTYPE_HEARTBEAT)
			this->receiveHeartbeat(msg);
		else if (msg->mtype == MSGTYPE_PROCESSDEATH)
			this->receiveProcessDeath(msg);
	}
	delete msg;

}

bool Process::pingProcess(int pid) {
	printf("Pinging pid %d\n", pid);
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PINGREQUEST;
	TIME msgTime = getCurTime();
	sprintf(msg->mtext, "%d %lld", this->pid, msgTime);
	printf("sent message is %ld %s\n", msg->mtype, msg->mtext);
	sendMessage(pid, msg);

	while (getCurTime() - msgTime <= PING_TIMEOUT) {// wait till you receive a reply
//		printf("%lld %lld %d\n", getCurTime(), msgTime, PING_TIMEOUT);
		int val = receiveMessage(this->pid, MSGTYPE_PINGREPLY, msg);
		if (val >= 0) {	// reply received
			printf("Received %ld\n%s\n", msg->mtype, msg->mtext);
			printf("msgrcv val is %d\n", val);
			int pingReplySender;
			sscanf(msg->mtext, "%d", &pingReplySender);
			if (pingReplySender == pid)	// reply was successful
				return 1;
		}
//		sleep(1);
	}
	return 0;
}
