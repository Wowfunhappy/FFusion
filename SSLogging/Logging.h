/*
 *  Logging.h
 *  QTImage2Mov
 *
 *  Created by Ren� J.V. Bertin on 20100713.
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *
 */

#ifndef _LOGGING_H

#include <stdio.h>

#	ifdef _PC_LOG_ACTIVE
#		ifndef _PC_LOG_EMBEDDED
#			include <LogController/PCLogController.h>
#		else
#			include "PCLogController.h"
#		endif
#		include <libgen.h>
//#		define	Log		fprintf
//#		define	qtLogPtr	stderr
		extern int PCLogStatus( id, const char*, int, NSString*, ... );
		extern BOOL PCLogActive();
		extern void *qtLogName();
//		extern void *nsString(char *s);
#		define	Log(d,f,...)	PCLogStatus((d),basename(__FILE__),__LINE__,nsString((char*)(f)), ##__VA_ARGS__)
#		define	qtLogPtr		qtLogName()
#	else
#		define	Log(d,f,...)	/**/
#		define	qtLogPtr		/**/
#	endif
#endif


#define _LOGGING_H
#endif
