#include <asl.h>

//---------------------------------------------------------------------------
//FFusion
//Modern Video Codec for macOS
//H.265/HEVC and VP9 Support
//Based on FFmpeg
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
#include <QuickTime/QuickTime.h>
#include <sys/sysctl.h>
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
	AVPicture		*lastDisplayedPicture;
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
static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic, int flags);
static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic);
static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf);
static void releaseBuffer(AVCodecContext *s, FFusionBuffer *buf);

int GetPPUserPreference();
void SetPPUserPreference(int value);
pascal OSStatus HandlePPDialogWindowEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
pascal OSStatus HandlePPDialogControlEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
void ChangeHintText(int value, ControlRef staticTextField);

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

static enum AVPixelFormat FindPixFmtFromVideo(AVCodec *codec, AVCodecContext *avctx, Ptr data, int bufferSize)
{
	AVCodecContext *tmpContext;
    AVFrame *tmpFrame;
	int got_picture = 0;
    enum AVPixelFormat pix_fmt;
	
	tmpContext = avcodec_alloc_context3(codec);
	tmpFrame   = av_frame_alloc();
	tmpContext->width = avctx->width;
	tmpContext->height = avctx->height;
	tmpContext->pix_fmt = avctx->pix_fmt;
	tmpContext->codec_id = avctx->codec_id;
	tmpContext->extradata = avctx->extradata;
	tmpContext->extradata_size = avctx->extradata_size;

    if (avcodec_open2(tmpContext, codec, NULL)) {
		pix_fmt = AV_PIX_FMT_NONE;
		goto bail;
	}
	AVPacket *pkt = av_packet_alloc();
	pkt->data = (UInt8*)data;
	pkt->size = bufferSize;
	int len = avcodec_send_packet(tmpContext, pkt);
	if (len == 0) {
		len = avcodec_receive_frame(tmpContext, tmpFrame);
		got_picture = (len == 0);
	}
	av_packet_free(&pkt);
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "Decoded frame to find pix_fmt, got %d", len);
	
	if (len == -1094995529) {
		//Wowfunhappy: AVERROR_INVALIDDATA. Let's just guess the pixel format.
		//(This video probably won't work anyway, but worth a try?)
		return 0;
	}
	
    pix_fmt = tmpContext->pix_fmt;
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "Found pix_fmt is: %d", pix_fmt);
    avcodec_close(tmpContext);
bail:
	av_frame_free(&tmpFrame);
    return pix_fmt;
}

static void SetupMultithreadedDecoding(AVCodecContext *s, enum AVCodecID codecID)
{
	int nthreads = 1;
	size_t len = 32;

	if (sysctlbyname("hw.activecpu", &nthreads, &len, NULL, 0) == -1) nthreads = 1;

	s->thread_count = nthreads;
	s->thread_type  = FF_THREAD_SLICE;
}


void setFutureFrame(FFusionGlobals glob, FFusionBuffer *newFuture)
{
	FFusionBuffer *temp = glob->decode.futureBuffer;
	if(newFuture != NULL)
		retainBuffer(glob, newFuture);
	glob->decode.futureBuffer = newFuture;
	if(temp != NULL)
	{
		releaseBuffer(glob->avContext, temp);
	}
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
        SetComponentInstanceStorage(self, (Handle)glob);
		
        glob->self = self;
        glob->target = self;
        glob->drawBandUPP = NULL;
        glob->pixelTypes = NewHandleClear((kNumPixelFormatsSupportedFFusion+1) * sizeof(OSType));
        glob->avCodec = 0;
        glob->componentType = descout.componentSubType;
		glob->data.frames = NULL;
		glob->begin.parser = NULL;
		
        // Open and target an instance of the base decompressor as we delegate
        // most of our calls to the base decompressor instance
		
        err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
        if (!err)
        {
            if( ComponentSetTarget(glob->delegateComponent, self) == badComponentSelector ){
				//asl_log(NULL, NULL, ASL_LEVEL_ERR, "Error %d targeting the base image decompressor with ourselves!\n", err );
			}
        }
        else
        {
            //asl_log(NULL, NULL, ASL_LEVEL_ERR, "Error %d opening the base image decompressor!\n", err );
        }
		
		// we allocate some space for copying the frame data since we need some padding at the end
		// for ffmpeg's optimized bitstream readers. Size doesn't really matter, it'll grow if need be
		FFusionDataSetup(&(glob->data), 256, 1024*1024);
		glob->isFrameDroppingEnabled = IsFrameDroppingEnabled();
		glob->isForcedDecodeEnabled = IsForcedDecodeEnabled();
    }
	
    //asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p opened for '%s'\n", glob, FourCCString(glob->componentType));
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
        {
			ComponentDescription cDescr;
			Handle cName = NewHandleClear(200);
			if (glob->avContext->extradata)
				free(glob->avContext->extradata);
			
			if (glob->avContext->codec){
				avcodec_close(glob->avContext);
			}
            av_free(glob->avContext);
			if( GetComponentInfo( (Component) self, &cDescr, cName, NULL, NULL ) == noErr ){
				char tStr[5];
				strcpy( tStr, FourCCString(cDescr.componentSubType) );
				//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p closed - '%s' type '%s' by '%s'\n", glob, &(*cName)[1], tStr, FourCCString(cDescr.componentManufacturer) );
			}
			else {
				//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p closed.\n", glob);
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
    cap->canAsync = true;
	
	// QT 7
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames))
	{
		cap->subCodecIsMultiBufferAware = true;
		cap->subCodecSupportsOutOfOrderDisplayTimes = true;
		cap->baseCodecShouldCallDecodeBandForAllFrames = false;
		cap->subCodecSupportsScheduledBackwardsPlaybackWithDifferenceFrames = true;
		cap->subCodecSupportsDrawInDecodeOrder = true;
		cap->subCodecSupportsDecodeSmoothing = true;
	}
	
	//	FFusionDebugPrint2( "FFusionCodecInitialize(%p) done\n", glob );
    return noErr;
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
	
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Preflight called.\n", glob);
	/*//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Frame dropping is %senabled for '%s'\n",
					  glob, not(glob->isFrameDroppingEnabled), FourCCString(glob->componentType) );*/
	
    if (!glob->avCodec)
    { OSType componentType = glob->componentType;
		enum AVCodecID codecID = getCodecID(componentType);
		
		FFInitFFmpeg();
		initFFusionParsers();
		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "ComponentType: %s", FourCCString(glob->componentType));
		
		if(codecID == AV_CODEC_ID_NONE)
		{
			err = featureUnsupported;
		}
		else if (codecID == AV_CODEC_ID_VP9) {
			//Wowfunhappy: For some reason, FFMpeg's default VP9 decoder displays some videos (e.g. Youtube downloads) as colored vertical bands.
			glob->avCodec = avcodec_find_decoder_by_name("libvpx-vp9");
		}
		else {
			glob->avCodec = avcodec_find_decoder(codecID);
		}
		
		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "Codec Long Name: %s", glob->avCodec->long_name);
		
		
		
        // we do the same for the AVCodecContext since all context values are
        // correctly initialized when calling the alloc function
		
        glob->avContext = avcodec_alloc_context3(glob->avCodec);
		
		// Use low delay
		glob->avContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
		
        // Image size is mandatory for video codecs
		
        glob->avContext->width = (**p->imageDescription).width;
        glob->avContext->height = (**p->imageDescription).height;
		glob->avContext->bits_per_coded_sample = (**p->imageDescription).depth;
		
		glob->avContext->codec_id  = codecID;
		
		if (glob->avContext->codec_id == AV_CODEC_ID_HEVC) {
			
			/* Wowfunhappy:
			 * This magical code fixes HEVC. I don't know why.
			 * I just copied and pasted the avc1, and I changed the 'a' to an 'h'.
			 * I guess this is some kind of header offset?
			 */
			
			//asl_log(NULL, NULL, ASL_LEVEL_ERR, "Does hevc video have mysterious hvcC image Description? %d", isImageDescriptionExtensionPresent(p->imageDescription, 'hvcC'));
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'hvcC');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'hvcC', 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + AV_INPUT_BUFFER_PADDING_SIZE);
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
					glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) - 40 + AV_INPUT_BUFFER_PADDING_SIZE);
					memcpy(glob->avContext->extradata, *imgDescExt + 40, GetHandleSize(imgDescExt) - 40);
					glob->avContext->extradata_size = GetHandleSize(imgDescExt) - 40;
				}
				DisposeHandle(imgDescExt);
			}
		}
		
		//		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p preflighted for %dx%d '%s'; %d bytes of extradata; frame dropping %senabled\n",
		//						   glob, (**p->imageDescription).width, (**p->imageDescription).height,
		//						   FourCCString(glob->componentType), glob->avContext->extradata_size,
		//						   not(glob->isFrameDroppingEnabled) );
		
		
		// some hooks into ffmpeg's buffer allocation to get frames in
		// decode order without delay more easily
		glob->avContext->opaque = glob;
		//glob->avContext->get_buffer = FFusionGetBuffer;
		glob->avContext->get_buffer2 = FFusionGetBuffer;
		//glob->avContext->release_buffer = FFusionReleaseBuffer;
		
		// multi-slice decoding
		SetupMultithreadedDecoding(glob->avContext, codecID);
		
		//glob->avContext->flags2 |= CODEC_FLAG2_FAST;

        // Finally we open the avcodec
		
        if (avcodec_open2(glob->avContext, glob->avCodec, NULL))
        {
            //asl_log(NULL, NULL, ASL_LEVEL_ERR, "Error opening avcodec!\n");
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
	
    // Set the pixel depth from the image description
    capabilities->wantedPixelSize = (**p->imageDescription).depth;
	
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "capabilities->wantedPixelSize: %d", capabilities->wantedPixelSize);
	
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
		else
			err = featureUnsupported;
	}
	
    p->wantedDestinationPixelTypes = (OSType **)glob->pixelTypes;
	
    // Specify the number of pixels the image must be extended in width and height if
    // the component cannot accommodate the image at its given width and height
    // It is not the case here
	
    capabilities->extendWidth = 0;
    capabilities->extendHeight = 0;
	
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen;
	
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Preflight requesting colorspace '%s'. (error %d)\n", glob, FourCCString(pos[0]), err);
	
    return err;
}

static int qtTypeForFrameInfo(int original, int fftype, int skippable)
{
	if(fftype == AV_PICTURE_TYPE_I)
	{
		if(!skippable)
			return kCodecFrameTypeKey;
	}
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
	
	//	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p BeginBand #%ld. (%sdecoded)\n", glob, p->frameNumber, not(myDrp->decoded));
	
	if (!glob->avContext) {
		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "FFusion: QT tried to call BeginBand without preflighting!\n");
		return internalComponentErr;
	}
	
	if (p->frameNumber == 0 && myDrp->pixelFormat != ColorConversionDstForPixFmt(glob->avCodec->id, glob->avContext->pix_fmt)) {
		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "QT gave us unwanted pixelFormat %s (%08x), this will not work\n", FourCCString(myDrp->pixelFormat), (unsigned)myDrp->pixelFormat);
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
		/*//asl_log(NULL, NULL, ASL_LEVEL_ERR, "FFusionCodecBeginBand(%p) for %dx%d '%s' with %d bytes of extradata;\n"
						   "    frame dropping %senabled; using %d threads; QT rowBytes=%d\n",
						   glob, (**p->imageDescription).width, (**p->imageDescription).height,
						   FourCCString(glob->componentType), glob->avContext->extradata_size,
						   not(glob->isFrameDroppingEnabled), glob->avContext->thread_count,
						   drp->rowBytes );*/
	}
	
	
	myDrp->bufferSize = p->bufferSize;
	glob->begin.lastFrame = p->frameNumber;
	if(drp->frameType == kCodecFrameTypeKey)
		glob->begin.lastIFrame = p->frameNumber;
	myDrp->frameNumber = p->frameNumber;
	myDrp->GOPStartFrameNumber = glob->begin.lastIFrame;
	
	glob->stats.type[drp->frameType].begin_calls++;
	RecomputeMaxCounts(glob);
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p BeginBand: frame #%d type %d. (%sskippable)\n", glob, myDrp->frameNumber, type, not(skippable));
	
    return noErr;
}

static OSErr PrereqDecompress(FFusionGlobals glob, FrameData *prereq, AVCodecContext *context, int width, int height, AVFrame *picture)
{ FrameData *preprereq = FrameDataCheckPrereq(prereq);
	unsigned char *dataPtr = (unsigned char *)prereq->buffer;
	int dataSize = prereq->dataSize;
	OSErr err;
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p prereq-decompressing frame #%ld.\n", glob, prereq->frameNumber);
	
	if(preprereq)
		PrereqDecompress(glob, preprereq, context, width, height, picture);
	
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "About to call FFusionDecompress from PrereqDecompress");
	err = FFusionDecompress(glob, context, dataPtr, width, height, picture, dataSize);
	
	return err;
}

pascal ComponentResult FFusionCodecDecodeBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	OSErr err = noErr;
	AVFrame *tempFrame = av_frame_alloc();
	FrameData *frameData = NULL;
	unsigned char *dataPtr = NULL;
	unsigned int dataSize;
	
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	
	glob->stats.type[drp->frameType].decode_calls++;
	RecomputeMaxCounts(glob);
	// 	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p DecodeBand #%d qtType %d.\n", glob, myDrp->frameNumber, drp->frameType);
	
	// Check if we need to flush buffers on frame discontinuity
	if(myDrp->frameNumber != glob->decode.lastFrame + 1)
	{
		if(drp->frameType == kCodecFrameTypeKey || myDrp->GOPStartFrameNumber > glob->decode.lastFrame || myDrp->frameNumber < glob->decode.lastFrame)
		{
			avcodec_flush_buffers(glob->avContext);
		}
	}
	
	// Simple case: data is already set up properly for us
	dataSize = myDrp->bufferSize;
	dataPtr = FFusionCreateEntireDataBuffer(&(glob->data), (uint8_t *)drp->codecData, dataSize);
	
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "About to call FFusionDecompress from FFusionCodecDecodeBand");
	err = FFusionDecompress(glob, glob->avContext, dataPtr, myDrp->width, myDrp->height, tempFrame, dataSize);
	
	// Modern codecs: straightforward buffer handling
	myDrp->buffer = &glob->buffers[glob->lastAllocatedBuffer];
	myDrp->buffer->frameNumber = myDrp->frameNumber;
	retainBuffer(glob, myDrp->buffer);
	myDrp->decoded = true;
	glob->decode.lastFrame = myDrp->frameNumber;
	av_frame_free(&tempFrame);
	
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
	// 	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p DrawBand #%d. (%sdecoded)\n", glob, myDrp->frameNumber, not(myDrp->decoded));
	
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
	if(buf && buf->frame) {
		releaseBuffer(glob->avContext, buf);
	}
	
	//	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p EndBand #%d.\n", glob, myDrp->frameNumber);
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

static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic, int flags)
{
	FFusionGlobals glob = s->opaque;
	int ret = avcodec_default_get_buffer2(s, pic, NULL);
	int i;
	
	if (ret >= 0) {
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (!glob->buffers[i].retainCount) {
				//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Starting Buffer %p.\n", glob, &glob->buffers[i]);
				pic->opaque = &glob->buffers[i];
				glob->buffers[i].frame = pic;
				// Copy data and linesize from AVFrame to AVPicture (compatible fields)
				memcpy(glob->buffers[i].picture.data, pic->data, sizeof(glob->buffers[i].picture.data));
				memcpy(glob->buffers[i].picture.linesize, pic->linesize, sizeof(glob->buffers[i].picture.linesize));
				glob->buffers[i].retainCount = 0;
				glob->lastAllocatedBuffer = i;
				break;
			} else {
				
			}
		}
	}
	
	return ret;
}

static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf)
{
	buf->retainCount++;

	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Retained Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	return buf;
}

static void releaseBuffer(AVCodecContext *s, FFusionBuffer *buf)
{
	FFusionGlobals glob = (FFusionGlobals)s->opaque;
	buf->retainCount--;
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Released Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	if(!buf->retainCount)
	{
		//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Buffer gone %p #%d", glob, buf, buf->frameNumber);
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
	AVPacket *pkt = av_packet_alloc();
	//asl_log(NULL, NULL, ASL_LEVEL_ERR, "%p Decompress %d bytes.\n", glob, length);
	
	pkt->data = dataPtr;
	pkt->size = length;
	
	// Modern codecs use the new API
	avcodec_send_packet(context, pkt);
	avcodec_receive_frame(context, picture);
	
	av_packet_free(&pkt);
	
	return err;
}
