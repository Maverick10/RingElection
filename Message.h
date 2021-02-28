#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Config.h"

/*
 * A structure to represent messages used in message queues
 */

struct Message {
	long mtype;	// type of message. refer to MessageType.h
	char mtext[MSGMAXSZ];	// message buffer. refer to Config.h to change MSGMAXSZ value
};

#endif /* MESSAGE_H_ */
