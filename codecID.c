#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#ifdef _MSC_VER
	// prevent the GNU compatibility stdint.h header included with the QuickTime SDK from being included:
#	define _STDINT_H
#endif

#include "libavcodec/avcodec.h"
#ifdef __MACH__
#	include <QuickTime/QuickTime.h>
#else
#	include <ConditionalMacros.h>
#	include <Endian.h>
#	include <ImageCodec.h>
#endif
#include "CodecIDs.h"
#include "FFusionResourceIDs.h"

int getCodecID(OSType componentType)
{
	enum AVCodecID codecID = CODEC_ID_NONE;
	switch(componentType)
	{

		case 'MPG4':
  		case 'mpg4':
  		case 'DIV1':
  		case 'div1':
			codecID = CODEC_ID_MSMPEG4V1;
			break;

		case 'MP42':
  		case 'mp42':
  		case 'DIV2':
  		case 'div2':
			codecID = CODEC_ID_MSMPEG4V2;
			break;

		case 'MPG3':
  		case 'mpg3':
  		case 'MP43':
  		case 'mp43':
  		case 'DIV3':
  		case 'div3':
  		case 'DIV4':
  		case 'div4':
  		case 'DIV5':
  		case 'div5':
  		case 'DIV6':
  		case 'div6':
  		case 'AP41':
  		case 'COL0':
  		case 'col0':
  		case 'COL1':
  		case 'col1':
			codecID = CODEC_ID_MSMPEG4V3;
			break;

		case 'DIVX':
  		case 'divx':
  		case 'mp4s':
  		case 'MP4S':
  		case 'M4S2':
  		case 'm4s2':
  		case 0x04000000:
  		case 'UMP4':

		case 'DX50':

		case '3IVD':
  		case '3ivd':
  		case '3IV2':
  		case '3iv2':

		case 'XVID':
  		case 'xvid':
  		case 'XviD':
  		case 'XVIX':
  		case 'BLZ0':

		case 'RMP4':
  		case 'SEDG':
  		case 'WV1F':
  		case 'FMP4':
  		case 'SMP4':

		case 'mp4v':
			codecID = CODEC_ID_MPEG4;
			break;

		case 'H264':
  		case 'h264':
  		case 'X264':
  		case 'x264':
  		case 'DAVC':
  		case 'VSSH':
  		case 'AVC1':
  		case 'avc1':
			codecID = CODEC_ID_H264;
			break;
			
		case 'H265':
  		case 'h265':
  		case 'X265':
  		case 'x265':
  		case 'HEV1':
  		case 'hev1':
  		case 'HEVC':
  		case 'hevc':
		case 'hvc1':
			codecID = AV_CODEC_ID_HEVC;
			break;

		case 'FLV1':
			codecID = CODEC_ID_FLV1;
			break;

		case 'FSV1':
			codecID = CODEC_ID_FLASHSV;
			break;

		case 'VP60':
  		case 'VP61':
  		case 'VP62':
			codecID = CODEC_ID_VP6;
			break;

		case 'VP6F':
  		case 'FLV4':
			codecID = CODEC_ID_VP6F;
			break;

		case 'I263':
  		case 'i263':
			codecID = CODEC_ID_H263I;
			break;

		case 'VP30':
  		case 'VP31':
			codecID = CODEC_ID_VP3;
			break;

		case 'HFYU':
			codecID = CODEC_ID_HUFFYUV;
			break;

		case 'FFVH':
			codecID = CODEC_ID_FFVHUFF;
			break;

		case 'MPEG':
  		case 'mpg1':
  		case 'mp1v':
			codecID = CODEC_ID_MPEG1VIDEO;
			break;

		case 'MPG2':
  		case 'mpg2':
  		case 'mp2v':
  		case 'm2v1':
			codecID = CODEC_ID_MPEG2VIDEO;
			break;

		case 'FPS1':
			codecID = CODEC_ID_FRAPS;
			break;

		case 'SNOW':
			codecID = CODEC_ID_SNOW;
			break;

		case 'RJPG':
  		case 'NUV1':
			codecID = CODEC_ID_NUV;
			break;

		case 'RT21':
			codecID = CODEC_ID_INDEO2;
			break;

		case 'IV32':
  		case 'iv32':
  		case 'IV31':
  		case 'iv31':
			codecID = CODEC_ID_INDEO3;
			break;

		case 'IV50':
  		case 'iv50':
			codecID = CODEC_ID_INDEO5;
			break;

		case 'tscc':
			codecID = CODEC_ID_TSCC;
			break;

		case 'ZMBV':
			codecID = CODEC_ID_ZMBV;
			break;

		case 'VP6A':
			codecID = CODEC_ID_VP6A;
			break;

		case 'VP80':
			codecID = CODEC_ID_VP8;
			break;
			
		case 'VP90':
			codecID = AV_CODEC_ID_VP9;
			break;

		case 'FFV1':
			codecID = CODEC_ID_FFV1;
			break;

		case 'PeTh':
			codecID = CODEC_ID_THEORA;
			break;

		// RJVB
		case 'mjp2':
		case '2pjm':
			codecID = CODEC_ID_JPEG2000;
			break;

		case kSubFormatVobSub:
			codecID = CODEC_ID_DVD_SUBTITLE;
			break;

		case kCompressedAVC1:

		case kCompressedMP4V:
			codecID = CODEC_ID_NONE;
			break;

		case kAudioFormatWMA1MS:
			codecID = CODEC_ID_WMAV1;
			break;

		case kAudioFormatWMA2MS:
			codecID = CODEC_ID_WMAV2;
			break;

		case kAudioFormatFlashADPCM:
			codecID = CODEC_ID_ADPCM_SWF;
			break;

		case kAudioFormatXiphVorbis:
			codecID = CODEC_ID_VORBIS;
			break;

		case '.mp1':
			codecID = CODEC_ID_MP1;
			break;

		case 0x6d730050:

		case '.mp2':
			codecID = CODEC_ID_MP2;
			break;

		case kAudioFormatTTA:
			codecID = CODEC_ID_TTA;
			break;

		case kAudioFormatDTS:
			codecID = CODEC_ID_DTS;
			break;

		case kAudioFormatNellymoser:
			codecID = CODEC_ID_NELLYMOSER;
			break;

		case kCompressedAC3:

		case kCompressedMP1:

		case kCompressedMP2:

		case kCompressedMP3:

		case kCompressedDTS:
			codecID = CODEC_ID_NONE;
			break;
		default:
			break;
	}
	return codecID;
}


pascal ComponentResult getFFusionCodecInfo(ComponentInstance self, OSType componentType, void *info)
{
    OSErr err = noErr;

    if (info == NULL)
    {
        err = paramErr;
    }
    else
    {
        CodecInfo **tempCodecInfo;

        switch (componentType)
        {

			case 'MPG4':
			case 'mpg4':
			case 'DIV1':
			case 'div1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'MP42':
			case 'mp42':
			case 'DIV2':
			case 'div2':

				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX2CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'MPG3':
			case 'mpg3':
			case 'MP43':
			case 'mp43':
			case 'DIV3':
			case 'div3':
			case 'DIV4':
			case 'div4':
			case 'DIV5':
			case 'div5':
			case 'DIV6':
			case 'div6':
			case 'AP41':
			case 'COL0':
			case 'col0':
			case 'COL1':
			case 'col1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX3CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'DIVX':
			case 'divx':
			case 'mp4s':
			case 'MP4S':
			case 'M4S2':
			case 'm4s2':
			case 0x04000000:
			case 'UMP4':

				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'DX50':

				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX5CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case '3IVD':
			case '3ivd':
			case '3IV2':
			case '3iv2':

				err = GetComponentResource((Component)self, codecInfoResourceType, k3ivxCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'XVID':
			case 'xvid':
			case 'XviD':
			case 'XVIX':
			case 'BLZ0':

				err = GetComponentResource((Component)self, codecInfoResourceType, kXVIDCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			// RJVB
			case 'mjp2':
			case '2pjm':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMJP2CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'RMP4':
			case 'SEDG':
			case 'WV1F':
			case 'FMP4':
			case 'SMP4':

			case 'mp4v':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG4CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'H264':
			case 'h264':
			case 'X264':
			case 'x264':
			case 'DAVC':
			case 'VSSH':
			case 'AVC1':
			case 'avc1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kH264CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'FLV1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kFLV1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'FSV1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kFlashSVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'VP60':
			case 'VP61':
			case 'VP62':

			case 'VP6F':
			case 'FLV4':

				err = GetComponentResource((Component)self, codecInfoResourceType, kVP6CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'I263':
			case 'i263':

				err = GetComponentResource((Component)self, codecInfoResourceType, kI263CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'VP30':
			case 'VP31':

				err = GetComponentResource((Component)self, codecInfoResourceType, kVP3CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'HFYU':

			case 'FFVH':

				err = GetComponentResource((Component)self, codecInfoResourceType, kHuffYUVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'MPEG':
			case 'mpg1':
			case 'mp1v':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'MPG2':
			case 'mpg2':
			case 'mp2v':
			case 'm2v1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG2CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'FPS1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kFRAPSCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'SNOW':

				err = GetComponentResource((Component)self, codecInfoResourceType, kSnowCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'RJPG':
			case 'NUV1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kNuvCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'RT21':

				err = GetComponentResource((Component)self, codecInfoResourceType, kIndeo2CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'IV32':
			case 'iv32':
			case 'IV31':
			case 'iv31':

				err = GetComponentResource((Component)self, codecInfoResourceType, kIndeo3CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'IV50':
			case 'iv50':

				err = GetComponentResource((Component)self, codecInfoResourceType, kIndeo5CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'tscc':

				err = GetComponentResource((Component)self, codecInfoResourceType, kTSCCCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'ZMBV':

				err = GetComponentResource((Component)self, codecInfoResourceType, kZMBVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'VP6A':

				err = GetComponentResource((Component)self, codecInfoResourceType, kVP6ACodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'VP80':

				err = GetComponentResource((Component)self, codecInfoResourceType, kVP8CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP90':
				
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP9CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'H265':
			case 'h265':
			case 'X265':
			case 'x265':
			case 'HEV1':
			case 'HEVC':
			case 'hevc':
			case 'hvc1':
			case 'hev1':
				
				err = GetComponentResource((Component)self, codecInfoResourceType, kHEVCCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'FFV1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kFFv1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'PeTh':

				err = GetComponentResource((Component)self, codecInfoResourceType, kTheoraCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case kSubFormatVobSub:

				err = GetComponentResource((Component)self, codecInfoResourceType, kVobSubCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedAVC1:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressAVC1CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedMP4V:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressMP4VCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kSubFormatSSA:

				err = GetComponentResource((Component)self, codecInfoResourceType, kSSASubCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kSubFormatUTF8:

				err = GetComponentResource((Component)self, codecInfoResourceType, kTextSubCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatWMA1MS:

				err = GetComponentResource((Component)self, codecInfoResourceType, kWMA1MSCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatWMA2MS:

				err = GetComponentResource((Component)self, codecInfoResourceType, kWMA2MSCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatFlashADPCM:

				err = GetComponentResource((Component)self, codecInfoResourceType, kFlashADPCMCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatXiphVorbis:

				err = GetComponentResource((Component)self, codecInfoResourceType, kXiphVorbisCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case '.mp1':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG1L1CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case 0x6d730050:

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG1L12CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case '.mp2':

				err = GetComponentResource((Component)self, codecInfoResourceType, kMPEG1L2CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatTTA:

				err = GetComponentResource((Component)self, codecInfoResourceType, kTrueAudioCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatDTS:

				err = GetComponentResource((Component)self, codecInfoResourceType, kDTSCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kAudioFormatNellymoser:

				err = GetComponentResource((Component)self, codecInfoResourceType, kNellymoserCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedAC3:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressAC3CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedMP1:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressMP1CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedMP2:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressMP2CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedMP3:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressMP3CodecResourceID, (Handle *)&tempCodecInfo);
				break;

			case kCompressedDTS:

				err = GetComponentResource((Component)self, codecInfoResourceType, kCompressDTSCodecResourceID, (Handle *)&tempCodecInfo);
				break;

			default:	// should never happen but we have to handle the case
				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
        }

        if (err == noErr)
        {
            *((CodecInfo *)info) = **tempCodecInfo;

            DisposeHandle((Handle)tempCodecInfo);
        }
    }

    return err;
}
