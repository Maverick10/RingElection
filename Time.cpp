#include "Time.h"

/*
 * Gets current time in milliseconds
 *
 * returns: current time in milliseconds
 */

TIME getCurTime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000ll + tp.tv_usec / 1000; //get current timestamp in milliseconds
}

string getCurTimeFormatted() {
	time_t now = time(0);

	char timebuf[20] = { 0 };

	strftime(timebuf, 20, "%H.%M.%S", localtime(&now));
	return timebuf;
}

