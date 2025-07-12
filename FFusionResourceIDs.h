

#include "CodecIDs.h"

#define kPerianManufacturer		'Peri'

#define kFFusionCodecManufacturer	kPerianManufacturer
#define kTextCodecManufacturer		kPerianManufacturer
#define kVobSubCodecManufacturer	kPerianManufacturer
#define kFFissionCodecManufacturer	kPerianManufacturer
#define kCompressCodecManufacturer	kPerianManufacturer

#define kPerianBaseVersion			(0x00000004)
#define kFFusionCodecVersion		(kPerianBaseVersion+0x00030003)
#define kTextSubCodecVersion		kPerianBaseVersion
#define kVobSubCodecVersion			kPerianBaseVersion
#define kFFissionCodecVersion		kFFusionCodecVersion
#define kMatroskaImportVersion		(kPerianBaseVersion+0x00020001)
#define kFFAviComponentVersion		(kPerianBaseVersion+0x00010003)
#define kCompressCodecVersion		kPerianBaseVersion
		
enum{

	kDivX1CodecInfoResID = 128,
	kDivX2CodecInfoResID,
	kDivX3CodecInfoResID,
	kDivX4CodecInfoResID,
	kDivX5CodecInfoResID,
	k3ivxCodecInfoResID,
	kXVIDCodecInfoResID,
	kMPEG4CodecInfoResID,

	kH264CodecInfoResID,
	kFLV1CodecInfoResID,
	kFlashSVCodecInfoResID,
	kVP6CodecInfoResID,

	kI263CodecInfoResID,
	kVP3CodecInfoResID,
	kHuffYUVCodecInfoResID,

	kMPEG1CodecInfoResID,
	kMPEG2CodecInfoResID,
	kFRAPSCodecInfoResID,
	kSnowCodecInfoResID,
	kNuvCodecInfoResID,
	kIndeo2CodecInfoResID,
	kIndeo3CodecInfoResID,
    kIndeo4CodecInfoResID,
	kIndeo5CodecInfoResID,
	kTSCCCodecInfoResID,
	kZMBVCodecInfoResID,
	kVP6ACodecInfoResID,
	kVP8CodecInfoResID,
	kFFv1CodecInfoResID,
	kTheoraCodecInfoResID,
    
    kH265CodecInfoResID,
    
    kEAC3CodecInfoResID,

	kVobSubCodecResourceID,

	kCompressAVC1CodecResourceID,
	kCompressMP4VCodecResourceID,

	kSSASubCodecResourceID,
	kTextSubCodecResourceID,

	kWMA1MSCodecResourceID,
	kWMA2MSCodecResourceID,
	kFlashADPCMCodecResourceID,

	kXiphVorbisCodecResourceID,
	kMPEG1L1CodecResourceID,
	kMPEG1L12CodecResourceID,
	kMPEG1L2CodecResourceID,
	kTrueAudioCodecResourceID,
	kDTSCodecResourceID,
	kNellymoserCodecResourceID,

	kCompressAC3CodecResourceID,
	kCompressMP1CodecResourceID,
	kCompressMP2CodecResourceID,
	kCompressMP3CodecResourceID,
	kCompressDTSCodecResourceID,

	// RJVB
	kMJP2CodecInfoResID,
};
