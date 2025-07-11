/*!
 *  @file QTImage2Mov/source/timing.c
 *
 *  (C) RenE J.V. Bertin on 20080926.
 *
 */

//#include <stdio.h>

#ifdef __MACH__
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <mach/mach_init.h>
#	include <sys/sysctl.h>
#endif
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "timing.h"

#if defined(__MACH__)
#pragma mark ---MacOSX---

static mach_timebase_info_data_t sTimebaseInfo;
static double calibrator= 0;

void init_HRTime()
{
	if( !calibrator ){
		mach_timebase_info(&sTimebaseInfo);
		  /* go from absolute time units to seconds (the timebase is calibrated in nanoseconds): */
		calibrator= 1e-9 * sTimebaseInfo.numer / sTimebaseInfo.denom;
	}
}

double HRTime_Time()
{
	return mach_absolute_time() * calibrator;
}

static double ticTime;
double HRTime_tic()
{
	return( ticTime= mach_absolute_time() * calibrator );
}

double HRTime_toc()
{
	return mach_absolute_time() * calibrator - ticTime;
}


#elif defined(linux)
#pragma mark ---linux---

#	if defined(CLOCK_MONOTONIC)

	static inline double gettime()
	{ struct timespec hrt;
		clock_gettime( CLOCK_MONOTONIC, &hrt );
		return hrt.tv_sec + hrt.tv_nsec * 1e-9;
	}

	static inline double clock_get_res()
	{ struct timespec hrt;
		clock_getres( CLOCK_MONOTONIC, &hrt );
		return hrt.tv_sec + hrt.tv_nsec * 1e-9;
	}

#	elif defined(CLOCK_REALTIME)

	static inline double gettime()
	{ struct timespec hrt;
		clock_gettime( CLOCK_REALTIME, &hrt );
		return hrt.tv_sec + hrt.tv_nsec * 1e-9;
	}

	static inline double clock_get_res()
	{ struct timespec hrt;
		clock_getres( CLOCK_REALTIME, &hrt );
		return hrt.tv_sec + hrt.tv_nsec * 1e-9;
	}


#	elif defined(CPUCLOCK_CYCLES_PER_SEC)

	typedef unsigned long long tsc_time;

	typedef struct tsc_timers{
		tsc_time t1, t2;
	} tsc_timers;

	static __inline__ tsc_time read_tsc()
	{ tsc_time ret;

		__asm__ __volatile__("rdtsc": "=A" (ret)); 
		/* no input, nothing else clobbered */
		return ret;
	}

#	define tsc_get_time(t)  ((*(tsc_time*)t)=read_tsc())
#	define tsc_time_to_sec(t) (((double) (*t)) / CPUCLOCK_CYCLES_PER_SEC)

	static inline gettime()
#		ifdef CPUCLOCK_CYCLES_PER_SEC
	{ tsc_time t;
		tsc_get_time(&t);
		return tsc_time_to_sec( &t );
	}
#		else
		  /* Use gettimeofday():	*/
	{ struct timezone tzp;
	  struct timeval ES_tv;

		gettimeofday( &ES_tv, &tzp );
		return ES_tv.tv_sec + ES_tv.tv_usec* 1e-6;
	}
#		endif

#	endif

void init_HRTime()
{
#if defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME)
	// just for kicks, probably not necessary:
	clock_get_res();
#endif
}

double HRTime_Time()
{
	return( gettime() );
}

static double ticTime;
double HRTime_tic()
{
	return( ticTime= gettime() );
}

double HRTime_toc()
{
	return gettime() - ticTime;
}


#else
#pragma mark ---using gettimeofday---

	  /* Use gettimeofday():	*/
#define gettime(time)	{ struct timezone tzp; \
	  struct timeval ES_tv; \
		gettimeofday( &ES_tv, &tzp ); \
		time= ES_tv.tv_sec + ES_tv.tv_usec* 1e-6; \
	}

void init_HRTime()
{
}

double HRTime_Time()
{ double time;
	gettime(time);
	return( time );
}

static double ticTime;
double HRTime_tic()
{
	gettime(ticTime);

	return( ticTime );
}

double HRTime_toc()
{ double time;
	gettime(time);
	return time - ticTime;
}

#endif
