#include "Process.h"

//Process::Process(bool b) { // debugging
//	this->pid = (rand() % MAXPID) + 1;
//	std::cout << "PID = " << pid << std::endl;
//	this->initShm();
//
//	// writeToShm(semAddress, pid, headShm);
//	Head x = readFromShm(semAddress, headShm);
//	std::cout << "Got value " << x.id << " " << x.next << std::endl;
//}

Process::Process() {
	this->pid = (rand() % MAXPID) + 1;
	this->isHead = 0;
	std::cout << "PID = " << pid << std::endl;
	this->initShm();

//	Head h(this->pid, this->pid);
//	writeToShm(semAddress, h, headShm);

	this->enterRing();
	printf("entered ring\n");
	this->lifeLoop();

//	std::cout << "Got value " << head.id << " " << head.next << std::endl;
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

void Process::sendChangeNextMsg(int first, int mid, int last) { // mid.next = last, first.next = mid
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
			this->sendChangeNextMsg(head.id, this->pid, head.next);	// this->next = head.next, head.id.next = this->pid
		} else if (this->pingProcess(head.next)) {// head is dead, must send a message to ring that this node is dead
			printf("Process %d is alive and well\n", head.next);
			this->appointAsHead(head.next);
			// TODO: cur process is new head
		} else {
			puts("All dead");
			this->appointAsHead(this->pid);
		}
	}
//	printf("exiting enterRing()");
}

void Process::lifeLoop() {
	while (1) {
		listenToQueue();
	}
}

void Process::listenToQueue() {

	Message *msg = new Message();
	int val = receiveMessage(this->pid, 0, msg);
	if (val >= 0) {	// received a message
		printf("Received %ld\n%s\n", msg->mtype, msg->mtext);
		printf("msgrcv val is %d\n", val);
		if (msg->mtype == MSGTYPE_PINGREQUEST) {
			int pingSender;
			sscanf(msg->mtext, "%d", &pingSender);
			Message *pingReply = new Message();
			pingReply->mtype = MSGTYPE_PINGREPLY;
			sprintf(pingReply->mtext, "%d %lld", this->pid, getCurTime());
			sendMessage(pingSender, pingReply);
			delete pingReply;
		} else if (msg->mtype == MSGTYPE_CHANGENEXT) {
			int newNext;
			sscanf(msg->mtext, "%d %d %d", &newNext, &newNext, &newNext);
			this->next = newNext;
			if (this->isHead)
				this->appointAsHead(this->next);
			printf("Changed next to process %d\n", this->next);
		}
	}
//		break;
	delete msg;

}

bool Process::pingProcess(int pid) {
	Message *msg = new Message();
	msg->mtype = MSGTYPE_PINGREQUEST;
	TIME msgTime = getCurTime();
	sprintf(msg->mtext, "%d %lld", this->pid, msgTime);
	printf("sent message is %ld %s\n", msg->mtype, msg->mtext);
	sendMessage(pid, msg);

	while (getCurTime() - msgTime <= PING_TIMEOUT) {// wait till you receive a reply
		int val = receiveMessage(this->pid, MSGTYPE_PINGREPLY, msg);
		if (val >= 0) {	// reply received
			printf("Received %ld\n%s\n", msg->mtype, msg->mtext);
			printf("msgrcv val is %d\n", val);
			int pingReplySender;
			sscanf(msg->mtext, "%d", &pingReplySender);
			if (pingReplySender == pid) // reply was successful
				return 1;
		}
		sleep(1);
	}
	return 0;
//	std::cout << "sent message to head to change next\n";
}
