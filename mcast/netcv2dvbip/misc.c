#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include <pthread.h>
#include "misc.h"

void log_socket_error(const char* msg)
{
#ifdef WIN32
        printf(msg);
        printf(": WSAGetLastError(): %d",  WSAGetLastError());
#else
        perror(msg);
#endif
}

#ifdef WIN32
static const unsigned __int64 epoch = 116444736000000000;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    FILETIME	file_time;
    SYSTEMTIME	system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

    return 0;
}
#endif

bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) {           // get current time
     now.tv_sec  += MillisecondsFromNow / 1000;  // add full seconds
     now.tv_usec += (MillisecondsFromNow % 1000) * 1000;  // add microseconds
     if (now.tv_usec >= 1000000) {               // take care of an overflow
        now.tv_sec++;
        now.tv_usec -= 1000000;
        }
     Abstime->tv_sec = now.tv_sec;          // seconds
     Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
     return true;
     }
  return false;
}

// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
  Set(Ms);
}

uint64_t cTimeMs::Now(void)
{
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  return Now() - begin;
}
