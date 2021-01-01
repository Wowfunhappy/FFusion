/*
 * ColorConversions.h
 * Created by Alexander Strange on 1/10/07.
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

#ifndef __COLORCONVERSIONS_H__
#define __COLORCONVERSIONS_H__

#if TARGET_OS_MAC
#	include <Carbon/Carbon.h>
#endif
#include "libavcodec/avcodec.h"

#ifdef _MSCVER
#	define FASTCALL	__fastcall
#elif defined(__i386__) && !defined(__llvm__) && !defined(_MSC_VER)
#	define FASTCALL	__attribute__((fastcall))
#else
#	define FASTCALL
#endif

typedef void ColorConversionFunc(AVPicture *inPicture, UInt8 *outBaseAddr, long outRowBytes, int outWidth, int outHeight) FASTCALL;
typedef void ColorClearFunc(UInt8 *outBaseAddr, long outRowBytes, int outWidth, int outHeight) FASTCALL;

typedef ColorConversionFunc *ColorConversionFuncPtr;
typedef ColorClearFunc *ColorClearFuncPtr;

typedef struct ColorConversionFuncs {
	ColorConversionFuncPtr convert;
	ColorClearFuncPtr      clear;
} ColorConversionFuncs;

extern OSType ColorConversionDstForPixFmt(enum AVCodecID codecID, enum PixelFormat ffPixFmt);
extern int ColorConversionFindFor( ColorConversionFuncs *funcs, enum AVCodecID codecID, enum PixelFormat ffPixFmt,
								  AVPicture *ffPicture, OSType qtPixFmt, UInt8 *baseAddr, long rowBytes, int width, int height );

#endif
