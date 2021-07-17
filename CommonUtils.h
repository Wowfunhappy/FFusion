/*
 * CommonUtils.h
 * Created by David Conrad on 10/13/06.
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

#ifndef __COMMONUTILS_H__
#define __COMMONUTILS_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#ifdef _MSC_VER
	// prevent the GNU compatibility stdint.h header included with the QuickTime SDK from being included:
#	define _STDINT_H
#endif
#ifdef __MACH__
#	include <QuickTime/QuickTime.h>
#else
#	include <ConditionalMacros.h>
#	include <Endian.h>
#	include <ImageCodec.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER)
	extern char *ProgName();
#endif

#ifndef FFUSION_CODEC_ONLY
// ISO 639-1 to language ID expected by SetMediaLanguage
short ISO639_1ToQTLangCode(const char *lang);

// ISO 639-2 to language ID expected by SetMediaLanguage
short ISO639_2ToQTLangCode(const char *lang);
#endif

/* write the int32_t data to target & then return a pointer which points after that data */
uint8_t *write_int32(uint8_t *target, int32_t data);

/* write the int16_t data to target & then return a pointer which points after that data */
uint8_t *write_int16(uint8_t *target, int16_t data);

/* write the data to the target adress & then return a pointer which points after the written data */
uint8_t *write_data(uint8_t *target, uint8_t* data, int32_t data_size);

// mallocs the buffer and copies the codec-specific description to it, in the same format
// as is specified in Matroska and is used in libavcodec
ComponentResult ReadESDSDescExt(Handle descExt, UInt8 **buffer, int *size);

int isImageDescriptionExtensionPresent(ImageDescriptionHandle desc, long type);

// does the current process break if we signal droppable frames?
int IsFrameDroppingEnabled();

// does the current process break if we return errors in Preflight?
int IsForcedDecodeEnabled();

int IsAltivecSupported();

#ifndef FFUSION_CODEC_ONLY
	// is this font name known to be incompatible with ATSUI?
	int ShouldImportFontFileName(const char *filename);
	// does the current process break if we use graphicsModePreBlackAlpha?
	int IsTransparentSubtitleHackEnabled();
#endif
	
// can we safely create an HE-AAC track with a frequency of more than 48khz?
int ShouldPlayHighFreqSBR();
	
// CFPreferencesCopyAppValue() wrapper which checks the type of the value returned
CFPropertyListRef CopyPreferencesValueTyped(CFStringRef key, CFTypeID type);

// critical region for initializing stuff
// component/ffmpeg initialization should be the only thing that really needs a mutex
int FFusionInitEnter(volatile Boolean *inited);
void FFusionInitExit(int unlock);

void *fast_realloc_with_padding(void *ptr, unsigned int *size, unsigned int min_size);

// return an sRGB colorspace. safe to use on 10.4.
CGColorSpaceRef GetSRGBColorSpace();

// postprocess RGBA+8-bit to not look terrible when displayed with 'transparent' blend mode
void ConvertImageToQDTransparent(Ptr baseAddr, OSType pixelFormat, int rowBytes, int width, int height);
	
#define FFUSION_PREF_DOMAIN CFSTR("org.perian.FFusion")

#ifdef _MSC_VER
#	define DLLIMPORT	__declspec(dllimport)
#	define _CRT_SECURE_NO_WARNINGS	1
#else
#	define DLLIMPORT	/**/
#endif
	
#if TARGET_OS_WIN32
#	define IntToFixed(a)       ((Fixed)(a) << 16)
#	define FloatToFixed(a) (_IntSaturate((a) * fixed1))
#	define _IntSaturate(x) ((int)(x) <= (int)-2147483648 ? (int) -2147483648 : (int)2147483648 <= (int)(x) ? (int) 0x7fffffff : (int) (x))
#endif

#ifdef __cplusplus
}
#endif

#endif





//Wowfunhappy

#ifndef SUINT
#define SUINT   unsigned
#endif

#ifndef SUINT32
#define SUINT32 uint32_t
#endif