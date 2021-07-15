#define thng_RezTemplateVersion 1	// multiplatform 'thng' resource
#define cfrg_RezTemplateVersion 1	// extended version of 'cfrg' resource

#if TARGET_REZ_CARBON_MACHO
#	include <Carbon/Carbon.r>
#	include <QuickTime/QuickTime.r>
#	undef __CARBON_R__
#	undef __CORESERVICES_R__
#	undef __CARBONCORE_R__
#	undef __COMPONENTS_R__
#else
#	include "ConditionalMacros.r"
#	include "MacTypes.r"
#	include "Components.r"
#	include "ImageCodec.r"
#	include "CodeFragments.r"
#	undef __COMPONENTS_R__
// RJVB 20130131: not sure whether defining cmpThreadSafe is necessary nor whether it will actually be taken into account!
#	define cmpThreadSafe	0x10000000	/*  component is thread-safe  */
#endif

#include "FFusionResourceIDs.h"

#define kStartTHNGResID 128
resource 'dlle' (kDivX1CodecInfoResID) {
	"FFusionCodecComponentDispatch"
};

#define kCodecManufacturer kFFusionCodecManufacturer
#define kCodecVersion kFFusionCodecVersion
#define kEntryPointID kDivX1CodecInfoResID

#define kDecompressionFlags ( codecInfoDoes32 | codecInfoDoes16 | codecInfoDoes8 | codecInfoDoes1 | codecInfoDoesTemporal | cmpThreadSafe )
#define kFormatFlags ( codecInfoDepth32 | codecInfoDepth24 | codecInfoDepth16 | codecInfoDepth8 | codecInfoDepth1 )





#define kCodecInfoResID kFFv1CodecInfoResID
#define kCodecName "FFv1"
#define kCodecDescription "Decompresses video stored in FFv1 format."
#define kCodecSubType 'FFV1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kHuffYUVCodecInfoResID
#define kCodecName "HuffYUV"
#define kCodecDescription "Decompresses videos stored in the HuffYUV format."
#define kCodecSubType 'HFYU'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'FFVH'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kFRAPSCodecInfoResID
#define kCodecName "Fraps"
#define kCodecDescription "Decompresses videos stored in the Fraps format."
#define kCodecSubType 'FPS1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kZMBVCodecInfoResID
#define kCodecName "DosBox Capture"
#define kCodecDescription "Decompresses videos stored in the DosBox Capture format."
#define kCodecSubType 'ZMBV'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kDivX3CodecInfoResID
#define kCodecName "DivX 3"
#define kCodecDescription "Decompresses videos stored in the DivX 3.11 alpha format."
#define kCodecSubType 'MPG3'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'mpg3'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'MP43'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'mp43'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'DIV3'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'div3'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'DIV4'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'div4'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'DIV5'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'div5'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'DIV6'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'div6'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'AP41'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'COL0'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'col0'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'COL1'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'col1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kDivX4CodecInfoResID
#define kCodecName "DivX 4"
#define kCodecDescription "Decompresses videos stored in the OpenDivX format."
#define kCodecSubType 'DIVX'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'divx'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'mp4s'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'MP4S'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'M4S2'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'm4s2'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'UMP4'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID k3ivxCodecInfoResID
#define kCodecName "3ivx"
#define kCodecDescription "Decompresses videos stored in the 3ivx format."
#define kCodecSubType '3IVD'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType '3ivd'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType '3IV2'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType '3iv2'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kXVIDCodecInfoResID
#define kCodecName "Xvid"
#define kCodecDescription "Decompresses videos stored in the Xvid format."
#define kCodecSubType 'XVID'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'xvid'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'XviD'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'XVIX'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'BLZ0'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kFLV1CodecInfoResID
#define kCodecName "Sorenson Spark"
#define kCodecDescription "Decompresses videos stored in the Sorenson Spark format."
#define kCodecSubType 'FLV1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kFlashSVCodecInfoResID
#define kCodecName "Flash Screen Video"
#define kCodecDescription "Decompresses videos stored in the Flash Screen Video format."
#define kCodecSubType 'FSV1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kSnowCodecInfoResID
#define kCodecName "Snow"
#define kCodecDescription "Decompresses videos stored in the Snow format."
#define kCodecSubType 'SNOW'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kIndeo2CodecInfoResID
#define kCodecName "Indeo 2"
#define kCodecDescription "Decompresses videos stored in Intel's Indeo 2 format."
#define kCodecSubType 'RT21'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kIndeo3CodecInfoResID
#define kCodecName "Indeo 3"
#define kCodecDescription "Decompresses videos stored in Intel's Indeo 3 format."
#define kCodecSubType 'IV32'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'iv32'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'IV31'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'iv31'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kIndeo4CodecInfoResID
#define kCodecName "Indeo 4"
#define kCodecDescription "Decompresses videos stored in Intel's Indeo 4 format."
#define kCodecSubType 'IV41'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'iv41'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kIndeo5CodecInfoResID
#define kCodecName "Indeo 5"
#define kCodecDescription "Decompresses videos stored in Intel's Indeo 5 format."
#define kCodecSubType 'IV50'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'iv50'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kI263CodecInfoResID
#define kCodecName "Intel H.263"
#define kCodecDescription "Decompresses videos stored in the Intel H.263 format."
#define kCodecSubType 'I263'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'i263'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kH265CodecInfoResID
#define kCodecName "H.265"
#define kCodecDescription "Decompresses video stored in the H.265 format."
#define kCodecSubType 'H265'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'h265'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'X265'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'x265'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'HEV1'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'hev1'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'HEVC'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'hevc'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'hvc1'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kVP3CodecInfoResID
#define kCodecName "TrueMotion VP3"
#define kCodecDescription "Decompresses videos stored in On2's VP3 format."
#define kCodecSubType 'VP30'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'VP31'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kVP6CodecInfoResID
#define kCodecName "TrueMotion VP6"
#define kCodecDescription "Decompresses videos stored in On2's TrueMotion VP6 format."
#define kCodecSubType 'VP60'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'VP61'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'VP62'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'VP6F'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'FLV4'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kVP6ACodecInfoResID
#define kCodecName "On2 VP6A"
#define kCodecDescription "Decompresses videos stored in On2's VP6A format."
#define kCodecSubType 'VP6A'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kVP8CodecInfoResID
#define kCodecName "On2 VP8"
#define kCodecDescription "Decompresses videos stored in On2's VP8 format."
#define kCodecSubType 'VP80'
#include "Resources/FFusionResourceInc.r"

#define kCodecInfoResID kVP9CodecInfoResID
#define kCodecName "Google VP9"
#define kCodecDescription "Decompresses videos stored in Google's VP9 format."
#define kCodecSubType 'VP90'
#include "Resources/FFusionResourceInc.r"
#define kCodecSubType 'vp09'
#include "Resources/FFusionResourceInc.r"





//#define AudioComponentType
//#define decompressorComponentType 'adec'
//#ifndef cmpThreadSafeOnMac
//    #define cmpThreadSafeOnMac 0x00000000
//#endif
//#define kDecompressionFlags cmpThreadSafeOnMac

//#define kCodecInfoResID kEAC3CodecInfoResID
//#define kCodecName "E-AC3"
//#define kCodecDescription "Decompresses audio stored in the EAC3 format."
//#define kCodecSubType 'ec-3'
//#include "Resources/FFusionResourceInc.r"
