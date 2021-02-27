#ifndef CONFIG_H_
#define CONFIG_H_

const int MAXPID = 1e6;

const char *const HEADSHM_FTOK_PATH = "./main.cpp";
const int HEADSHM_FTOK_PROJID = 5002;

const int PING_TIMEOUT = 5000;	// TODO: reduce timeout to reasonable value
const int HEARTBEAT_TIMEOUT = 5000;	// timeout to detect process death
const int HEARTBEAT_SEND_GAP = 1000;// time between consecutive heartbeats sent

#endif
