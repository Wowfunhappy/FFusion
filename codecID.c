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
		case 'VP90':
		case 'vp09':
		case 'VP09':
		case 'vp90':
			codecID = AV_CODEC_ID_VP9;
			break;
			
		case 'AV01':
		case 'av01':
		case 'AV1 ':
		case 'av1 ':
			codecID = AV_CODEC_ID_AV1;
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
			case 'VP90':
			case 'vp09':
			case 'VP09':
			case 'vp90':
				err = GetComponentResource((Component)self, codecInfoResourceType, kVP9CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'AV01':
			case 'av01':
			case 'AV1 ':
			case 'av1 ':
				err = GetComponentResource((Component)self, codecInfoResourceType, kAV1CodecInfoResID, (Handle *)&tempCodecInfo);
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
