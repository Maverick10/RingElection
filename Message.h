#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Config.h"

struct Message {
	long mtype;
	char mtext[MSGMAXSZ];
};

#endif /* MESSAGE_H_ */
