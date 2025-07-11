#include <asl.h>

/*
 * ColorConversions.c
 * Created by Alexander Strange on 1/10/07.
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

#include <math.h>
#include "Codecprintf.h"
#include <QuickTime/QuickTime.h>
#include <Accelerate/Accelerate.h>
#include "ColorConversions.h"
#include "CommonUtils.h"
#include <libswscale/swscale.h>

/*
 Converts (without resampling) from ffmpeg pixel formats to the ones QT accepts
 
 Todo:
 - rewrite everything in asm (or C with all loop optimization opportunities removed)
 - add a version with bilinear resampling
 - handle YUV 4:2:0 with odd width
 */

#ifdef __GNUC__
#	define unlikely(x) __builtin_expect(x, 0)
#	define likely(x) __builtin_expect(x, 1)
#   define impossible(x) if (x) __builtin_unreachable()
#else
#	define unlikely(x)	(x)
#	define likely(x)	(x)
#   define impossible(x)
#endif

//Handles the last row for Y420 videos with an odd number of luma rows
//FIXME: odd number of luma columns is not handled and they will be lost
static void Y420toY422_lastrow(UInt8 *o, UInt8 *yc, UInt8 *uc, UInt8 *vc, int halfWidth)
{
	int x;
	for(x=0; x < halfWidth; x++)
	{
		int x4 = x*4, x2 = x*2;

		o[x4]   = uc[x];
		o[++x4] = yc[x2];
		o[++x4] = vc[x];
		o[++x4] = yc[++x2];
	}
}

#define HandleLastRow(o, yc, uc, vc, halfWidth, height) if (unlikely(height & 1)) Y420toY422_lastrow(o, yc, uc, vc, halfWidth)

//Y420 Planar to Y422 Packed
//The only one anyone cares about, so implemented with SIMD

#include <emmintrin.h>

static FASTCALL void Y420toY422_sse2(AVPicture *picture, UInt8 *o, long outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *uc = picture->data[1], *vc = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		y, x, halfwidth = width >> 1, halfheight = height >> 1;
	int		vWidth = width >> 5;

// 	Codecprintf( stderr, "Y420toY422_sse2(%p,%p,outRB=%d,w=%d,h=%d,rY=%d,rUV=%d,hw=%d,hh=%d,vw=%d)\n",
// 		picture, o, outRB, width, height, rY, rUV, halfwidth, halfheight, vWidth );
	for (y = 0; y < halfheight; y++) {
		UInt8   *o2 = o + outRB,   *yc2 = yc + rY;
		__m128i *ov = (__m128i*)o, *ov2 = (__m128i*)o2, *yv = (__m128i*)yc, *yv2 = (__m128i*)yc2;
		__m128i *uv = (__m128i*)uc,*vv  = (__m128i*)vc;
// RJVB
// 20130203: nothing to fix here. Processing speed differences between the GCC-style inline assembly and
// SSE2 intrinsics are minimal (using GCC); there is no reason to assume that this would be different with MSVC.
// Also, Y420toY422_sse2() execution speed for a 720x576 image with outRB=1440 and linesize=752,376
// is between approx. 900Hz (1Ghz AMD C60 netbook) and 10000Hz (Core i7, 2.7Ghz), meaning that the function is not a bottleneck
// for "normal" resolutions.
		int vWidth_ = vWidth;

		asm volatile(
			"\n0:			\n\t"
			"movdqa		(%2),	%%xmm0	\n\t"
			"movdqa		16(%2),	%%xmm2	\n\t"
			"movdqa		(%3),		%%xmm1	\n\t"
			"movdqa		16(%3),	%%xmm3	\n\t"
			"movdqu		(%4),	%%xmm4	\n\t"
			"movdqu		(%5),	%%xmm5	\n\t"
			"addl		$32,	%2		\n\t"
			"addl		$32,	%3		\n\t"
			"addl		$16,	%4		\n\t"
			"addl		$16,	%5		\n\t"
			"movdqa		%%xmm4, %%xmm6	\n\t"
			"punpcklbw	%%xmm5, %%xmm4	\n\t" /*chroma_l*/
			"punpckhbw	%%xmm5, %%xmm6	\n\t" /*chroma_h*/
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpcklbw	%%xmm0, %%xmm5	\n\t"
			"movntdq	%%xmm5, (%0)	\n\t" /*ov[x4]*/
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpckhbw	%%xmm0, %%xmm5	\n\t"
			"movntdq	%%xmm5, 16(%0)	\n\t" /*ov[x4+1]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpcklbw	%%xmm2, %%xmm5	\n\t"
			"movntdq	%%xmm5, 32(%0)	\n\t" /*ov[x4+2]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpckhbw	%%xmm2, %%xmm5	\n\t"
			"movntdq	%%xmm5, 48(%0)	\n\t" /*ov[x4+3]*/
			"addl		$64,	%0		\n\t"
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpcklbw	%%xmm1, %%xmm5	\n\t"
			"movntdq	%%xmm5, (%1)	\n\t" /*ov2[x4]*/
			"punpckhbw	%%xmm1, %%xmm4	\n\t"
			"movntdq	%%xmm4, 16(%1)	\n\t" /*ov2[x4+1]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpcklbw	%%xmm3, %%xmm5	\n\t"
			"movntdq	%%xmm5, 32(%1)	\n\t" /*ov2[x4+2]*/
			"punpckhbw	%%xmm3, %%xmm6	\n\t"
			"movntdq	%%xmm6, 48(%1)	\n\t" /*ov2[x4+3]*/
			"addl		$64,	%1		\n\t"
			"decl		%6				\n\t"
			"jnz		0b				\n\t"
			: "+r" (ov), "+r" (ov2), "+r" (yv),
			  "+r" (yv2), "+r" (uv), "+r" (vv), "+m"(vWidth_)
			:
			: "memory", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
			);

		for (x=vWidth * 16; x < halfwidth; x++) {
		  int x4 = x*4, x2 = x*2;
			o2[x4]     = o[x4] = uc[x];
//			o [x4 + 1] = yc[x2];
//			o2[x4 + 1] = yc2[x2];
//			o2[x4 + 2] = o[x4 + 2] = vc[x];
//			o [x4 + 3] = yc[x2 + 1];
//			o2[x4 + 3] = yc2[x2 + 1];
			x4++;
			o [x4] = yc[x2], o2[x4] = yc2[x2];
			x4++;
			o2[x4] = o[x4] = vc[x];
			x4++, x2++;
			o [x4] = yc[x2], o2[x4] = yc2[x2];
		}			
		
		o  += outRB*2;
		yc += rY*2;
		uc += rUV;
		vc += rUV;
	}

	HandleLastRow(o, yc, uc, vc, halfwidth, height);
}

static FASTCALL void Y420toY422_x86_scalar(AVPicture *picture, UInt8 *o, long outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		halfheight = height >> 1, halfwidth = width >> 1;
	int		y, x;
	
	for (y = 0; y < halfheight; y ++) {
		UInt8 *o2 = o + outRB, *yc2 = yc + rY;
		
		for (x = 0; x < halfwidth; x++) {
			int x4 = x*4, x2 = x*2;
			o2[x4]     = o[x4] = u[x];
//			o [x4 + 1] = yc[x2];
//			o2[x4 + 1] = yc2[x2];
//			o2[x4 + 2] = o[x4 + 2] = v[x];
//			o [x4 + 3] = yc[x2 + 1];
//			o2[x4 + 3] = yc2[x2 + 1];
			o [++x4] = yc[x2];
			o2[x4] = yc2[x2];
			x4++;
			o2[x4] = o[x4] = v[x];
			o [++x4] = yc[++x2];
			o2[x4] = yc2[x2];
		}
		
		o  += outRB*2;
		yc += rY*2;
		u  += rUV;
		v  += rUV;
	}

	HandleLastRow(o, yc, u, v, halfwidth, height);
}

//Y420+Alpha Planar to V408 (YUV 4:4:4+Alpha 32-bit packed)
//Could be fully unrolled to avoid x/2
static FASTCALL void YA420toV408(AVPicture *picture, UInt8 *o, long outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2], *a = picture->data[3];
	int		rYA = picture->linesize[0], rUV = picture->linesize[1];
	int y, x;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		  int x4 = x*4;
			o[x4]   = u[x>>1];
			o[++x4] = yc[x];
			o[++x4] = v[x>>1];
			o[++x4] = a[x];
		}
		
		o  += outRB;
		yc += rYA;
		a  += rYA;
		if (y & 1) {
			u += rUV;
			v += rUV;
		}
	}
}

static FASTCALL void BGR24toRGB24(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x3 = x * 3;
			baseAddr[x3] = srcPtr[x3+2];
			baseAddr[x3+1] = srcPtr[x3+1];
			baseAddr[x3+2] = srcPtr[x3];
		}
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

static FASTCALL void RGBtoRGB(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height, int bytesPerPixel)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int y;
	
	for (y = 0; y < height; y++) {
		memcpy(baseAddr, srcPtr, width * bytesPerPixel);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

static FASTCALL void RGBtoRGBXOR(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height, int bytesPerPixel)
{
	UInt8 *srcPtr = picture->data[0], *bA = baseAddr;
	int srcRB = picture->linesize[0];
	size_t y, size;
	
	for (y = 0; y < height; y++) {
		memcpy(baseAddr, srcPtr, width * bytesPerPixel);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
	size = height * rowBytes;
	for( y = 0, baseAddr = bA ; y < rowBytes ; y++, baseAddr++ ){
		*baseAddr ^= 0xff;
	}
}

//Big-endian XRGB32 to big-endian XRGB32
static FASTCALL void RGB32toRGB32Copy(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height)
{
	RGBtoRGB(picture, baseAddr, rowBytes, width, height, 4);
}

// RJVB
static FASTCALL void RGB32toARGB( AVPicture *picture, register UInt8 *baseAddr, long rowBytes, register int width, register int height )
{ register UInt8 *srcPtr = picture->data[0];
  register int x, y;
  int srcRB;

	srcRB = picture->linesize[0] - width * 4, rowBytes -= width * 4;
	for( y = 0; y < height; y++ ){
		for( x = 0 ; x < width ; x++ ){
			*baseAddr++ = srcPtr[3];
			memcpy( baseAddr, srcPtr, 3 );
			baseAddr += 3, srcPtr += 4;
		}

		baseAddr += rowBytes, srcPtr += srcRB;
	}
}

static FASTCALL void RGB24toRGB24(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height)
{
	RGBtoRGB(picture, baseAddr, rowBytes, width, height, 3);
}



//Little-endian XRGB32 to big-endian XRGB32
static FASTCALL void RGB32toRGB32Swap(AVPicture *picture, UInt8 *baseAddr, long rowBytes, int width, int height)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int x, y;
	
	for (y = 0; y < height; y++) {
		UInt32 *oRow = (UInt32 *)baseAddr, *iRow = (UInt32 *)srcPtr;
		for (x = 0; x < width; x++) oRow[x] = EndianU32_LtoB(iRow[x]);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}


static FASTCALL void Y422toY422(AVPicture *picture, UInt8 *o, long outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		x, y, halfwidth = width >> 1;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < halfwidth; x++) {
			int x2 = x * 2, x4 = x * 4;
			o[x4] = u[x];
			o[++x4] = yc[x2];
			o[++x4] = v[x];
			o[++x4] = yc[x2 + 1];
		}
		
		o  += outRB;
		yc += rY;
		u  += rUV;
		v  += rUV;
	}
}


static FASTCALL void Y420_10toY422_8_sse2(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt16	*yc = (UInt16*)picture->data[0], *u = (UInt16*)picture->data[1], *v = (UInt16*)picture->data[2];
	int		rY = picture->linesize[0]/2, rUV = picture->linesize[1]/2;
	int		halfheight = height >> 1, halfwidth = width >> 1;
	int		y, x;
	int		vWidth = halfwidth >> 3;
	
	impossible(width <= 1 || height <= 1 || outRB <= 0 || rY <= 0 || rUV <= 0);
	
	for (y = 0; y < halfheight; y++) {
		UInt8 *o2 = o + outRB;
		UInt16 *yc2 = yc + rY;
		
		for (x = 0; x < vWidth; x++) {
			__m128i y_row1 = _mm_loadu_si128((__m128i*)(yc + x*16));
			__m128i y_row1_2 = _mm_loadu_si128((__m128i*)(yc + x*16 + 8));
			__m128i y_row2 = _mm_loadu_si128((__m128i*)(yc2 + x*16));
			__m128i y_row2_2 = _mm_loadu_si128((__m128i*)(yc2 + x*16 + 8));
			__m128i u_vals = _mm_loadu_si128((__m128i*)(u + x*8));
			__m128i v_vals = _mm_loadu_si128((__m128i*)(v + x*8));
			
			y_row1 = _mm_srli_epi16(y_row1, 2);
			y_row1_2 = _mm_srli_epi16(y_row1_2, 2);
			y_row2 = _mm_srli_epi16(y_row2, 2);
			y_row2_2 = _mm_srli_epi16(y_row2_2, 2);
			u_vals = _mm_srli_epi16(u_vals, 2);
			v_vals = _mm_srli_epi16(v_vals, 2);
			
			__m128i y_packed1 = _mm_packus_epi16(y_row1, y_row1_2);
			__m128i y_packed2 = _mm_packus_epi16(y_row2, y_row2_2);
			__m128i u_packed = _mm_packus_epi16(u_vals, _mm_setzero_si128());
			__m128i v_packed = _mm_packus_epi16(v_vals, _mm_setzero_si128());
			
			__m128i u_dup = _mm_unpacklo_epi8(u_packed, u_packed);
			__m128i v_dup = _mm_unpacklo_epi8(v_packed, v_packed);
			
			__m128i uv_lo = _mm_unpacklo_epi8(u_dup, v_dup);
			__m128i uv_hi = _mm_unpackhi_epi8(u_dup, v_dup);
			
			__m128i uyvy1_lo = _mm_unpacklo_epi8(uv_lo, y_packed1);
			__m128i uyvy1_hi = _mm_unpackhi_epi8(uv_lo, y_packed1);
			__m128i uyvy2_lo = _mm_unpacklo_epi8(uv_hi, y_packed2);
			__m128i uyvy2_hi = _mm_unpackhi_epi8(uv_hi, y_packed2);
			
			_mm_storeu_si128((__m128i*)(o + x*32), uyvy1_lo);
			_mm_storeu_si128((__m128i*)(o + x*32 + 16), uyvy1_hi);
			_mm_storeu_si128((__m128i*)(o2 + x*32), uyvy2_lo);
			_mm_storeu_si128((__m128i*)(o2 + x*32 + 16), uyvy2_hi);
		}
		
		for (x = vWidth * 8; x < halfwidth; x++) {
			UInt8 u_val = u[x] >> 2;
			UInt8 v_val = v[x] >> 2;
			UInt8 y0 = yc[x*2] >> 2;
			UInt8 y1 = yc[x*2+1] >> 2;
			UInt8 y2 = yc2[x*2] >> 2;
			UInt8 y3 = yc2[x*2+1] >> 2;
			
			int x4 = x*4;
			o[x4] = u_val;
			o[x4+1] = y0;
			o[x4+2] = v_val;
			o[x4+3] = y1;
			
			o2[x4] = u_val;
			o2[x4+1] = y2;
			o2[x4+2] = v_val;
			o2[x4+3] = y3;
		}
		
		o  += outRB*2;
		yc += rY*2;
		u  += rUV;
		v  += rUV;
	}
	
	if (unlikely(height & 1)) {
		for(x = 0; x < halfwidth; x++) {
			UInt8 u_val = u[x] >> 2;
			UInt8 v_val = v[x] >> 2;
			UInt8 y0 = yc[x*2] >> 2;
			UInt8 y1 = yc[x*2+1] >> 2;
			
			int x4 = x*4;
			o[x4] = u_val;
			o[x4+1] = y0;
			o[x4+2] = v_val;
			o[x4+3] = y1;
		}
	}
}

static FASTCALL void Y420_10toY422_8(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt16	*yc = (UInt16*)picture->data[0], *u = (UInt16*)picture->data[1], *v = (UInt16*)picture->data[2];
	int		rY = picture->linesize[0]/2, rUV = picture->linesize[1]/2;
	int		halfheight = height >> 1, halfwidth = width >> 1;
	int		y, x;
	
	impossible(width <= 1 || height <= 1 || outRB <= 0 || rY <= 0 || rUV <= 0);
	
	for (y = 0; y < halfheight; y ++) {
		UInt8 *o2 = o + outRB;
		UInt16 *yc2 = yc + rY;
		UInt8 *op = o, *op2 = o2;
		UInt16 *yp = yc, *yp2 = yc2, *up = u, *vp = v;
		
		for (x = 0; x < halfwidth; x++) {
			UInt8 u_val = *up++ >> 2;
			UInt8 v_val = *vp++ >> 2;
			UInt8 y0 = *yp++ >> 2;
			UInt8 y1 = *yp++ >> 2;
			UInt8 y2 = *yp2++ >> 2;
			UInt8 y3 = *yp2++ >> 2;
			
			*op++ = u_val;
			*op++ = y0;
			*op++ = v_val;
			*op++ = y1;
			
			*op2++ = u_val;
			*op2++ = y2;
			*op2++ = v_val;
			*op2++ = y3;
		}
		
		o  += outRB*2;
		yc += rY*2;
		u  += rUV;
		v  += rUV;
	}
	
	if (unlikely(height & 1)) {
		UInt8 *op = o;
		UInt16 *yp = yc, *up = u, *vp = v;
		
		for(x = 0; x < halfwidth; x++) {
			UInt8 u_val = *up++ >> 2;
			UInt8 v_val = *vp++ >> 2;
			UInt8 y0 = *yp++ >> 2;
			UInt8 y1 = *yp++ >> 2;
			
			*op++ = u_val;
			*op++ = y0;
			*op++ = v_val;
			*op++ = y1;
		}
	}
}

static void ClearRGB(UInt8 *baseAddr, long rowBytes, int width, int height, int bytesPerPixel)
{
	int y;
	
	for (y = 0; y < height; y++) {
		memset(baseAddr, 0, width * bytesPerPixel);
		
		baseAddr += rowBytes;
	}
}

static FASTCALL void ClearRGB32(UInt8 *baseAddr, long rowBytes, int width, int height)
{
	ClearRGB(baseAddr, rowBytes, width, height, 4);
}

static FASTCALL void ClearRGB24(UInt8 *baseAddr, long rowBytes, int width, int height)
{
	ClearRGB(baseAddr, rowBytes, width, height, 3);
}


static FASTCALL void ClearV408(UInt8 *baseAddr, long rowBytes, int width, int height)
{
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x4 = x * 4;
			baseAddr[x4]   = 0x80; //zero chroma
			baseAddr[++x4] = 0x10; //black
			baseAddr[++x4] = 0x80; 
			baseAddr[++x4] = 0xEB; //opaque
		}
		baseAddr += rowBytes;
	}
}

static FASTCALL void ClearY422(UInt8 *baseAddr, long rowBytes, int width, int height)
{
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x2 = x * 2;
			baseAddr[x2]   = 0x80; //zero chroma
			baseAddr[x2+1] = 0x10; //black
		}
		baseAddr += rowBytes;
	}
}

OSType ColorConversionDstForPixFmt(enum AVCodecID codecID, enum AVPixelFormat ffPixFmt)
{
	switch (ffPixFmt) {
		case AV_PIX_FMT_BGR24:
		case AV_PIX_FMT_RGB24:
			return k24RGBPixelFormat;
		case AV_PIX_FMT_ARGB:
		case AV_PIX_FMT_BGRA:
		case AV_PIX_FMT_RGBA:
			return k32ARGBPixelFormat;
		case AV_PIX_FMT_YUVJ420P:
		case AV_PIX_FMT_YUV420P:
			return k2vuyPixelFormat;
		case AV_PIX_FMT_YUV422P:
			return k2vuyPixelFormat;
		case AV_PIX_FMT_YUVA420P:
			return k4444YpCbCrA8PixelFormat;
		case AV_PIX_FMT_YUV420P10LE:
			return k2vuyPixelFormat;
		default:
			return k2vuyPixelFormat;
	}
}

int ColorConversionFindFor( ColorConversionFuncs *funcs, enum AVCodecID codecID, enum AVPixelFormat ffPixFmt,
						   AVPicture *ffPicture, OSType qtPixFmt, UInt8 *baseAddr, long rowBytes, int width, int height )
{
	switch (ffPixFmt) {
		case AV_PIX_FMT_YUVJ420P:
		case AV_PIX_FMT_YUV420P:
			funcs->clear = ClearY422;
			
			//can't set this without the first real frame
			if (ffPicture) {
				if( (((size_t)baseAddr) % 16) || (rowBytes % 16) ){
					funcs->convert = Y420toY422_x86_scalar;
					Codecprintf( stderr,
								"ColorConversionFindFor(): bitmap destination address=%p and/or rowBytes=%ld not aligned to 16 bits;"
								" using scalar Y420toY422 conversion for safety",
								baseAddr, rowBytes );
				}
				else if (ffPicture->linesize[0] & 15){
					funcs->convert = Y420toY422_x86_scalar;
				}
				else{
					funcs->convert = Y420toY422_sse2;
					Codecprintf( stderr, "ColorConversionFindFor(): using SSE accelerated Y420 to Y422 conversion" );
				}
			}
			break;
		case AV_PIX_FMT_BGR24:
			funcs->clear = ClearRGB24;
			funcs->convert = BGR24toRGB24;
			break;
		case AV_PIX_FMT_ARGB:
			funcs->clear = ClearRGB32;
			funcs->convert = RGB32toRGB32Copy;
			break;
			// RJVB
		case AV_PIX_FMT_RGBA:
			funcs->clear = ClearRGB32;
			funcs->convert = RGB32toARGB;
			break;
		case AV_PIX_FMT_BGRA:
			funcs->clear = ClearRGB32;
			funcs->convert = RGB32toRGB32Swap;
			break;
		case AV_PIX_FMT_RGB24:
			funcs->clear = ClearRGB24;
			funcs->convert = RGB24toRGB24;
			break;
		case AV_PIX_FMT_YUV422P:
			funcs->clear = ClearY422;
			funcs->convert = Y422toY422;
			break;
		case AV_PIX_FMT_YUVA420P:
			funcs->clear = ClearV408;
			funcs->convert = YA420toV408;
			break;
		case AV_PIX_FMT_YUV420P10LE:
			funcs->clear = ClearY422;
			
			if (ffPicture) {
				if( (((size_t)baseAddr) % 16) || (rowBytes % 16) ){
					funcs->convert = Y420_10toY422_8;
					Codecprintf( stderr,
								"ColorConversionFindFor(): bitmap destination address=%p and/or rowBytes=%ld not aligned to 16 bits;"
								" using scalar Y420_10toY422_8 conversion for safety",
								baseAddr, rowBytes );
				}
				else if (ffPicture->linesize[0] & 15){
					funcs->convert = Y420_10toY422_8;
				}
				else{
					funcs->convert = Y420_10toY422_8_sse2;
					Codecprintf( stderr, "ColorConversionFindFor(): using SSE accelerated Y420_10toY422_8 conversion" );
				}
			}
			break;
		default:
			return paramErr;
	}
	
	return noErr;
}
