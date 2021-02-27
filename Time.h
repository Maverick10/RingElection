#ifndef TIME_H_
#define TIME_H_

#include <string>
#include <sys/timeb.h>
#include <time.h>
#include <sys/time.h>

using namespace std;

typedef long long TIME;

TIME getCurTime();
string getCurTimeFormatted();

#endif /* TIME_H_ */
