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
	enum AVCodecID codecID = AV_CODEC_ID_NONE;
	switch(componentType)
	{
		case 'FFV1':
			codecID = AV_CODEC_ID_FFV1;
			break;
		case 'HFYU':
			codecID = AV_CODEC_ID_HUFFYUV;
			break;
		case 'FFVH':
			codecID = AV_CODEC_ID_FFVHUFF;
			break;
		case 'FPS1':
			codecID = AV_CODEC_ID_FRAPS;
			break;
		case 'ZMBV':
			codecID = AV_CODEC_ID_ZMBV;
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
			codecID = AV_CODEC_ID_MSMPEG4V3;
			break;
	
		case 'DIVX':
  		case 'divx':
  		case 'mp4s':
  		case 'MP4S':
  		case 'M4S2':
  		case 'm4s2':
  		case 'UMP4':
		//---
		case '3IVD':
  		case '3ivd':
  		case '3IV2':
  		case '3iv2':
		//---
		case 'XVID':
  		case 'xvid':
  		case 'XviD':
  		case 'XVIX':
  		case 'BLZ0':
			codecID = AV_CODEC_ID_MPEG4;
			break;
		
		case 'FLV1':
			codecID = AV_CODEC_ID_FLV1;
			break;
		case 'FSV1':
			codecID = AV_CODEC_ID_FLASHSV;
			break;
			
		case 'SNOW':
			codecID = AV_CODEC_ID_SNOW;
			break;
			
		case 'RT21':
			codecID = AV_CODEC_ID_INDEO2;
			break;
			
		case 'IV32':
  		case 'iv32':
  		case 'IV31':
  		case 'iv31':
			codecID = AV_CODEC_ID_INDEO3;
			break;
		
		case 'IV41':
		case 'iv41':
			codecID = AV_CODEC_ID_INDEO4;
			break;
			
		case 'IV50':
  		case 'iv50':
			codecID = AV_CODEC_ID_INDEO5;
			break;
			
		case 'I263':
  		case 'i263':
			codecID = AV_CODEC_ID_H263I;
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
		
		case 'VP30':
  		case 'VP31':
			codecID = AV_CODEC_ID_VP3;
			break;
			
		case 'VP60':
  		case 'VP61':
  		case 'VP62':
			codecID = AV_CODEC_ID_VP6;
			break;
			
		case 'VP6F':
  		case 'FLV4':
			codecID = AV_CODEC_ID_VP6F;
			break;

		case 'VP6A':
			codecID = AV_CODEC_ID_VP6A;
			break;
			
		case 'VP80':
			codecID = AV_CODEC_ID_VP8;
			break;
			
		case 'VP90':
		case 'vp09':
		case 'VP09':
		case 'vp90':
			codecID = AV_CODEC_ID_VP9;
			break;
			
		//case 'ec-3':
			//codecID = AV_CODEC_ID_EAC3;
			//break;
		
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
			case 'FFV1':
				err = GetComponentResource((Component)self, codecInfoResourceType, kFFv1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'HFYU':
			case 'FFVH':
				err = GetComponentResource((Component)self, codecInfoResourceType, kHuffYUVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FPS1':
				err = GetComponentResource((Component)self, codecInfoResourceType, kFRAPSCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'ZMBV':
				err = GetComponentResource((Component)self, codecInfoResourceType, kZMBVCodecInfoResID, (Handle *)&tempCodecInfo);
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
			case 'UMP4':
				err = GetComponentResource((Component)self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
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
				
			case 'FLV1':
				err = GetComponentResource((Component)self, codecInfoResourceType, kFLV1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FSV1':
				err = GetComponentResource((Component)self, codecInfoResourceType, kFlashSVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;

			case 'SNOW':
				err = GetComponentResource((Component)self, codecInfoResourceType, kSnowCodecInfoResID, (Handle *)&tempCodecInfo);
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
				
			case 'IV41':
			case 'iv41':
				err = GetComponentResource((Component)self, codecInfoResourceType, kIndeo4CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'IV50':
			case 'iv50':
				err = GetComponentResource((Component)self, codecInfoResourceType, kIndeo5CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'I263':
			case 'i263':
				err = GetComponentResource((Component)self, codecInfoResourceType, kI263CodecInfoResID, (Handle *)&tempCodecInfo);
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
				err = GetComponentResource((Component)self, codecInfoResourceType, kH265CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
	
			case 'VP30':
			case 'VP31':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP3CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP60':
			case 'VP61':
			case 'VP62':
			//---
			case 'VP6F':
			case 'FLV4':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP6CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP6A':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP6ACodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP80':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP8CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP90':
			case 'vp09':
			case 'VP09':
			case 'vp90':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP9CodecInfoResID, (Handle *)&tempCodecInfo);
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
