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
#if TARGET_OS_MAC
#	include <pthread.h>
#else
#	include <windows.h>
#endif
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

#if TARGET_OS_MAC
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
#else
static int FFusionLockMgrCallback(void **mutex, enum AVLockOp op)
{
	CRITICAL_SECTION **m = (CRITICAL_SECTION **)mutex;
	int ret = 0;
	
	switch (op) {
		case AV_LOCK_CREATE:
			*m = malloc(sizeof(CRITICAL_SECTION));
			ret = !InitializeCriticalSectionAndSpinCount( *m, 4000 );
			break;
		case AV_LOCK_OBTAIN:
			EnterCriticalSection(*m);
			break;
		case AV_LOCK_RELEASE:
			LeaveCriticalSection(*m);
			break;
		case AV_LOCK_DESTROY:
			DeleteCriticalSection(*m);
			free(*m);
	}
	
	return ret;
}
#endif

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
		avcodec_register(&x##_decoder); }
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

// RJVB
#ifndef FFUSION_CODEC_ONLY
		REGISTER_DEMUXER(avi);
		REGISTER_DEMUXER(flv);
		REGISTER_DEMUXER(tta);
		REGISTER_DEMUXER(nuv);
		REGISTER_PARSER(ac3);
		REGISTER_PARSER(mpegaudio);
		
		REGISTER_DECODER(msmpeg4v1);
		REGISTER_DECODER(msmpeg4v2);
		REGISTER_DECODER(msmpeg4v3);
		REGISTER_DECODER(mpeg4);
		REGISTER_DECODER(h264);
		REGISTER_DECODER(hevc);
		REGISTER_DECODER(flv);
		REGISTER_DECODER(flashsv);
		REGISTER_DECODER(vp3);
		REGISTER_DECODER(vp6);
		REGISTER_DECODER(vp6f);
		REGISTER_DECODER(vp8);
		REGISTER_DECODER(vp9);
		REGISTER_DECODER(h263i);
		REGISTER_DECODER(huffyuv);
		REGISTER_DECODER(ffvhuff);
		REGISTER_DECODER(mpeg1video);
		REGISTER_DECODER(mpeg2video);
		REGISTER_DECODER(fraps);
		REGISTER_DECODER(snow);
		REGISTER_DECODER(nuv);
		REGISTER_DECODER(ffv1);
		REGISTER_DECODER(theora);
		
		REGISTER_DECODER(wmav1);
		REGISTER_DECODER(wmav2);
		REGISTER_DECODER(adpcm_swf);
		REGISTER_DECODER(vorbis);
		REGISTER_DECODER(mp1);
		REGISTER_DECODER(mp2);
		REGISTER_DECODER(tta);
		REGISTER_DECODER(dca);
		REGISTER_DECODER(nellymoser);
		REGISTER_DECODER(aac);
		
		REGISTER_DECODER(dvdsub);
		REGISTER_DECODER(tscc);
		REGISTER_DECODER(vp6a);
		REGISTER_DECODER(zmbv);
		REGISTER_DECODER(indeo2);
		REGISTER_DECODER(indeo3);
		REGISTER_DECODER(indeo5);
#else
		avcodec_register_all();
#endif

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

#ifndef FFUSION_CODEC_ONLY
// List of codec IDs we know about and that map to audio fourccs
// XXX this is probably a duplicate of something inside libavformat
static const struct {
	OSType mFormatID;
	enum CodecID codecID;
} kAudioCodecMap[] =
{
	{ kAudioFormatWMA1MS, CODEC_ID_WMAV1 },
	{ kAudioFormatWMA2MS, CODEC_ID_WMAV2 },
	{ kAudioFormatFlashADPCM, CODEC_ID_ADPCM_SWF },
	{ kAudioFormatXiphVorbis, CODEC_ID_VORBIS },
	{ kAudioFormatMPEGLayer1, CODEC_ID_MP1 },
	{ kAudioFormatMPEGLayer2, CODEC_ID_MP2 },
	{ kAudioFormatMPEGLayer3, CODEC_ID_MP3 },
	{ 'ms\0\0' + 0x50, CODEC_ID_MP2 },
	{ kAudioFormatDTS, CODEC_ID_DTS },
	{ kAudioFormatNellymoser, CODEC_ID_NELLYMOSER },
	{ kAudioFormatTTA, CODEC_ID_TTA },
	
	{ kAudioFormatAC3MS, CODEC_ID_AC3 },
	{ kAudioFormatLinearPCM, CODEC_ID_PCM_S16LE },
	{ kAudioFormatLinearPCM, CODEC_ID_PCM_U8 },
	{ kAudioFormatALaw, CODEC_ID_PCM_ALAW },
	{ kAudioFormatULaw, CODEC_ID_PCM_MULAW },
	{ kMicrosoftADPCMFormat, CODEC_ID_ADPCM_MS },
	{ kAudioFormatMPEG4AAC, CODEC_ID_AAC },
	{ kAudioFormatDTS, CODEC_ID_DTS },
	{ kAudioFormatFlashADPCM, CODEC_ID_ADPCM_SWF },
	{ 0, CODEC_ID_NONE }
};
#endif

enum AVCodecID FFFourCCToCodecID(OSType formatID)
{
#ifndef FFUSION_CODEC_ONLY
	for (int i = 0; kAudioCodecMap[i].codecID != CODEC_ID_NONE; i++) {
		if (kAudioCodecMap[i].mFormatID == formatID)
			return kAudioCodecMap[i].codecID;
	}
#endif
	return AV_CODEC_ID_NONE;
}

OSType FFCodecIDToFourCC(enum AVCodecID codecID)
{
#ifndef FFUSION_CODEC_ONLY
	for (int i = 0; kAudioCodecMap[i].codecID != CODEC_ID_NONE; i++) {
		if (kAudioCodecMap[i].codecID == codecID)
			return kAudioCodecMap[i].mFormatID;
	}
#endif
	return AV_CODEC_ID_NONE;
}
