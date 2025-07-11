#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "libavcodec/avcodec.h"
#include <QuickTime/QuickTime.h>
#include "CodecIDs.h"
#include "FFusionResourceIDs.h"

int getCodecID(OSType componentType)
{
	enum AVCodecID codecID = AV_CODEC_ID_NONE;
	switch(componentType)
	{
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
			
		case 'VP90':
		case 'vp09':
		case 'VP09':
		case 'vp90':
			codecID = AV_CODEC_ID_VP9;
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
				
			case 'VP90':
			case 'vp09':
			case 'VP09':
			case 'vp90':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP9CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			default:	// unsupported codec
				err = featureUnsupported;
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
