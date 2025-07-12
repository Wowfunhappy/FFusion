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
