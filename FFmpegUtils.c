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

static int FFusionLockMgrCallback(void **mutex, enum AVLockOp op)
{
	pthread_mutex_t **m = (pthread_mutex_t **)mutex;
	int ret = 0;
	
	switch (op) {
		case AV_LOCK_CREATE:
			*m = malloc(sizeof(pthread_mutex_t));
			ret = pthread_mutex_init(*m, NULL);
			break;
		case AV_LOCK_OBTAIN:
			ret = pthread_mutex_lock(*m);
			break;
		case AV_LOCK_RELEASE:
			ret = pthread_mutex_unlock(*m);
			break;
		case AV_LOCK_DESTROY:
			ret = pthread_mutex_destroy(*m);
			free(*m);
	}
	
	return ret;
}

#define REGISTER_MUXER(x) { \
	extern DLLIMPORT AVOutputFormat x##_muxer; \
		av_register_output_format(&x##_muxer); }
#define REGISTER_DEMUXER(x) { \
	extern DLLIMPORT AVInputFormat x##_demuxer; \
		av_register_input_format(&x##_demuxer); }
#define REGISTER_MUXDEMUX(x)  REGISTER_MUXER(x); REGISTER_DEMUXER(x)
#define REGISTER_PROTOCOL(x) { \
	extern DLLIMPORT URLProtocol x##_protocol; \
		register_protocol(&x##_protocol); }

#define REGISTER_ENCODER(x) { \
	extern DLLIMPORT AVCodec x##_encoder; \
		register_avcodec(&x##_encoder); }
#define REGISTER_DECODER(x) { \
	extern DLLIMPORT AVCodec x##_decoder; \
		/* avcodec_register deprecated in FFmpeg 4.x+ */ }
#define REGISTER_ENCDEC(x)  REGISTER_ENCODER(x); REGISTER_DECODER(x)

#define REGISTER_PARSER(x) { \
	extern DLLIMPORT AVCodecParser x##_parser; \
		av_register_codec_parser(&x##_parser); }
#define REGISTER_BSF(x) { \
	extern DLLIMPORT AVBitStreamFilter x##_bsf; \
		av_register_bitstream_filter(&x##_bsf); }

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
	
	/* Register the Parser of ffmpeg, needed because we do no proper setup of the libraries */
	if(!inited) {
		inited = TRUE;

		av_log_set_callback(FFMpegCodecprintf);

#if LIBAVCODEC_VERSION_MAJOR <= 52
		avcodec_init();
#endif
		av_lockmgr_register(FFusionLockMgrCallback);
		
		// Register only H265/HEVC, VP9, and AV1 decoders
		REGISTER_DECODER(ff_hevc);
		REGISTER_DECODER(ff_vp9);
		REGISTER_DECODER(ff_libdav1d);
		
		// avcodec_register_all() is deprecated in FFmpeg 4.x - codecs are auto-registered

		Codecprintf( stderr, "FFusion decoder using libavcodec, version %d.%d.%d (%u) / \"%s\"\n",
					LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO,
					avcodec_version(), avcodec_configuration() );
		cpuFlags = av_get_cpu_flags();
		cpuFlagString[0] = '\0';
		if( cpuFlags & AV_CPU_FLAG_MMX ){
			strncat( cpuFlagString, " MMX", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_MMX2 ){
			strncat( cpuFlagString, " MMX2", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_3DNOW ){
			strncat( cpuFlagString, " 3DNOW", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE ){
			strncat( cpuFlagString, " SSE", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE2 ){
			strncat( cpuFlagString, " SSE2", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE2SLOW ){
			strncat( cpuFlagString, " SSE2SLOW", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_3DNOWEXT ){
			strncat( cpuFlagString, " 3DNOWEXT", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE3 ){
			strncat( cpuFlagString, " SSE3", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE3SLOW ){
			strncat( cpuFlagString, " SSE3SLOW", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSSE3 ){
			strncat( cpuFlagString, " SSSE3", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE4 ){
			strncat( cpuFlagString, " SSE4", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		if( cpuFlags & AV_CPU_FLAG_SSE42 ){
			strncat( cpuFlagString, " SSE42", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}
		/*if( cpuFlags & AV_CPU_FLAG_IWMMXT ){
			strncat( cpuFlagString, " IWMMXT", sizeof(cpuFlagString) - strlen(cpuFlagString) - 1 );
		}*/
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
