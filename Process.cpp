#include "Process.h"

/*
 * Constructs the Process object and begins the Process's lifecycle
 */

Process::Process() {
	this->pid = (rand() % MAXPID) + 1;	// pid is generated randomly
	this->isHead = 0;
	this->lastHeartbeatSender = -1;
	this->lastHeartbeatReceivedTimestamp = this->lastHeartbeatSentTimestamp =
			this->lastSentDeathNoteTimestamp = 0;
	printf("PID = %d\n", this->pid);
	this->initShm();// gets a pointer to the shared memory holding the head process

	this->enterRing();	// enters the process ring
//	printf("entered ring\n");
	this->initiateElection();// new processes always initiate election when created
	this->lifeLoop();// process stays inside loop forever until forcefully killed
	puts("");
}

Process::~Process() {
}

/*
 * Gets pid of process
 *
 * returns: pid of process
 */

int Process::getPid() {
	return this->pid;
}

/*
 * Initializes shared memory and semaphores
 *
 * returns: void
 */

void Process::initShm() {
	this->headShm = getShm(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID,
			sizeof(Head));
//	std::cout << "HEAD SHM " << headShm << std::endl;
	this->semAddress = getSem(HEADSHM_FTOK_PATH, HEADSHM_FTOK_PROJID, 1);
	puts("");
}

/*
 * Letting process enter the ring topology
 *
 * returns: void
 */

void Process::enterRing() {
	Head head = readFromShm(semAddress, this->headShm);	// head is not to be confused with coordinator, they are two different concepts. although they can be the same process
	if (head.id == 0) {	// shm is clean, this is the first process
		this->appointAsHead(this->pid);
	} else {
		if (this->pingProcess(head.id)) {// if shm has a head, ping the head to check if it's alive or not
			printf("Process %d: Process %d is alive and well\n", this->pid,
					head.id);
			this->sendChangeNext(head.id, this->pid); // this->next = head.next, head.id.next = this->pid
			this->next = head.next;
		} else if (this->pingProcess(head.next)) { // head is dead, must send a message to ring that this node is dead
			printf("Process %d: Process %d is alive and well\n", this->pid,
					head.next);
			this->appointAsHead(head.next);
			this->sendProcessDeath(this->pid, getCurTime(), head.id);
		} else { // head and next of head are dead. so this must be a new ring
			printf("Process %d: All dead\n", this->pid);
			this->appointAsHead(this->pid);
		}
	}
	puts("");
}

/*
 * Listening to message queues to see if there's any messages to be received
 *
 * returns: void
 */

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
		else if (msg->mtype == MSGTYPE_COUNT)
			this->receiveCount(msg);
		else if (msg->mtype == MSGTYPE_DATA)
			this->receiveData(msg);
//		puts("");
	}
	delete msg;

}

/*
 * Send a message to another process to check whether it's alive or dead
 *
 * param pid: pid of the process in question
 *
 * returns: true if process is alive, false otherwise
 */

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
// hasn't received a reply before the time ran out. process must be dead
	return 0;
}

/*
 * Make process the head of the ring so that all new processes enter the ring through it
 *
 * param next: pid of the next process
 *
 * returns: void
 */

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

/*
 * Lifecycle of the process. Infinite loop
 *
 * returns: void
 *
 */

void Process::lifeLoop() {
	while (1) {
		this->listenToQueue();
//		sleep(1);	// debugging
		this->sendHeartbeat();	// to tell next process that this pid is alive
		if (this->isPrevProcessDead()) {

			TIME timestamp = getCurTime();
			if (timestamp - this->lastSentDeathNoteTimestamp
					> DEATHNOTE_SEND_GAP) {	// to ensure that death msg is sent to all processes in the ring
											// also to not to flood the message queues with the same message
				if (this->next != this->lastHeartbeatSender) {
					sendProcessDeath(this->pid, timestamp,
							this->lastHeartbeatSender);
					this->lastSentDeathNoteTimestamp = timestamp;
				} else {	// corner case, loop only contained two processes

					appointAsHead(this->pid);
				}
			}
		}
		if (this->pid == this->coordinatorPid) {
			if (!this->hasStartedCounting
					&& getCurTime() - this->lastSendDataTimestamp
							> DATA_SEND_GAP) {
				this->dataCurMin = INT32_MAX;
				this->lastSendDataTimestamp = getCurTime();
				sendCount(this->pid, 0);
			}
		}
	}
}

/*
 * A check of whether the process which has the current process as next is aiive or not
 *
 * returns: true if the prev process is dead, false otherwise
 */

bool Process::isPrevProcessDead() {
	return getCurTime() - this->lastHeartbeatReceivedTimestamp
			> HEARTBEAT_TIMEOUT;
}

/*
 * Initiate election. Sends election message through the ring
 *
 * returns: void
 */

void Process::initiateElection() {
	printf("Process %d: initiated election\n", this->pid);
	this->hasInitiatedElection = 1;
	this->sendElection(this->pid, this->pid);
	puts("");
}

// TODO: Write documentation

char* Process::generateData() {
	int totalSize = PROCESSSEGSZ * this->processCount;
	stringstream ss;
	while (totalSize--) {
		int element = (rand() % ELEMENT_MAX) + 1;
		ss << element << ' ';
	}
	char *ret = new char[MSGMAXSZ];
	ss.getline(ret, MSGMAXSZ);
	return ret;
}

/*
 * Sends a message to the process with pid (from) to change its next pointer to the pid (to)
 *
 * param from: pid of the process to change its next pointer
 * param to: pid of the process to point to
 *
 * returns: void
 */

void Process::sendChangeNext(int from, int to) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_CHANGENEXT;
	sprintf(msg->mtext, "%d %lld %d", this->pid, getCurTime(), to);
	sendMessage(from, msg);
	delete msg;
}

/*
 * Sends a heartbeat message to next process to inform it that the current process is alive
 *
 * returns: void
 */

void Process::sendHeartbeat() {
	TIME curTime = getCurTime();
	if (curTime - this->lastHeartbeatSentTimestamp > HEARTBEAT_SEND_GAP) {// check is to prevent message queue flood
		Message *msg = new Message();
		msg->mtype = MSGTYPE_HEARTBEAT;
		sprintf(msg->mtext, "%d %lld", this->pid, curTime);
		this->lastHeartbeatSentTimestamp = curTime;
		sendMessage(this->next, msg);
		delete msg;
	}
}

/*
 * Sends message to the next process that a process is dead
 *
 * param originalSender: pid of the first process to discover the dead process (needed to stop the message from going around the ring forever)
 * param t: timestamp of death
 * param deadProcess: pid of the deadProcess
 *
 * returns: void
 */

void Process::sendProcessDeath(int originalSender, TIME t, int deadProcess) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PROCESSDEATH;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, t, deadProcess,
			originalSender);
	sendMessage(this->next, msg);
	delete msg;
}

/*
 * Sends election message to next process
 *
 * param initiator: pid of the process that initiated election
 * param curWinner: maximum pid of the processes that received the election message so far (used to determine the winner)
 *
 * returns: void
 */

void Process::sendElection(int initiator, int curWinner) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_ELECTION;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, getCurTime(), initiator,
			curWinner);
	printf("Process %d: sent election message to Process %d\n", this->pid,
			this->next);
	sendMessage(this->next, msg);
	puts("");
	delete msg;
}

/*
 * Sends a victory message to the next process
 *
 * param originalSender: pid of the process that announced the election victory
 * param winner: pid of the new coordinator
 *
 * returns: void
 */

void Process::sendVictory(int originalSender, int winner) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_VICTORY;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, getCurTime(),
			originalSender, winner);
	sendMessage(this->next, msg);
	delete msg;
}

// TODO: Write documentation

void Process::sendCount(int coordinator, int curCount) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_COUNT;
	sprintf(msg->mtext, "%d %lld %d %d", this->pid, getCurTime(), coordinator,
			curCount);
	sendMessage(this->next, msg);
	delete msg;
}

// TODO: Write documentation

void Process::sendData(int coordinator, char *data) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_DATA;
	sprintf(msg->mtext, "%d %lld %d %s", this->pid, getCurTime(), coordinator,
			data);
	sendMessage(this->next, msg);
	delete msg;
	delete data;
}

/*
 * Receive a ping request message and send a ping reply to the process that sent the ping request
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

void Process::receivePingRequest(Message *msg) {
	int pingSender;
	sscanf(msg->mtext, "%d", &pingSender);
	Message *pingReply = new Message();
	pingReply->mtype = MSGTYPE_PINGREPLY;
	sprintf(pingReply->mtext, "%d %lld", this->pid, getCurTime());
	sendMessage(pingSender, pingReply);	// when you receive a ping request you must respond with a ping reply
	delete pingReply;
}

/*
 * Receive a change next message and changes next process accordingly
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

void Process::receiveChangeNext(Message *msg) {
	int newNext;
	sscanf(msg->mtext, "%d %d %d", &newNext, &newNext, &newNext);// only interested in the last value
	this->next = newNext;
	if (this->isHead)
		this->appointAsHead(this->next);// updates shm with head.id = pid and head.next = this->next
	printf("Process %d: Changed next to process %d\n", this->pid, this->next);
	puts("");
}

/*
 * Receives a heartbeat message and update timestamp of last received heartbeat as well as the last sending heartbeat process
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

void Process::receiveHeartbeat(Message *msg) {
	int sender;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld", &sender, &timestamp);
	this->lastHeartbeatReceivedTimestamp = timestamp;
	this->lastHeartbeatSender = sender;
//	printf("Process %d: Received heartbeat from %d at %lld\n", this->pid,
//			sender, timestamp);
//	puts("");
}

/*
 * Receive a process death message. Initiates election if the dead process is the coordinator. Updates next process if the dead process was indeed the next one. Relays message to
 * next process if need be.
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

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

		if (oldNext == coordinatorPid) // if your next was the coordinator then election must be initiated bec coordinator is dead
			this->initiateElection();
		printf("Process %d: changed next to %d\n", this->pid, this->next);
	} else if (this->pid != originalSender
			&& getCurTime() - timestamp <= MSG_TIMEOUT) // relay message to next process, stop message if it took a full lap
														// also timeout was added so that if for any reasons the message was not stopped by the original process
														// or the preceding process to the dead one, this would stop it
		this->sendProcessDeath(originalSender, timestamp, deadProcess);
	puts("");
}

/*
 * Receives election message. Relays message if need be. Stops message if the current process was the initiator and then send a victory message depending on the winner.
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

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
		hasStartedCounting = 0;
		hasSentData = 0;
		dataReceivedCount = 0;
		processCount = 0;

		this->coordinatorPid = curWinner;
		sendVictory(this->pid, curWinner);
	} else {
		sendElection(initiator, max(curWinner, this->pid));	// election message relayed
															// maximizing between curWinner and this->pid ensures that the process with the highest pid always is curWinner
	}
	puts("");
}

/*
 * Receives victory message. Relays message if need be. Stops message if the current process was the original sender. Updates coordinator value depending on the value of coordinator
 * in the message
 *
 * param msg: pointer to message object that contains the received message
 *
 * returns: void
 */

void Process::receiveVictory(Message *msg) {
	int sender, originalSender, coordinator;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &originalSender,
			&coordinator);
	printf(
			"Process %d: Received victory message, original sender is process %d, coordinator is process %d\n",
			this->pid, originalSender, coordinator);
	if (originalSender != this->pid) { // relay to next process
		hasStartedCounting = 0;
		hasSentData = 0;
		dataReceivedCount = 0;
		processCount = 0;

		this->coordinatorPid = coordinator;
		sendVictory(originalSender, coordinator);
	}
	puts("");
}

// TODO: write documentation

void Process::receiveCount(Message *msg) {
	int sender, coordinator, cnt;
	TIME timestamp;
	sscanf(msg->mtext, "%d %lld %d %d", &sender, &timestamp, &coordinator,
			&cnt);
	printf(
			"Process %d: Received count message, original sender (coordinator) is process %d, cur count is %d\n\n",
			this->pid, coordinator, cnt);
	if (this->pid == this->coordinatorPid) { // stop message
		if (coordinator != this->coordinatorPid) // the process is the current coordinator, but this message was sent by an old coordinator
			return;
		this->hasStartedCounting = 0;
		this->processCount = cnt;
		this->sendData(coordinator, generateData());
		// TODO: data will be sent
	} else
		sendCount(coordinator, cnt + 1);
}

// TODO: Write documentation

void Process::receiveData(Message *msg) {
	stringstream ss(msg->mtext);
	int sender, coordinator;
	TIME timestamp;
	ss >> sender >> timestamp >> coordinator;
	printf(
			"Process %d: Received data message, original sender (coordinator) is process %d Data received is ",
			this->pid, coordinator);
	int curMin = INT32_MAX, element;
	for (int i = 0; i < PROCESSSEGSZ && (ss >> element); i++) {
		printf("%d ", element);
		curMin = min(curMin, element);
	}
	string restOfData, strElement;
	while (ss >> strElement) {
		restOfData += strElement, restOfData += ' ';
		printf("%s ", strElement.c_str());
	}
	puts("");
	printf("Process %d: Cur Min is %d\n\n", this->pid, curMin);
	char *restOfDataCSTR = new char[MSGMAXSZ];
	strcpy(restOfDataCSTR, restOfData.c_str());
	sendData(coordinator, restOfDataCSTR);
}
