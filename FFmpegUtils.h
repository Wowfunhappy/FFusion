/*
 * FFmpegUtils.h
 * Created by Alexander Strange on 11/7/10.
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

/*
 * FFmpeg API interfaces and data structure conversion
 */

#ifndef __FFMPEGUTILS__
#define __FFMPEGUTILS__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#ifdef _MSC_VER
	// prevent the GNU compatibility stdint.h header included with the QuickTime SDK from being included:
#	define _STDINT_H
#endif
#ifdef __MACH__
#	include <QuickTime/QuickTime.h>
#else
#	include <ConditionalMacros.h>
#	include <Endian.h>
#	include <ImageCodec.h>
#endif
//#ifndef FFUSION_CODEC_ONLY
#	include <CoreAudio/CoreAudio.h>
//#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"

#ifndef FFUSION_CODEC_ONLY
void FFAVCodecContextToASBD(AVCodecContext *avctx, AudioStreamBasicDescription *asbd);

void FFASBDToAVCodecContext(AudioStreamBasicDescription *asbd, AVCodecContext *avctx);
#endif

void FFInitFFmpeg();

enum AVCodecID FFFourCCToCodecID(OSType formatID);
OSType FFCodecIDToFourCC(enum AVCodecID codecID);

#ifdef __cplusplus
}
#endif

#endif
