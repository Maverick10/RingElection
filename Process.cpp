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
	std::cout << "PID = " << pid << std::endl;
	this->initShm();

//	Head h(this->pid, this->pid);
//	writeToShm(semAddress, h, headShm);

	this->enterRing();
	printf("entered ring\n");
	this->listenToQueue();

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

void Process::enterRing() {
	Head head = readFromShm(semAddress, this->headShm);
	if (head.id == 0) {	// shm is clean, this is the first process
		std::cout << "Shm is clean, wrote to shm\n";
		Head entry(this->pid, this->pid);
		writeToShm(semAddress, entry, this->headShm);
		this->next = this->pid;
	} else {
		if (this->pingProcess(head.id)) {
			printf("Process %d is alive and well\n", head.id);
		} else if (this->pingProcess(head.next)) {
			printf("Process %d is alive and well\n", head.next);
		} else {
			puts("All dead");
		}
	}
//	printf("exiting enterRing()");
}

void Process::listenToQueue() {
	while (1) {
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
			}
		}
//		break;
	}
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
