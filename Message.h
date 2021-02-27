#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Time.h"

struct Message {
	long mtype;
	char mtext[1024];
};

#endif /* MESSAGE_H_ */
