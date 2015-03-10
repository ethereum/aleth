#include "CommonTime.h"

namespace dev
{

tm *timeToUTC(const time_t *_timeInput, struct tm *_result)
{
#ifdef __WIN32
	return gmtime_s(_result, _timeInput);
#else
	return gmtime_r(_timeInput, _result);
#endif
}

}
