#ifndef CONFIG_H_
#define CONFIG_H_

const int MAXPID = 1e6;

const int MSGMAXSZ = 1024; // maximum size for message mtext used in message queues

const int PROCESSSEGSZ = 5;	// number of elements that a single process deals with when coordinator sends data

const int ELEMENT_MAX = 10;

const char *const HEADSHM_FTOK_PATH = "./Process.cpp";
const int HEADSHM_FTOK_PROJID = 5008;

const int PING_TIMEOUT = 1000;	// TODO: reduce timeout to reasonable value
const int HEARTBEAT_TIMEOUT = 5000;	// timeout to detect process death
const int MSG_TIMEOUT = 15000; // to stop process death message from going around the ring forever

const int HEARTBEAT_SEND_GAP = 1000; // time between consecutive heartbeats sent
const int DEATHNOTE_SEND_GAP = 1000; // time between consecutive death notes
const int DATA_SEND_GAP = 20000; // time between consecutive data sent by coordinator

#endif
