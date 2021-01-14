/*
 * Codecprintf.c
 * Created by Augie Fackler on 7/16/06.
 *
 * This file was part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Codecprintf.h"
#include <stdio.h>
#include <stdarg.h>
#include "libavutil/log.h"

#ifdef _SS_LOG_ACTIVE
#	include "CommonUtils.h"
#	define HAS_LOG_INIT
#	include "Logging.h"
#endif

#define CODEC_HEADER			"FFusion: "

static int Codecvprintf(FILE *fileLog, const char *format, va_list va, int print_header)
{
	int ret = 0;
#if _SS_LOG_ACTIVE
#	ifndef DEBUG_BUILD
	if( fileLog )
#	endif
	{
		if( !qtLog_Initialised ){
			qtLogPtr = Initialise_Log( "FFusion Codec Log", ProgName() );
			qtLog_Initialised = 1;
		}
		vcWriteLog( qtLogPtr, (char*) format, va );
	}
#else
	if( fileLog ){
		if( print_header ){
			fprintf(fileLog, CODEC_HEADER);
		}
		ret = vfprintf(fileLog, format, va);
		fflush(fileLog);
	}
	else{
#ifdef DEBUG_BUILD
		if( print_header ){
			printf(CODEC_HEADER);
		}
		ret = vprintf(format, va);
#endif
	}
#endif
	return ret;
}

#if defined(_MSC_VER) && defined(_SS_LOG_ACTIVE)
static const char *basename( const char *url )
{ const char *c = NULL;
	if( url ){
		if( (c =  strrchr( url, '\\' )) ){
			c++;
		}
		else{
			c = url;
		}
	}
	return c;
}
#endif

int _ffCodecprintf(const char *fileName, int lineNr, FILE *fileLog, const char *format, ...)
{int ret;
	va_list va;
#ifdef _SS_LOG_ACTIVE
	cLogStoreFileLine( basename(fileName), lineNr );
#endif
	va_start(va, format);
	ret = Codecvprintf(fileLog, format, va, !fileLog);
	va_end(va);
	return ret;
}

const char *FourCCString(FourCharCode c)
{
    static char fourcc[sizeof("0xFFFF")] = {0};
    int i;
    
	//not a fourcc or twocc
	if (c < '\0\0AA') {
#ifdef _MSC_VER
		_snprintf(fourcc, sizeof(fourcc), "%#x", (unsigned)c);
#else
		snprintf(fourcc, sizeof(fourcc), "%#x", (unsigned)c);
#endif
	} else {
		for (i = 0; i < 4; i++) fourcc[i] = c >> 8*(3-i);
		fourcc[4] = '\0';
	}
    
    return (char*)fourcc;
}

void FourCCprintf(const char *string, FourCharCode a)
{
    Codecprintf(NULL, "%s%s\n", string, FourCCString(a));
}

void FFMpegCodecprintf(void* ptr, int level, const char* fmt, va_list vl)
{/*
    static int print_prefix=1;
	int print_header = 1;
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

//    if(level>av_log_get_level())
//        return;

#ifdef _NSLOGGERCLIENT_H
	if( print_prefix ){
		//NSCodecvprintf( __FILE__, __LINE__, __FUNCTION__, 1, avc->item_name(ptr), avc, level, fmt, vl );
	}
	else{
		NSCodecvprintf( __FILE__, __LINE__, __FUNCTION__, 1, NULL, NULL, level, fmt, vl );
	}
#elif defined(_SS_LOG_ACTIVE)
    if( print_prefix && avc ){
	  char buf[256];
#ifdef _MSC_VER
		_snprintf( buf, sizeof(buf), ":%s 0x%lx lvl:", avc->item_name(ptr), (unsigned long) avc );
#else
		snprintf( buf, sizeof(buf), ":%s 0x%lx lvl:", avc->item_name(ptr), (unsigned long) avc );
#endif
		cLogStoreFileLine( buf, level );
		print_header = 0;
    }
	else{
		cLogStoreFileLine( "[AVLog]", level );
	}

	Codecvprintf(stderr, fmt, vl, print_header);
#endif
    print_prefix= strstr(fmt, "\n") != NULL;
*/}