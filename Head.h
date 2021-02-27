#ifndef HEAD_H_
#define HEAD_H_

#include "Time.h"

struct Head {
	int id;
	int next;
	TIME timestamp;
	Head(int id, int next) :
			id(id), next(next) {
		timestamp = getCurTime();
	}
	Head() :
			id(0), next(0) {
		timestamp = getCurTime();
	}
};

#endif
