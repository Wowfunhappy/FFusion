#include <asl.h>

/*
 * FFmpegUtils.c
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
 * FFmpeg API interfaces and such
 */

#include "FFmpegUtils.h"
#include "CommonUtils.h"
#include "Codecprintf.h"
#include "CodecIDs.h"
#include <pthread.h>
#ifndef FFUSION_CODEC_ONLY
#	include "libavformat/avformat.h"
#endif
#include "libavcodec/avcodec.h"

#ifndef FFUSION_CODEC_ONLY
void FFAVCodecContextToASBD(AVCodecContext *avctx, AudioStreamBasicDescription *asbd)
{
	asbd->mFormatID         = avctx->codec_tag;
	asbd->mSampleRate       = avctx->sample_rate;
	asbd->mChannelsPerFrame = avctx->channels;
	asbd->mBytesPerPacket   = avctx->block_align;
	asbd->mFramesPerPacket  = avctx->frame_size;
	asbd->mBitsPerChannel   = avctx->bits_per_coded_sample;
}

void FFASBDToAVCodecContext(AudioStreamBasicDescription *asbd, AVCodecContext *avctx)
{
	avctx->codec_tag   = asbd->mFormatID;
	avctx->sample_rate = asbd->mSampleRate;
	avctx->channels    = asbd->mChannelsPerFrame;
	avctx->block_align = asbd->mBytesPerPacket;
	avctx->frame_size  = asbd->mFramesPerPacket;
	avctx->bits_per_coded_sample = asbd->mBitsPerChannel;
}
#endif

// Lock manager callback removed - FFmpeg 4.x+ handles threading internally

// Registration macros removed - FFmpeg 4.x+ auto-registers codecs, parsers, and filters

void FFInitFFmpeg()
{
	/* This one is used because Global variables are initialized ONE time
	* until the application quits. Thus, we have to make sure we're initialize
	* the libavformat only once or we get an endlos loop when registering the same
	* element twice!! */
	static Boolean inited = FALSE;
	int unlock = FFusionInitEnter(&inited);
	int cpuFlags;
	char cpuFlagString[1024];
	
	/* Initialize FFmpeg logging and CPU detection */
	if(!inited) {
		inited = TRUE;

		av_log_set_callback(FFMpegCodecprintf);
		

		Codecprintf( stderr, "FFusion decoder using libavcodec, version %d.%d.%d (%u) / \"%s\"\n",
					LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO,
					avcodec_version(), avcodec_configuration() );
		cpuFlags = av_get_cpu_flags();
		cpuFlagString[0] = '\0';
		if( cpuFlags & AV_CPU_FLAG_SSE2 ){
			strncat( cpuFlagString, " SSE2", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE3 ){
			strncat( cpuFlagString, " SSE3", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSSE3 ){
			strncat( cpuFlagString, " SSSE3", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE4 ){
			strncat( cpuFlagString, " SSE4.1", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE42 ){
			strncat( cpuFlagString, " SSE4.2", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_AVX ){
			strncat( cpuFlagString, " AVX", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_AVX2 ){
			strncat( cpuFlagString, " AVX2", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_AVX512 ){
			strncat( cpuFlagString, " AVX-512", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		Codecprintf( stderr, "Extensions supported by the current CPU: %s\n", cpuFlagString );
#ifdef _NSLOGGERCLIENT_H
		NSCodecFlushLog();
#endif
	}
	
	FFusionInitExit(unlock);
}


enum AVCodecID FFFourCCToCodecID(OSType formatID)
{
	// FFusion only supports video codecs (H265/VP9)
	return AV_CODEC_ID_NONE;
}

OSType FFCodecIDToFourCC(enum AVCodecID codecID)
{
	// FFusion only supports video codecs (H265/VP9)
	return AV_CODEC_ID_NONE;
}
