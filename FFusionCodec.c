#include <asl.h>

//---------------------------------------------------------------------------
//FFusion
//Alternative DivX Codec for MacOS X
//version 2.2 (build 72)
//by Jerome Cornet
//Copyright 2002-2003 Jerome Cornet
//parts by Dan Christiansen
//Copyright 2003 Dan Christiansen
//from DivOSX by Jamby
//Copyright 2001-2002 Jamby
//uses libavcodec from ffmpeg 0.4.6
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//---------------------------------------------------------------------------
// Source Code
//---------------------------------------------------------------------------

//#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#ifdef _MSC_VER
// prevent the GNU compatibility stdint.h header included with the QuickTime SDK from being included:
#	define _STDINT_H
#endif
#ifdef __MACH__
#	include <QuickTime/QuickTime.h>
#	include <sys/sysctl.h>
#else
#	include <windows.h>
#	include <ConditionalMacros.h>
#	include <Endian.h>
#	include <ImageCodec.h>
#endif
//#include <Accelerate/Accelerate.h>

//RJVB
#include "FFusionResourceIDs.h"
#include "libavcodec/avcodec.h"
#include "Codecprintf.h"
#include "ColorConversions.h"
#include "bitstream_info.h"
#include "FrameBuffer.h"
#include "CommonUtils.h"
#include "CodecIDs.h"
#include "FFmpegUtils.h"

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------

// 64 because that's 2 * ffmpeg's INTERNAL_BUFFER_SIZE and QT sometimes uses more than 32
#define FFUSION_MAX_BUFFERS 64

#define kNumPixelFormatsSupportedFFusion 1

typedef struct
{
	AVFrame		*frame;
	AVPicture	picture;
	int		retainCount;
	int		frameNumber;
} FFusionBuffer;

typedef enum
{
	PACKED_QUICKTIME_KNOWS_ORDER, /* This is for non-stupid container formats which actually know the difference between decode and display order */
	PACKED_ALL_IN_FIRST_FRAME, /* This is for the divx hack which contains a P frame and all subsequent B frames in a single frame. Ex: I, PB, -, PB, -, I...*/
	PACKED_DELAY_BY_ONE_FRAME /* This is for stupid containers where frames are soley writen in decode order.  Ex: I, P, B, P, B, P, I.... */
} FFusionPacked;

/* Why do these small structs?  It makes the usage of these variables clearer, no other reason */

/* globs used by the BeginBand routine */
struct begin_glob
{
	FFusionParserContext	*parser;
	int			lastFrame;
	int			lastIFrame;
	int			lastFrameType;
	int			futureType;
	FrameData	*lastPFrameData;
};

/* globs used by the DecodeBand routine */
struct decode_glob
{
	int				lastFrame;
	FFusionBuffer	*futureBuffer;
};

struct per_frame_decode_stats
{
	int		begin_calls;
	int		decode_calls;
	int		draw_calls;
	int		end_calls;
};

struct decode_stats
{
	struct per_frame_decode_stats type[4];
	int		max_frames_begun;
	int		max_frames_decoded;
};

typedef struct
{
    ComponentInstance		self;
    ComponentInstance		delegateComponent;
    ComponentInstance		target;
    ImageCodecMPDrawBandUPP 	drawBandUPP;
    Handle			pixelTypes;
    AVCodec			*avCodec;
    AVCodecContext	*avContext;
    OSType			componentType;
	FILE			*fileLog;
	AVPicture		*lastDisplayedPicture;
	FFusionPacked	packedType;
	FFusionBuffer	buffers[FFUSION_MAX_BUFFERS];	// the buffers which the codec has retained
	int				lastAllocatedBuffer;		// the index of the buffer which was last allocated
	// by the codec (and is the latest in decode order)
	struct begin_glob	begin;
	FFusionData		data;
	struct decode_glob	decode;
	struct decode_stats stats;
	ColorConversionFuncs colorConv;
	int isFrameDroppingEnabled, isForcedDecodeEnabled;
} FFusionGlobalsRecord, *FFusionGlobals;

typedef struct
{
    int			width;
    int			height;
    int			depth;
    OSType		pixelFormat;
	int			decoded;
	int			frameNumber;
	int			GOPStartFrameNumber;
	int			bufferSize;
	FFusionBuffer	*buffer;
	FrameData		*frameData;
} FFusionDecompressRecord;


//---------------------------------------------------------------------------
// Prototypes of private subroutines
//---------------------------------------------------------------------------

static OSErr FFusionDecompress(FFusionGlobals glob, AVCodecContext *context, UInt8 *dataPtr, int width, int height, AVFrame *picture, int length);
static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic);
static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic);
static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf);
static void releaseBuffer(AVCodecContext *s, AVFrame *pic);

int GetPPUserPreference();
void SetPPUserPreference(int value);
pascal OSStatus HandlePPDialogWindowEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
pascal OSStatus HandlePPDialogControlEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
void ChangeHintText(int value, ControlRef staticTextField);

//#define FFusionDebugPrint(x...) if (glob->fileLog) Codecprintf(glob->fileLog, x);
#define FFusionDebugPrint(x,...) Codecprintf(glob->fileLog, x, ##__VA_ARGS__)
#define FFusionDebugPrint2(x,...) Codecprintf(((glob->fileLog)?glob->fileLog:stderr), x, ##__VA_ARGS__)
#define not(x) ((x) ? "" : "not ")

//---------------------------------------------------------------------------
// Component Dispatcher
//---------------------------------------------------------------------------

#define IMAGECODEC_BASENAME() 		FFusionCodec
#define IMAGECODEC_GLOBALS() 		FFusionGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"FFusionCodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#if __MACH__
#	include <CoreServices/Components.k.h>
#	include <QuickTime/ImageCodec.k.h>
#	include <QuickTime/ComponentDispatchHelper.c>
#else
#	include <Components.k.h>
#	include <ImageCodec.k.h>
#	include <ComponentDispatchHelper.c>
#endif

static void RecomputeMaxCounts(FFusionGlobals glob)
{
	int i;
	int begun = 0, decoded = 0, ended = 0, drawn = 0;
	
	for (i = 0; i < 4; i++) {
		struct per_frame_decode_stats *f = &glob->stats.type[i];
		
		begun += f->begin_calls;
		decoded += f->decode_calls;
		drawn += f->draw_calls;
		ended += f->end_calls;
	}
	
	{ signed begin_diff = begun - ended, decode_diff = decoded - drawn;
		
		if (abs(begin_diff) > glob->stats.max_frames_begun) glob->stats.max_frames_begun = begin_diff;
		if (abs(decode_diff) > glob->stats.max_frames_decoded) glob->stats.max_frames_decoded = decode_diff;
	}
}

static void DumpFrameDropStats(FFusionGlobals glob)
{
	static const char types[4] = {'?', 'I', 'P', 'B'};
	int i;
	
	if (!glob->fileLog || glob->decode.lastFrame == 0) return;
	
	Codecprintf( glob->fileLog,
				"'%s' frame drop stats\nType    | BeginBands    | DecodeBands    | DrawBands    | # before decode    | # before draw\n",
				FourCCString(glob->componentType) );
	
	for (i = 0; i < 4; i++) {
		struct per_frame_decode_stats *f = &glob->stats.type[i];
		
		Codecprintf(glob->fileLog, "%c    | %d        | %d        | %d        | %d/%g%%        | %d/%g%%\n", types[i], f->begin_calls, f->decode_calls, f->draw_calls,
					f->begin_calls - f->decode_calls,(f->begin_calls > f->decode_calls) ? ((float)(f->begin_calls - f->decode_calls)/(float)f->begin_calls) * 100. : 0.,
					f->decode_calls - f->draw_calls,(f->decode_calls > f->draw_calls) ? ((float)(f->decode_calls - f->draw_calls)/(float)f->decode_calls) * 100. : 0.);
	}
}


static enum AVPixelFormat FindPixFmtFromVideo(AVCodec *codec, AVCodecContext *avctx, Ptr data, int bufferSize)
{
    AVCodecContext *tmpContext;
    AVFrame *tmpFrame;
	int got_picture = 0;
    enum AVPixelFormat pix_fmt;
	
	tmpContext = avcodec_alloc_context3(codec);
	tmpFrame   = av_frame_alloc();
	avcodec_copy_context(tmpContext, avctx);

    if (avcodec_open2(tmpContext, codec, NULL)) {
		pix_fmt = AV_PIX_FMT_NONE;
		goto bail;
	}
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (UInt8*)data;
	pkt.size = bufferSize;
	int len = avcodec_decode_video2(tmpContext, tmpFrame, &got_picture, &pkt);
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "Decoded frame to find pix_fmt, got %d", len);
    pix_fmt = tmpContext->pix_fmt;
    avcodec_close(tmpContext);
bail:
	av_frame_free(&tmpFrame);
    return pix_fmt;
}

static void SetupMultithreadedDecoding(AVCodecContext *s, enum AVCodecID codecID)
{
	int nthreads = 1;
	size_t len = 4;
	
    // multithreading is only effective for mpeg1/2 and h.264 with slices
	//    if (codecID != CODEC_ID_MPEG1VIDEO && codecID != CODEC_ID_MPEG2VIDEO && codecID != CODEC_ID_H264) return;
	
	// two threads on multicore, otherwise 1
#if TARGET_OS_MAC
	if (sysctlbyname("hw.activecpu", &nthreads, &len, NULL, 0) == -1) nthreads = 1;
#else
	{ SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		nthreads = sysinfo.dwNumberOfProcessors;
	}
#endif
	//	nthreads = FFMIN(nthreads, 2);
	
#if LIBAVCODEC_VERSION_MAJOR > 52
	s->thread_count = nthreads;
#else
	avcodec_thread_init(s, nthreads);
#endif
}

static void SetSkipLoopFilter(FFusionGlobals glob, AVCodecContext *avctx)
{
	Boolean keyExists = FALSE;
	int loopFilterSkip = CFPreferencesGetAppIntegerValue(CFSTR("SkipLoopFilter"), FFUSION_PREF_DOMAIN, &keyExists);
	if(keyExists)
	{
		enum AVDiscard loopFilterValue = AVDISCARD_DEFAULT;
		switch (loopFilterSkip) {
			case 1:
				loopFilterValue = AVDISCARD_NONREF;
				break;
			case 2:
				loopFilterValue = AVDISCARD_BIDIR;
				break;
			case 3:
				loopFilterValue = AVDISCARD_NONKEY;
				break;
			case 4:
				loopFilterValue = AVDISCARD_ALL;
				break;
			default:
				break;
		}
		avctx->skip_loop_filter = loopFilterValue;
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Preflight set skip loop filter to %d", glob, loopFilterValue);
	}
}

// A list of codec types (mostly official Apple codecs) which always have DTS info.
// This is the wrong way to do it, instead we should check for a ctts atom directly.
// This way causes files to play frames out of order if we guess wrong. Doesn't seem
// possible to do it right, though.
FFusionPacked DefaultPackedTypeForCodec(OSType codec)
{
	switch (codec) {
		case kMPEG1VisualCodecType:
		case kMPEG2VisualCodecType:
		case 'hdv1':
		case kMPEG4VisualCodecType:
		case kH264CodecType:
			return PACKED_QUICKTIME_KNOWS_ORDER;
		default:
			return PACKED_ALL_IN_FIRST_FRAME;
	}
}

void setFutureFrame(FFusionGlobals glob, FFusionBuffer *newFuture)
{
	FFusionBuffer *temp = glob->decode.futureBuffer;
	if(newFuture != NULL)
		retainBuffer(glob, newFuture);
	glob->decode.futureBuffer = newFuture;
	if(temp != NULL)
		releaseBuffer(glob->avContext, temp->frame);
}

//---------------------------------------------------------------------------
// Component Routines
//---------------------------------------------------------------------------

// -- This Image Decompressor Use the Base Image Decompressor Component --
//	The base image decompressor is an Apple-supplied component
//	that makes it easier for developers to create new decompressors.
//	The base image decompressor does most of the housekeeping and
//	interface functions required for a QuickTime decompressor component,
//	including scheduling for asynchronous decompression.

//-----------------------------------------------------------------
// Component Open Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecOpen(FFusionGlobals glob, ComponentInstance self)
{
    ComponentResult err;
    ComponentDescription descout;
	
    GetComponentInfo((Component)self, &descout, 0, 0, 0);
	
	//  FourCCprintf("Opening component for type ", descout.componentSubType);
    // Allocate memory for our globals, set them up and inform the component manager that we've done so
	
    glob = (FFusionGlobals)NewPtrClear(sizeof(FFusionGlobalsRecord));
	
    if ((err = MemError()))
    {
        Codecprintf( stderr, "Unable to allocate globals! Exiting.\n");
    }
    else
    {
#if TARGET_OS_MAC
		CFStringRef pathToLogFile = CopyPreferencesValueTyped(CFSTR("DebugLogFile"), CFStringGetTypeID());
#endif
		char path[1024];
        SetComponentInstanceStorage(self, (Handle)glob);
		
        glob->self = self;
        glob->target = self;
        glob->drawBandUPP = NULL;
        glob->pixelTypes = NewHandleClear((kNumPixelFormatsSupportedFFusion+1) * sizeof(OSType));
        glob->avCodec = 0;
        glob->componentType = descout.componentSubType;
		glob->data.frames = NULL;
		glob->begin.parser = NULL;
#if TARGET_OS_MAC
		if (pathToLogFile) {
			CFStringGetCString(pathToLogFile, path, sizeof(path), kCFStringEncodingUTF8);
			CFRelease(pathToLogFile);
			glob->fileLog = fopen(path, "a");
		}
#else
		glob->fileLog = NULL;
#endif
		
        // Open and target an instance of the base decompressor as we delegate
        // most of our calls to the base decompressor instance
		
        err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
        if (!err)
        {
            if( ComponentSetTarget(glob->delegateComponent, self) == badComponentSelector ){
				asl_log(NULL, NULL, ASL_LEVEL_ERR, "Error %d targeting the base image decompressor with ourselves!\n", err );
			}
        }
        else
        {
            asl_log(NULL, NULL, ASL_LEVEL_ERR, "Error %d opening the base image decompressor!\n", err );
        }
		
		// we allocate some space for copying the frame data since we need some padding at the end
		// for ffmpeg's optimized bitstream readers. Size doesn't really matter, it'll grow if need be
		FFusionDataSetup(&(glob->data), 256, 1024*1024);
		glob->isFrameDroppingEnabled = IsFrameDroppingEnabled();
		glob->isForcedDecodeEnabled = IsForcedDecodeEnabled();
    }
	
    asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p opened for '%s'\n", glob, FourCCString(glob->componentType));
	//    Codecprintf( NULL, "%p opened for '%s'\n", glob, FourCCString(glob->componentType));
    return err;
}

//-----------------------------------------------------------------
// Component Close Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecClose(FFusionGlobals glob, ComponentInstance self)
{
    // Make sure to close the base component and deallocate our storage
    if (glob)
    {
        if (glob->delegateComponent)
        {
            CloseComponent(glob->delegateComponent);
        }
		
        if (glob->drawBandUPP)
        {
            DisposeImageCodecMPDrawBandUPP(glob->drawBandUPP);
        }
		setFutureFrame(glob, NULL);
		
        if (glob->avContext)
        { FILE *fp = glob->fileLog;
			ComponentDescription cDescr;
			Handle cName = NewHandleClear(200);
			if( !glob->fileLog ){
				glob->fileLog = stderr;
			}
			DumpFrameDropStats(glob);
			glob->fileLog = fp;
			if (glob->avContext->extradata)
				free(glob->avContext->extradata);
			
			if (glob->avContext->codec){
				avcodec_close(glob->avContext);
			}
            av_free(glob->avContext);
			if( GetComponentInfo( (Component) self, &cDescr, cName, NULL, NULL ) == noErr ){
				char tStr[5];
				strcpy( tStr, FourCCString(cDescr.componentSubType) );
				FFusionDebugPrint( "%p closed - '%s' type '%s' by '%s'\n", glob,
								  &(*cName)[1], tStr, FourCCString(cDescr.componentManufacturer) );
			}
			else{
				asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p closed.\n", glob);
			}
			DisposeHandle(cName);
        }
		
		if (glob->begin.parser)
		{
			ffusionParserFree(glob->begin.parser);
		}
		
		if (glob->pixelTypes)
		{
			DisposeHandle(glob->pixelTypes);
		}
		
		FFusionDataFree(&(glob->data));
		
		if(glob->fileLog && glob->fileLog != stderr ){
			fclose(glob->fileLog);
		}
		
        memset(glob, 0, sizeof(FFusionGlobalsRecord));
        DisposePtr((Ptr)glob);
    }
	
    return noErr;
}

//-----------------------------------------------------------------
// Component Version Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecVersion(FFusionGlobals glob)
{
    return kFFusionCodecVersion;
}

//-----------------------------------------------------------------
// Component Target Request
//-----------------------------------------------------------------
// Allows another component to "target" you i.e., you call
// another component whenever you would call yourself (as a result
// of your component being used by another component)
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecTarget(FFusionGlobals glob, ComponentInstance target)
{
    glob->target = target;
	
    return noErr;
}

//-----------------------------------------------------------------
// Component GetMPWorkFunction Request
//-----------------------------------------------------------------
// Allows your image decompressor component to perform asynchronous
// decompression in a single MP task by taking advantage of the
// Base Decompressor. If you implement this selector, your DrawBand
// function must be MP-safe. MP safety means not calling routines
// that may move or purge memory and not calling any routines which
// might cause 68K code to be executed. Ideally, your DrawBand
// function should not make any API calls whatsoever. Obviously
// don't implement this if you're building a 68k component.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecGetMPWorkFunction(FFusionGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if (glob->drawBandUPP == NULL)
		glob->drawBandUPP = NewImageCodecMPDrawBandUPP((ImageCodecMPDrawBandProcPtr)FFusionCodecDrawBand);
	
	return ImageCodecGetBaseMPWorkFunction(glob->delegateComponent, workFunction, refCon, glob->drawBandUPP, glob);
}

//-----------------------------------------------------------------
// ImageCodecInitialize
//-----------------------------------------------------------------
// The first function call that your image decompressor component
// receives from the base image decompressor is always a call to
// ImageCodecInitialize . In response to this call, your image
// decompressor component returns an ImageSubCodecDecompressCapabilities
// structure that specifies its capabilities.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecInitialize(FFusionGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
#if TARGET_OS_MAC
	Boolean doExperimentalFlags = CFPreferencesGetAppBooleanValue(CFSTR("ExperimentalQTFlags"), FFUSION_PREF_DOMAIN, NULL);
#else
	Boolean doExperimentalFlags = FALSE;
#endif
	
    // Specifies the size of the ImageSubCodecDecompressRecord structure
    // and say we can support asyncronous decompression
    // With the help of the base image decompressor, any image decompressor
    // that uses only interrupt-safe calls for decompression operations can
    // support asynchronous decompression.
	
    cap->decompressRecordSize = sizeof(FFusionDecompressRecord) + 12;
    //cap->canAsync = true;
	
	// QT 7
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames))
	{
		cap->subCodecIsMultiBufferAware = true;
		cap->subCodecSupportsOutOfOrderDisplayTimes = true;
		cap->baseCodecShouldCallDecodeBandForAllFrames = !doExperimentalFlags;
		cap->subCodecSupportsScheduledBackwardsPlaybackWithDifferenceFrames = true;
		cap->subCodecSupportsDrawInDecodeOrder = doExperimentalFlags;
		cap->subCodecSupportsDecodeSmoothing = true;
	}
	
	//	FFusionDebugPrint2( "FFusionCodecInitialize(%p) done\n", glob );
    return noErr;
}

static inline int shouldDecode(FFusionGlobals glob, enum AVCodecID codecID)
{
	FFusionDecodeAbilities decode = FFUSION_PREFER_DECODE;
	if (glob->componentType == 'avc1'){
		decode = ffusionIsParsedVideoDecodable(glob->begin.parser);
	}
	if(decode > FFUSION_CANNOT_DECODE &&
	   (codecID == CODEC_ID_H264 || codecID == CODEC_ID_MPEG4)
#if TARGET_OS_MAC
	   && CFPreferencesGetAppBooleanValue(CFSTR("PreferAppleCodecs"), FFUSION_PREF_DOMAIN, NULL)
#endif
	   ){
		decode = FFUSION_PREFER_NOT_DECODE;
	}
	if(decode > FFUSION_CANNOT_DECODE){
		if(glob->isForcedDecodeEnabled){
			decode = FFUSION_PREFER_DECODE;
		}
	}
	FFusionDebugPrint( "shouldDecode(%p,%d='%s') = %d\n",
					  glob, codecID, FourCCString(glob->componentType), decode > FFUSION_PREFER_NOT_DECODE );
	return decode > FFUSION_PREFER_NOT_DECODE;
}

//-----------------------------------------------------------------
// ImageCodecPreflight
//-----------------------------------------------------------------
// The base image decompressor gets additional information about the
// capabilities of your image decompressor component by calling
// ImageCodecPreflight. The base image decompressor uses this
// information when responding to a call to the ImageCodecPredecompress
// function, which the ICM makes before decompressing an image. You
// are required only to provide values for the wantedDestinationPixelSize
// and wantedDestinationPixelTypes fields and can also modify other
// fields if necessary.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecPreflight(FFusionGlobals glob, CodecDecompressParams *p)
{
    OSType *pos;
    int index;
    CodecCapabilities *capabilities = p->capabilities;
	int count = 0;
	Handle imgDescExt;
	OSErr err = noErr;
	
    // We first open libavcodec library and the codec corresponding
    // to the fourCC if it has not been done before
	
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Preflight called.\n", glob);
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Frame dropping is %senabled for '%s'\n",
					  glob, not(glob->isFrameDroppingEnabled), FourCCString(glob->componentType) );
	
    if (!glob->avCodec)
    { OSType componentType = glob->componentType;
		enum AVCodecID codecID = getCodecID(componentType);
		
#if TARGET_OS_MAC
		SwitchCocoaToMultiThreadedMode();
#endif
		FFInitFFmpeg();
		initFFusionParsers();
		
		glob->packedType = DefaultPackedTypeForCodec(componentType);
		
		if(codecID == CODEC_ID_NONE)
		{
			asl_log(NULL, NULL, ASL_LEVEL_ERR, "ComponentType: %s", FourCCString(glob->componentType));
			asl_log(NULL, NULL, ASL_LEVEL_ERR, "Warning! Unknown codec type! Using MPEG4 by default.\n");
			codecID = AV_CODEC_ID_MPEG4;
		}
		
		if (codecID == AV_CODEC_ID_VP9) {
			glob->avCodec = avcodec_find_decoder_by_name("libvpx-vp9");
		}
		else {
			glob->avCodec = avcodec_find_decoder(codecID);
		}
		
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "Testing: %s", glob->avCodec->long_name);
		//		if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
		glob->begin.parser = ffusionParserInit(codecID);
		
		if ((codecID == CODEC_ID_MPEG4 || codecID == CODEC_ID_H264) && !glob->begin.parser)
			asl_log(NULL, NULL, ASL_LEVEL_ERR, "This is a parseable format, but we couldn't open a parser!\n");
		
        // we do the same for the AVCodecContext since all context values are
        // correctly initialized when calling the alloc function
		
        glob->avContext = avcodec_alloc_context3(glob->avCodec);
		
		// Use low delay
		glob->avContext->flags |= CODEC_FLAG_LOW_DELAY;
		
        // Image size is mandatory for DivX-like codecs
		
        glob->avContext->width = (**p->imageDescription).width;
        glob->avContext->height = (**p->imageDescription).height;
		glob->avContext->bits_per_coded_sample = (**p->imageDescription).depth;
		
        // We also pass the FourCC since it allows the H263 hybrid decoder
        // to make the difference between the various flavours of DivX
        glob->avContext->codec_tag = Endian32_Swap(glob->componentType);
		glob->avContext->codec_id  = codecID;
		
		// avc1 requires the avcC extension
		if (glob->componentType == 'avc1') {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'avcC');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'avcC', 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + FF_INPUT_BUFFER_PADDING_SIZE);
				memcpy(glob->avContext->extradata, *imgDescExt, GetHandleSize(imgDescExt));
				glob->avContext->extradata_size = GetHandleSize(imgDescExt);
				
				DisposeHandle(imgDescExt);
			} else {
				count = isImageDescriptionExtensionPresent(p->imageDescription, 'strf');
				
				// avc1 in AVI, need to reorder frames
				if (count >= 1)
					glob->packedType = PACKED_ALL_IN_FIRST_FRAME;
			}
		} else if (glob->componentType == 'mp4v') {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'esds');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'esds', 1);
				
				ReadESDSDescExt(imgDescExt, &glob->avContext->extradata, &glob->avContext->extradata_size);
				
				DisposeHandle(imgDescExt);
			}
		} else if (glob->componentType == 'mjp2') {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'jp2h');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'jp2h', 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + FF_INPUT_BUFFER_PADDING_SIZE);
				memcpy(glob->avContext->extradata, *imgDescExt, GetHandleSize(imgDescExt));
				glob->avContext->extradata_size = GetHandleSize(imgDescExt);
				
				DisposeHandle(imgDescExt);
			}
		} else if (glob->componentType == kVideoFormatTheora) {
			count = isImageDescriptionExtensionPresent(p->imageDescription, kVideoFormatTheora);
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, kVideoFormatTheora, 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + FF_INPUT_BUFFER_PADDING_SIZE);
				memcpy(glob->avContext->extradata, *imgDescExt, GetHandleSize(imgDescExt));
				glob->avContext->extradata_size = GetHandleSize(imgDescExt);
				
				DisposeHandle(imgDescExt);
			}
		} else {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'strf');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'strf', 1);
				
				if (GetHandleSize(imgDescExt) - 40 > 0) {
					glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) - 40 + FF_INPUT_BUFFER_PADDING_SIZE);
					memcpy(glob->avContext->extradata, *imgDescExt + 40, GetHandleSize(imgDescExt) - 40);
					glob->avContext->extradata_size = GetHandleSize(imgDescExt) - 40;
				}
				DisposeHandle(imgDescExt);
			}
		}
		
		//		asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p preflighted for %dx%d '%s'; %d bytes of extradata; frame dropping %senabled\n",
		//						   glob, (**p->imageDescription).width, (**p->imageDescription).height,
		//						   FourCCString(glob->componentType), glob->avContext->extradata_size,
		//						   not(glob->isFrameDroppingEnabled) );
		
		if(glob->avContext->extradata_size != 0 && glob->begin.parser != NULL){
			ffusionParseExtraData(glob->begin.parser, glob->avContext->extradata, glob->avContext->extradata_size);
		}
		
		if (glob->fileLog)
			ffusionLogDebugInfo(glob->begin.parser, glob->fileLog);
		
		if (!shouldDecode(glob, codecID))
			err = featureUnsupported;
		
		// some hooks into ffmpeg's buffer allocation to get frames in
		// decode order without delay more easily
		glob->avContext->opaque = glob;
		glob->avContext->get_buffer = FFusionGetBuffer;
		//glob->avContext->release_buffer = FFusionReleaseBuffer;
		
		// multi-slice decoding
		SetupMultithreadedDecoding(glob->avContext, codecID);
		//		if( glob->avContext->thread_count > 1 ){
		//			asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p using %d threads for '%s'\n",
		//							  glob, glob->avContext->thread_count, FourCCString(glob->componentType));
		//		}
		
		// deblock skipping for h264
		SetSkipLoopFilter(glob, glob->avContext);
		


		//glob->avContext->flags2 |= CODEC_FLAG2_FAST;


		
        // Finally we open the avcodec
		
        if (avcodec_open2(glob->avContext, glob->avCodec, NULL))
        {
            Codecprintf(glob->fileLog, "Error opening avcodec!\n");
			
			err = paramErr;
        }
		
        // codec was opened, but didn't give us its pixfmt
		// we have to decode the first frame to find out one
		else if (glob->avContext->pix_fmt == AV_PIX_FMT_NONE && p->bufferSize && p->data)
            glob->avContext->pix_fmt = FindPixFmtFromVideo(glob->avCodec, glob->avContext, p->data, p->bufferSize);
    }
	
    // Specify the minimum image band height supported by the component
    // bandInc specifies a common factor of supported image band heights -
    // if your component supports only image bands that are an even
    // multiple of some number of pixels high report this common factor in bandInc
	
    capabilities->bandMin = (**p->imageDescription).height;
    capabilities->bandInc = capabilities->bandMin;
	
    // libavcodec 0.4.x is no longer stream based i.e. you cannot pass just
    // an arbitrary amount of data to the library.
    // Instead we have to tell QT to just pass the data corresponding
    // to one frame
	
    capabilities->flags |= codecWantsSpecialScaling;
	
    p->requestedBufferWidth = (**p->imageDescription).width;
    p->requestedBufferHeight = (**p->imageDescription).height;
	
    // Indicate the pixel depth the component can use with the specified image
    // normally should be capabilities->wantedPixelSize = (**p->imageDescription).depth;
    // but we don't care since some DivX are so bugged that the depth information
    // is not correct
	
    //capabilities->wantedPixelSize = 0;
	capabilities->wantedPixelSize = (**p->imageDescription).depth;
	
    pos = *((OSType **)glob->pixelTypes);
	
    index = 0;
	
	if (!err) {
		OSType qtPixFmt = ColorConversionDstForPixFmt(glob->avCodec->id, glob->avContext->pix_fmt);
		
		/*
		 an error here means either
		 1) a color converter for this format isn't implemented
		 2) we know QT doesn't like this format and will give us argb 32bit instead
		 
		 in the case of 2 we have to special-case bail right here, since errors
		 in BeginBand are ignored
		 */
		if (qtPixFmt){
			// RJVB : yuv420p MJPEG2000 is (sometimes?) misrecognised as GRAY8 by the lavc version we use
			pos[index++] = qtPixFmt;
		}
		/*else
			err = featureUnsupported;*/
	}
	
    p->wantedDestinationPixelTypes = (OSType **)glob->pixelTypes;
	
    // Specify the number of pixels the image must be extended in width and height if
    // the component cannot accommodate the image at its given width and height
    // It is not the case here
	
    capabilities->extendWidth = 0;
    capabilities->extendHeight = 0;
	
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen;
	
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Preflight requesting colorspace '%s'. (error %d)\n", glob, FourCCString(pos[0]), err);
	
    return err;
}

static int qtTypeForFrameInfo(int original, int fftype, int skippable)
{
	if(fftype == AV_PICTURE_TYPE_I)
	{
		if(!skippable)
			return kCodecFrameTypeKey;
	}
	//	else if(skippable && IsFrameDroppingEnabled())
	else if(skippable)
		return kCodecFrameTypeDroppableDifference;
	else if(fftype != 0)
		return kCodecFrameTypeDifference;
	return original;
}

//-----------------------------------------------------------------
// ImageCodecBeginBand
//-----------------------------------------------------------------
// The ImageCodecBeginBand function allows your image decompressor
// component to save information about a band before decompressing
// it. This function is never called at interrupt time. The base
// image decompressor preserves any changes your component makes to
// any of the fields in the ImageSubCodecDecompressRecord or
// CodecDecompressParams structures. If your component supports
// asynchronous scheduled decompression, it may receive more than
// one ImageCodecBeginBand call before receiving an ImageCodecDrawBand
// call.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecBeginBand(FFusionGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	int redisplayFirstFrame = 0;
	int type = 0;
	int skippable = 0;
	
    //////
	/*  IBNibRef 		nibRef;
	 WindowRef 		window;
	 OSStatus		err;
	 CFBundleRef		bundleRef;
	 EventHandlerUPP  	handlerUPP, handlerUPP2;
	 EventTypeSpec    	eventType;
	 ControlID		controlID;
	 ControlRef		theControl;
	 KeyMap		currentKeyMap;
	 int			userPreference; */
    ///////
	
    myDrp->width = (**p->imageDescription).width;
    myDrp->height = (**p->imageDescription).height;
    myDrp->depth = (**p->imageDescription).depth;
	
    myDrp->pixelFormat = p->dstPixMap.pixelFormat;
	myDrp->decoded = p->frameTime ? (0 != (p->frameTime->flags & icmFrameAlreadyDecoded)) : false;
	myDrp->frameData = NULL;
	myDrp->buffer = NULL;
	
	//	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p BeginBand #%ld. (%sdecoded, packed %d)\n", glob, p->frameNumber, not(myDrp->decoded), glob->packedType);
	
	if (!glob->avContext) {
		Codecprintf(glob->fileLog, "FFusion: QT tried to call BeginBand without preflighting!\n");
		return internalComponentErr;
	}
	
	if (p->frameNumber == 0 && myDrp->pixelFormat != ColorConversionDstForPixFmt(glob->avCodec->id, glob->avContext->pix_fmt)) {
		Codecprintf(glob->fileLog, "QT gave us unwanted pixelFormat %s (%08x), this will not work\n", FourCCString(myDrp->pixelFormat), (unsigned)myDrp->pixelFormat);
	}
	
	if(myDrp->decoded)
	{
		int i;
		myDrp->frameNumber = p->frameNumber;
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (glob->buffers[i].retainCount && glob->buffers[i].frameNumber == myDrp->frameNumber) {
				myDrp->buffer = retainBuffer(glob, &glob->buffers[i]);
				break;
			}
		}
		return noErr;
	}
	
	if( p->frameNumber <= 1 ){
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "FFusionCodecBeginBand(%p) for %dx%d '%s' with %d bytes of extradata;\n"
						   "    frame dropping %senabled; using %d threads; QT rowBytes=%d\n",
						   glob, (**p->imageDescription).width, (**p->imageDescription).height,
						   FourCCString(glob->componentType), glob->avContext->extradata_size,
						   not(glob->isFrameDroppingEnabled), glob->avContext->thread_count,
						   drp->rowBytes );
	}
	
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && p->frameNumber != glob->begin.lastFrame + 1)
	{
		if(glob->decode.lastFrame < p->frameNumber && p->frameNumber < glob->begin.lastFrame + 1)
		{
			/* We already began this sucker, but haven't decoded it yet, find the data */
			FrameData *frameData = NULL;
			frameData = FFusionDataFind(&glob->data, p->frameNumber);
			if(frameData != NULL)
			{
				myDrp->frameData = frameData;
				drp->frameType = qtTypeForFrameInfo(drp->frameType, myDrp->frameData->type, (myDrp->frameData->skippabble && glob->isFrameDroppingEnabled) );
				myDrp->frameNumber = p->frameNumber;
				myDrp->GOPStartFrameNumber = glob->begin.lastIFrame;
				return noErr;
			}
		}
		else
		{
			/* Reset context, safe marking in such a case */
			glob->begin.lastFrameType = AV_PICTURE_TYPE_I;
			FFusionDataReadUnparsed(&(glob->data));
			glob->begin.lastPFrameData = NULL;
			redisplayFirstFrame = 1;
		}
	}
	
	if(glob->begin.parser != NULL)
	{
		int parsedBufSize = 0;
		uint8_t *buffer = (uint8_t *)drp->codecData;
		int bufferSize = p->bufferSize;
		int skipped = 0;
		
		if(glob->data.unparsedFrames.dataSize != 0)
		{
			buffer = glob->data.unparsedFrames.buffer;
			bufferSize = glob->data.unparsedFrames.dataSize;
		}
		
		if(ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable, &skipped) == 0 && (!skipped || glob->begin.futureType))
		{
			/* parse failed */
			myDrp->bufferSize = bufferSize;
			if(glob->begin.futureType != 0)
			{
				/* Assume our previously decoded P frame */
				type = glob->begin.futureType;
				glob->begin.futureType = 0;
				myDrp->frameData = glob->begin.lastPFrameData;
			}
			else
			{
				asl_log(NULL, NULL, ASL_LEVEL_ERR, "parse failed frame %ld with size %d\n", p->frameNumber, bufferSize);
				if(glob->data.unparsedFrames.dataSize != 0)
					asl_log(NULL, NULL, ASL_LEVEL_ERR, ", parser had extra data\n");
			}
		}
		else if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
		{
			if(type == AV_PICTURE_TYPE_B && glob->packedType == PACKED_ALL_IN_FIRST_FRAME && glob->begin.futureType == 0)
			/* Badly framed.  We hit a B frame after it was supposed to be displayed, switch to delaying by a frame */
				glob->packedType = PACKED_DELAY_BY_ONE_FRAME;
			else if(glob->packedType == PACKED_DELAY_BY_ONE_FRAME && parsedBufSize < bufferSize - 16)
			/* Seems to be switching back to packed in one frame; switch back*/
				glob->packedType = PACKED_ALL_IN_FIRST_FRAME;
			
			myDrp->frameData = FFusionDataAppend(&(glob->data), buffer, parsedBufSize, type);
			if(type != AV_PICTURE_TYPE_I)
				myDrp->frameData->prereqFrame = glob->begin.lastPFrameData;
			if(glob->packedType == PACKED_DELAY_BY_ONE_FRAME)
			{
				if(type != AV_PICTURE_TYPE_B)
				{
					FrameData *nextPFrame = myDrp->frameData;
					FrameData *lastPFrame = glob->begin.lastPFrameData;
					if(lastPFrame != NULL)
					/* Mark the next P or I frame, predictive decoding */
						lastPFrame->nextFrame = nextPFrame;
					myDrp->frameData = lastPFrame;
					glob->begin.lastPFrameData = nextPFrame;
					
					if(redisplayFirstFrame)
						myDrp->frameData = nextPFrame;
				}
				
				{ int displayType = glob->begin.lastFrameType;
					glob->begin.lastFrameType = type;
					type = displayType;
				}
			}
			else if(type != AV_PICTURE_TYPE_B)
				glob->begin.lastPFrameData = myDrp->frameData;
			
			if(type == AV_PICTURE_TYPE_I && glob->packedType == PACKED_ALL_IN_FIRST_FRAME)
			/* Wipe memory of past P frames */
				glob->begin.futureType = 0;
			
			if(parsedBufSize < p->bufferSize)
			{
				int oldType = type, success;
				
				buffer += parsedBufSize;
				bufferSize -= parsedBufSize;
				success = ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable, &skipped);
				if(success && type == AV_PICTURE_TYPE_B)
				{
					/* A B frame follows us, so setup the P frame for the future and set dependencies */
					glob->begin.futureType = oldType;
					if(glob->begin.lastPFrameData != NULL)
					/* Mark the next P or I frame, predictive decoding */
						glob->begin.lastPFrameData->nextFrame = myDrp->frameData;
					glob->begin.lastPFrameData = myDrp->frameData;
					myDrp->frameData = FFusionDataAppend(&(glob->data), buffer, parsedBufSize, type);
					myDrp->frameData->prereqFrame = glob->begin.lastPFrameData;
					buffer += parsedBufSize;
					bufferSize -= parsedBufSize;
				}
				if(bufferSize > 0)
					FFusionDataSetUnparsed(&(glob->data), buffer, bufferSize);
				else
					FFusionDataReadUnparsed(&(glob->data));
			}
			else
				FFusionDataReadUnparsed(&(glob->data));
			myDrp->bufferSize = 0;
		}
		else
		{
			myDrp->bufferSize = bufferSize;
		}
		
		drp->frameType = qtTypeForFrameInfo(drp->frameType, type, (skippable && glob->isFrameDroppingEnabled) );
		if(myDrp->frameData != NULL)
		{
			myDrp->frameData->frameNumber = p->frameNumber;
			myDrp->frameData->skippabble = skippable;
		}
	}
	else
		myDrp->bufferSize = p->bufferSize;
	glob->begin.lastFrame = p->frameNumber;
	if(drp->frameType == kCodecFrameTypeKey)
		glob->begin.lastIFrame = p->frameNumber;
	myDrp->frameNumber = p->frameNumber;
	myDrp->GOPStartFrameNumber = glob->begin.lastIFrame;
	
	glob->stats.type[drp->frameType].begin_calls++;
	RecomputeMaxCounts(glob);
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p BeginBand: frame #%d type %d. (%sskippable)\n", glob, myDrp->frameNumber, type, not(skippable));
	
    return noErr;
}

static OSErr PrereqDecompress(FFusionGlobals glob, FrameData *prereq, AVCodecContext *context, int width, int height, AVFrame *picture)
{ FrameData *preprereq = FrameDataCheckPrereq(prereq);
	unsigned char *dataPtr = (unsigned char *)prereq->buffer;
	int dataSize = prereq->dataSize;
	OSErr err;
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p prereq-decompressing frame #%ld.\n", glob, prereq->frameNumber);
	
	if(preprereq)
		PrereqDecompress(glob, preprereq, context, width, height, picture);
	
	err = FFusionDecompress(glob, context, dataPtr, width, height, picture, dataSize);
	
	return err;
}

pascal ComponentResult FFusionCodecDecodeBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	OSErr err = noErr;
	AVFrame tempFrame;
	FrameData *frameData = NULL;
	unsigned char *dataPtr = NULL;
	unsigned int dataSize;
	
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	
	glob->stats.type[drp->frameType].decode_calls++;
	RecomputeMaxCounts(glob);
	// 	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p DecodeBand #%d qtType %d. (packed %d)\n", glob, myDrp->frameNumber, drp->frameType, glob->packedType);
	
	// QuickTime will drop H.264 frames when necessary if a sample dependency table exists
	// we don't want to flush buffers in that case.
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && myDrp->frameNumber != glob->decode.lastFrame + 1)
	{
		/* Skipped some frames in here */
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p - frames skipped.\n", glob);
		if(drp->frameType == kCodecFrameTypeKey || myDrp->GOPStartFrameNumber > glob->decode.lastFrame || myDrp->frameNumber < glob->decode.lastFrame)
		{
			/* If this is a key frame or the P frame before us is after the last frame (skip ahead), or we are before the last decoded frame (skip back) *
			 * then we are in a whole new GOP */
			avcodec_flush_buffers(glob->avContext);
		}
	}
	
	if(myDrp->frameData && myDrp->frameData->decoded && glob->decode.futureBuffer != NULL)
	{
		myDrp->buffer = retainBuffer(glob, glob->decode.futureBuffer);
		myDrp->decoded = true;
		setFutureFrame(glob, NULL);
		FFusionDataMarkRead(myDrp->frameData);
		glob->decode.lastFrame = myDrp->frameNumber;
		return err;
	}
	
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && myDrp->frameData != NULL && myDrp->frameData->decoded == 0)
	{
		/* Pull from our buffer */
		FrameData *prereq = FrameDataCheckPrereq( (frameData = myDrp->frameData) );
		
		if(prereq)
		{
			PrereqDecompress(glob, prereq, glob->avContext, myDrp->width, myDrp->height, &tempFrame);
			if(tempFrame.data[0] != NULL)
			{
				setFutureFrame(glob, (FFusionBuffer *)tempFrame.opaque);
				prereq->decoded = TRUE;
			}
		}
		dataPtr = (unsigned char *)frameData->buffer;
		dataSize = frameData->dataSize;
		frameData->decoded = TRUE;
	}
	else
	{
		/* data is already set up properly for us */
		dataSize = myDrp->bufferSize;
		dataPtr = FFusionCreateEntireDataBuffer(&(glob->data), (uint8_t *)drp->codecData, dataSize);
	}
	
	// RJVB: decoding principle gleaned from Apple's ElectricImageComponent: the destination is a simple
	// contiguous pixmap that of size drp->height * drp->rowBytes and that should receive each line's
	// pixels without any inter-pixel (or inter-line) padding.
	//	destinationPtr=(UInt8 *)drp->baseAddr;
	//	switch( myDrp->depth ){
	//		case 1:
	//			movieLineByteWidth = ((myDrp->width + 7) / 8);
	//			endOfScanLine = destinationPtr + ((myDrp->width + 7) / 8);
	//			pixelWidth = sizeof(UInt8);
	//			break;
	//		case 8:
	//			movieLineByteWidth = (myDrp->width * 1);
	//			endOfScanLine = destinationPtr + (myDrp->width * 1);
	//			pixelWidth = sizeof(UInt8);
	//			break;
	//		case 16:
	//			movieLineByteWidth = (myDrp->width * 2);
	//			endOfScanLine = destinationPtr + (myDrp->width * 2);
	//			pixelWidth = sizeof(UInt16);
	//			break;
	//		case 24:
	//		case 32:
	//			movieLineByteWidth = (myDrp->width * 4):
	//			endOfScanLine = destinationPtr + (myDrp->width * 4);
	//			pixelWidth = sizeof(UInt32);
	//			break;
	//	}
	//	rowJump = drp->rowBytes - movieLineByteWidth;
	//	height = myDrp->height;
	//	while( decoding ){
	//		switch( myDrp->depth ){
	//			case 1:
	//			case 8:
	//				*(UInt8 *)destinationPtr = decodedPixel;
	//				destinationPtr += sizeof(UInt8);
	//				break;
	//			case 16:
	//				*(UInt16 *)destinationPtr = decodedPixel;
	//				destinationPtr += sizeof(UInt8);
	//				break;
	//				break;
	//			case 24:
	//			case 32:
	//				*(UInt32 *)destinationPtr = decodedPixel;
	//				destinationPtr += sizeof(UInt8);
	//				break;
	//				break;
	//		}
	//		if( baseAddr == endOfScanLine ){
	//			destinationPtr += rowJump;
	//			endOfScanLine += rowJump;
	//			height--;
	//		}
	//	}
	err = FFusionDecompress(glob, glob->avContext, dataPtr, myDrp->width, myDrp->height, &tempFrame, dataSize);
	
	if (glob->packedType == PACKED_QUICKTIME_KNOWS_ORDER) {
		myDrp->buffer = &glob->buffers[glob->lastAllocatedBuffer];
		myDrp->buffer->frameNumber = myDrp->frameNumber;
		retainBuffer(glob, myDrp->buffer);
		myDrp->decoded = true;
		glob->decode.lastFrame = myDrp->frameNumber;
		return err;
	}
	if(tempFrame.data[0] == NULL)
		myDrp->buffer = NULL;
	else
		myDrp->buffer = retainBuffer(glob, (FFusionBuffer *)tempFrame.opaque);
	
	if(tempFrame.pict_type == AV_PICTURE_TYPE_I)
	/* Wipe memory of past P frames */
		setFutureFrame(glob, NULL);
	glob->decode.lastFrame = myDrp->frameNumber;
	myDrp->decoded = true;
	
	FFusionDataMarkRead(frameData);
	
	return err;
}

//-----------------------------------------------------------------
// ImageCodecDrawBand
//-----------------------------------------------------------------
// The base image decompressor calls your image decompressor
// component's ImageCodecDrawBand function to decompress a band or
// frame. Your component must implement this function. If the
// ImageSubCodecDecompressRecord structure specifies a progress function
// or data-loading function, the base image decompressor will never call
// ImageCodecDrawBand at interrupt time. If the ImageSubCodecDecompressRecord
// structure specifies a progress function, the base image decompressor
// handles codecProgressOpen and codecProgressClose calls, and your image
// decompressor component must not implement these functions.
// If not, the base image decompressor may call the ImageCodecDrawBand
// function at interrupt time. When the base image decompressor calls your
// ImageCodecDrawBand function, your component must perform the decompression
// specified by the fields of the ImageSubCodecDecompressRecord structure.
// The structure includes any changes your component made to it when
// performing the ImageCodecBeginBand function. If your component supports
// asynchronous scheduled decompression, it may receive more than one
// ImageCodecBeginBand call before receiving an ImageCodecDrawBand call.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecDrawBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp)
{
    OSErr err = noErr;
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	AVPicture *picture = NULL;
	
	glob->stats.type[drp->frameType].draw_calls++;
	RecomputeMaxCounts(glob);
	// 	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p DrawBand #%d. (%sdecoded)\n", glob, myDrp->frameNumber, not(myDrp->decoded));
	
	if(!myDrp->decoded) {
		err = FFusionCodecDecodeBand(glob, drp, 0);
		
		if (err) goto err;
	}
	
	if (myDrp->buffer && myDrp->buffer->picture.data[0]) {
		picture = &myDrp->buffer->picture;
		glob->lastDisplayedPicture = picture;
	} else if (glob->lastDisplayedPicture && glob->lastDisplayedPicture->data[0]) {
		picture = glob->lastDisplayedPicture;
	} else {
		//Display black (no frame decoded yet)
		
		if (!glob->colorConv.clear) {
			err = ColorConversionFindFor(&glob->colorConv, glob->avCodec->id, glob->avContext->pix_fmt,
										 NULL, myDrp->pixelFormat, drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height );
			if (err) goto err;
		}
		
		glob->colorConv.clear((UInt8*)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height);
		return noErr;
	}
	
	if (!glob->colorConv.convert) {
		err = ColorConversionFindFor( &glob->colorConv, glob->avCodec->id, glob->avContext->pix_fmt,
									 picture, myDrp->pixelFormat, drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height );
		if (err) goto err;
	}
	
	glob->colorConv.convert(picture, (UInt8*)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height);
	
err:
    return err;
}

//-----------------------------------------------------------------
// ImageCodecEndBand
//-----------------------------------------------------------------
// The ImageCodecEndBand function notifies your image decompressor
// component that decompression of a band has finished or
// that it was terminated by the Image Compression Manager. Your
// image decompressor component is not required to implement the
// ImageCodecEndBand function. The base image decompressor may call
// the ImageCodecEndBand function at interrupt time.
// After your image decompressor component handles an ImageCodecEndBand
// call, it can perform any tasks that are required when decompression
// is finished, such as disposing of data structures that are no longer
// needed. Because this function can be called at interrupt time, your
// component cannot use this function to dispose of data structures;
// this must occur after handling the function. The value of the result
// parameter should be set to noErr if the band or frame was
// drawn successfully.
// If it is any other value, the band or frame was not drawn.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecEndBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
	FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	FFusionBuffer *buf = myDrp->buffer;
	
	glob->stats.type[drp->frameType].end_calls++;
	if(buf && buf->frame)
		releaseBuffer(glob->avContext, buf->frame);
	
	//	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p EndBand #%d.\n", glob, myDrp->frameNumber);
#if TARGET_OS_WIN32
	// are we being used?
	//	Sleep( 83 );
#endif
    return noErr;
}

// Gamma curve value for FFusion video.
ComponentResult FFusionCodecGetSourceDataGammaLevel(FFusionGlobals glob, Fixed *sourceDataGammaLevel)
{
	enum AVColorTransferCharacteristic color_trc = AVCOL_TRC_UNSPECIFIED;
	float gamma;
	
	if (glob->avContext)
		color_trc = glob->avContext->color_trc;
	
	switch (color_trc) {
		case AVCOL_TRC_GAMMA28:
			gamma = 2.8;
			break;
		case AVCOL_TRC_GAMMA22:
			gamma = 2.2;
			break;
		default: // absolutely everything in the world will reach here
			gamma = 1/.45; // and GAMMA22 is probably a typo for this anyway
	}
	
	*sourceDataGammaLevel = FloatToFixed(gamma);
	return noErr;
}

//-----------------------------------------------------------------
// ImageCodecGetCodecInfo
//-----------------------------------------------------------------
// Your component receives the ImageCodecGetCodecInfo request whenever
// an application calls the Image Compression Manager's GetCodecInfo
// function.
// Your component should return a formatted compressor information
// structure defining its capabilities.
// Both compressors and decompressors may receive this request.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecGetCodecInfo(FFusionGlobals glob, CodecInfo *info)
{
	//	FFusionDebugPrint2( "FFusionCodecGetCodecInfo(%p)\n", glob );
	return getFFusionCodecInfo(glob->self, glob->componentType, info);
}

static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic)
{
	FFusionGlobals glob = s->opaque;
	int ret = avcodec_default_get_buffer2(s, pic, NULL);
	int i;
	
	if (ret >= 0) {
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (!glob->buffers[i].retainCount) {
				//				asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Starting Buffer %p.\n", glob, &glob->buffers[i]);
				pic->opaque = &glob->buffers[i];
				glob->buffers[i].frame = pic;
				memcpy(&glob->buffers[i].picture, pic, sizeof(AVPicture));
				glob->buffers[i].retainCount = 0;
				glob->lastAllocatedBuffer = i;
				break;
			}
		}
	}
	
	return ret;
}

static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf)
{
	buf->retainCount++;

	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Retained Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	return buf;
}

static void releaseBuffer(AVCodecContext *s, AVFrame *pic)
{
	FFusionBuffer *buf = pic->opaque;
	
	buf->retainCount--;
	
	FFusionGlobals glob = (FFusionGlobals)s->opaque;
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Released Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	if(!buf->retainCount)
	{
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Buffer gone %p #%d", glob, buf, buf->frameNumber);
		buf->picture.data[0] = NULL;
	}
}

//-----------------------------------------------------------------
// FFusionDecompress
//-----------------------------------------------------------------
// This function calls libavcodec to decompress one frame.
//-----------------------------------------------------------------
OSErr FFusionDecompress(FFusionGlobals glob, AVCodecContext *context, UInt8 *dataPtr, int width, int height, AVFrame *picture, int length)
{
	OSErr err = noErr;
	int got_picture = false;
	int len = 0;
	AVPacket pkt;
	
	asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Decompress %d bytes.\n", glob, length);
	avcodec_get_frame_defaults(picture);
	av_frame_unref(picture);
	
	av_init_packet(&pkt);
	pkt.data = dataPtr;
	pkt.size = length;
	len = avcodec_decode_video2(context, picture, &got_picture, &pkt);
	if( !got_picture ){
		// RJVB 20131016
		memset( picture, 0, sizeof(*picture) );
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "Found no picture, len=%d", len );
	}
	
	if (len < 0)
	{
		Codecprintf(glob->fileLog, "Error while decoding frame\n");
	}
	// 	else{
	// 		FFusionDebugPrint2( "Found picture number %d, len=%d", picture->display_picture_number, len );
	// 	}
	
	return err;
}
