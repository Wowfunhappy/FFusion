/*
 *  version_check.c
 *  FFusion
 *
 *  Created by René J.V. Bertin on 20130125.
 *  Copyright 2013 Perian. All rights reserved.
 *
 */


#include <stdio.h>
#include <stddef.h>
#include <math.h>
#ifdef _MSC_VER
#	define inline	__inline
#	include <intrin.h>
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavcore/avcore.h>

#include "timing.c"

#ifdef _WIN32_WINNT
static const size_t w32nt = _WIN32_WINNT;
#endif

#ifdef _MSC_VER
#	define DLLIMPORT	__declspec(dllimport)
#else
#	define DLLIMPORT	/**/
#endif

#if defined(i386) || defined(__i386__)
#	define REG_b	"ebx"
#	define REG_S	"esi"
#	define cpuid(index,eax,ebx,ecx,edx)\
    __asm__ volatile\
        ("mov %%"REG_b", %%"REG_S"\n\t"\
         "cpuid\n\t"\
         "xchg %%"REG_b", %%"REG_S\
         : "=a" (eax), "=S" (ebx),\
           "=c" (ecx), "=d" (edx)\
         : "0" (index));

#	define cpuid2(func,ax,bx,cx,dx)\
	__asm__ __volatile__ ("cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
#endif

#ifdef __GNUC__
#	define unlikely(x) __builtin_expect(x, 0)
#	define likely(x) __builtin_expect(x, 1)
#else
#	define unlikely(x)	(x)
#	define likely(x)	(x)
#endif

//Handles the last row for Y420 videos with an odd number of luma rows
//FIXME: odd number of luma columns is not handled and they will be lost
#if 1 && ((__GNUC__ >= 4 && __GNUC_MINOR__ >= 7) || __GNUC__ > 4)
static void Y420toY422_lastrow(uint8_t *O, uint8_t *YC, uint8_t *UC, uint8_t *VC, int halfWidth)
{
	uint8_t *o = __builtin_assume_aligned(O,16),  *yc = __builtin_assume_aligned(YC,16),
		*uc = __builtin_assume_aligned(UC,16), *vc = __builtin_assume_aligned(VC,16);
#else
static void Y420toY422_lastrow(uint8_t *o, uint8_t *yc, uint8_t *uc, uint8_t *vc, int halfWidth)
{
#endif
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

#include <emmintrin.h>

#ifdef _MSCVER
#	define FASTCALL	__fastcall
#elif defined(__i386__) && !defined(__llvm__) && !defined(_MSC_VER)
#	define FASTCALL	__attribute__((fastcall))
#else
#	define FASTCALL
#endif

static FASTCALL void Y420toY422_sse2(AVFrame *picture, uint8_t *O, int outRB, int width, int height, unsigned long *N)
{
#if 1 && ((__GNUC__ >= 4 && __GNUC_MINOR__ >= 7) || __GNUC__ > 4)
	uint8_t	*yc = __builtin_assume_aligned(picture->data[0],16), *uc = __builtin_assume_aligned(picture->data[1], 16),
		*vc = __builtin_assume_aligned(picture->data[2],16), *o = __builtin_assume_aligned(O, 16);
#else
	uint8_t	*yc = picture->data[0], *uc = picture->data[1], *vc = picture->data[2], *o = O;
#endif
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		y, x, halfwidth = width >> 1, halfheight = height >> 1;
	int		vWidth = width >> 5;

	for (y = 0; y < halfheight; y++) {
		uint8_t   *o2 = o + outRB,   *yc2 = yc + rY;
		__m128i *ov = (__m128i*)o, *ov2 = (__m128i*)o2, *yv = (__m128i*)yc, *yv2 = (__m128i*)yc2;
		__m128i *uv = (__m128i*)uc,*vv  = (__m128i*)vc;
		
#if defined(__i386__) && !defined(_MSC_VER) //FIXMERJVB
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
#else
		for (x = 0; x < vWidth; x++) {
			int x2 = x<<1, x4 = x<<2;

			__m128i	tmp_y = yv[x2], tmp_y3 = yv[x2+1],
					tmp_y2 = yv2[x2], tmp_y4 = yv2[x2+1],
					tmp_u = _mm_loadu_si128(&uv[x]), tmp_v = _mm_loadu_si128(&vv[x]),
					chroma_l = _mm_unpacklo_epi8(tmp_u, tmp_v),
					chroma_h = _mm_unpackhi_epi8(tmp_u, tmp_v);
			
			_mm_stream_si128(&ov[x4],   _mm_unpacklo_epi8(chroma_l, tmp_y)); 
			_mm_stream_si128(&ov[x4+1], _mm_unpackhi_epi8(chroma_l, tmp_y)); 
			_mm_stream_si128(&ov[x4+2], _mm_unpacklo_epi8(chroma_h, tmp_y3)); 
			_mm_stream_si128(&ov[x4+3], _mm_unpackhi_epi8(chroma_h, tmp_y3)); 
			
			_mm_stream_si128(&ov2[x4],  _mm_unpacklo_epi8(chroma_l, tmp_y2)); 
			_mm_stream_si128(&ov2[x4+1],_mm_unpackhi_epi8(chroma_l, tmp_y2));
			_mm_stream_si128(&ov2[x4+2],_mm_unpacklo_epi8(chroma_h, tmp_y4));
			_mm_stream_si128(&ov2[x4+3],_mm_unpackhi_epi8(chroma_h, tmp_y4));
		}
#endif

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
	*N += 1;
}

static FASTCALL void Y420toY422_x86_scalar(AVFrame *picture, uint8_t *O, int outRB, int width, int height, unsigned long *N)
{
#if 1 && ((__GNUC__ >= 4 && __GNUC_MINOR__ >= 7) || __GNUC__ > 4)
	uint8_t	*yc = __builtin_assume_aligned(picture->data[0],16), *u = __builtin_assume_aligned(picture->data[1], 16),
		*v = __builtin_assume_aligned(picture->data[2],16), *o = __builtin_assume_aligned(O, 16);
#else
	uint8_t	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2], *o = O;
#endif
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		halfheight = height >> 1, halfwidth = width >> 1;
	int		y, x;
	
	for (y = 0; y < halfheight; y ++) {
		uint8_t *o2 = o + outRB, *yc2 = yc + rY;
		
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
	*N += 1;
}

int main( int argc, char *argv[] )
{ const char *license, *configuration;
  unsigned version;
  int cpuFlags;
	fprintf( stderr, "avcodec,avcore,avutil version functions at %p,%p,%p\n",
		avcodec_version, avcore_version, avutil_version );

	version = avcodec_version();
	license = avcodec_license();
	configuration = avcodec_configuration();
	fprintf( stdout, "libavcodec, version %u / \"%s\"\n", version, configuration );
	fprintf( stderr, "            license \"%s\"\n", license );
	fflush(stderr), fflush(stdout);

	version = avcore_version();
	license = avcore_license();
	configuration = avcore_configuration();
	fprintf( stdout, "libavcore, version %u / \"%s\"\n", version, configuration );
	fprintf( stderr, "           license \"%s\"\n", license );
	fflush(stderr), fflush(stdout);

	version = avutil_version();
	license = avutil_license();
	configuration = avutil_configuration();
	fprintf( stdout, "libavutil, version %u / \"%s\"\n", version, configuration );
	fprintf( stderr, "           license \"%s\"\n", license );
	fflush(stderr), fflush(stdout);

	cpuFlags = av_get_cpu_flags();
	fprintf( stdout, "Extensions supported by the current CPU:\n" );

	if( cpuFlags & AV_CPU_FLAG_FORCE ){
		fputs( "Force usage of selected flags\n", stdout );
	}

	if( cpuFlags & AV_CPU_FLAG_MMX ){
		fputs( "MMX\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_MMX2 ){
		fputs( "MMX2\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_3DNOW ){
		fputs( "3DNOW\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE ){
		fputs( "SSE\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE2 ){
		fputs( "SSE2\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE2SLOW ){
		fputs( "SSE2SLOW\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_3DNOWEXT ){
		fputs( "3DNOWEXT\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE3 ){
		fputs( "SSE3\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE3SLOW ){
		fputs( "SSE3SLOW\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSSE3 ){
		fputs( "SSSE3\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE4 ){
		fputs( "SSE4\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_SSE42 ){
		fputs( "SSE42\n", stdout );
	}
	if( cpuFlags & AV_CPU_FLAG_IWMMXT ){
		fputs( "IWMMXT\n", stdout );
	}
// "standard" => always set even if not on a G4 or G5 cpu?!
// 	if( cpuFlags & AV_CPU_FLAG_ALTIVEC ){
// 		fputs( "ALTIVEC\n", stdout );
// 	}
#if defined(i386) || defined(_MSC_VER)
	{ int val[4];
#	ifdef _MSC_VER
	  union { int i[4]; char c[16]; } vendor;
		__cpuid( vendor.i, 0 );
		vendor.c[16] = '\0';
		fprintf( stderr, "CPU vendor: 0x%x,%s ", vendor.i[0], &vendor.c[4] );
		__cpuid( val, 1 );
		fprintf( stderr, "cpuid(1) -> {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );
		
		__cpuid( val, 0x80000000 );
		if( val[0] >= 0x80000001 ){
			__cpuid( val, 0x80000001 );
			fprintf( stderr, "cpuid(0x80000001) -> {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );
		}
		else{
			fprintf( stderr, "cpuid(0x80000000) -> no AMD 3DNow/MMX {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );
		}
#	else
	  union { int i[3]; char c[12]; } vendor;
		cpuid( 0, val[0], vendor.i[0], vendor.i[1], vendor.i[2] );
		vendor.c[11] = '\0';
		fprintf( stderr, "CPU vendor: 0x%x,%s ", val[0], vendor.c );
		cpuid( 1, val[0], val[1], val[2], val[3] );
		fprintf( stderr, "cpuid(1) -> {0x%x,0x%x,0x%x,0x%x} ", val[0], val[1], val[2], val[3] );
		cpuid2( 0, val[0], vendor.i[0], vendor.i[1], vendor.i[2] );
		vendor.c[11] = '\0';
		fprintf( stderr, "/\n\t0x%x,%s ", val[0], vendor.c );
		cpuid2( 1, val[0], val[1], val[2], val[3] );
		fprintf( stderr, "cpuid(1) -> {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );

		cpuid(0x80000000, val[0], val[1], val[2], val[3]);
		if( val[0] >= 0x80000001 ){
			cpuid(0x80000001, val[0], val[1], val[2], val[3]);
			fprintf( stderr, "cpuid(0x80000001) -> {0x%x,0x%x,0x%x,0x%x} ", val[0], val[1], val[2], val[3] );
			cpuid2(0x80000001, val[0], val[1], val[2], val[3]);
			fprintf( stderr, "/ {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );
		}
		else{
			fprintf( stderr, "cpuid(0x80000000) -> no AMD 3DNow/MMX {0x%x,0x%x,0x%x,0x%x}\n", val[0], val[1], val[2], val[3] );
		}
#	endif
	}
#endif
	{ AVFrame pict;
	  uint8_t *baseAddr = NULL;
	  int width = 720, height = 576, outRB = 1440;
	  double t;
	  unsigned long N = 0;
		memset( &pict.data, 0, sizeof(pict.data) );
		pict.linesize[0] = 752, pict.linesize[1] = 376;
		init_HRTime();
		if( (pict.data[0] = (uint8_t*) malloc( width * height * 2 * sizeof(uint8_t) ))
		   && (pict.data[1] = (uint8_t*) malloc( width * height * 2 * sizeof(uint8_t) ))
		   && (pict.data[2] = (uint8_t*) malloc( width * height * 2 * sizeof(uint8_t) ))
		   && (baseAddr = (uint8_t*) malloc( width * height * 4 * 2 * sizeof(uint8_t) ))
		){
		  double startT = HRTime_Time();
			do{
				Y420toY422_sse2( &pict, baseAddr, outRB, width, height, &N );
			} while( (t = HRTime_Time() - startT) < 5 );
			fprintf( stderr, "%lu Y420toY422_sse2(outRB=%d,width=%d,height=%d) conversions in %gs; %gHz\n",
					N, outRB, width, height, t,
					N / t );
			startT = HRTime_Time(); N = 0;
			do{
				Y420toY422_x86_scalar( &pict, baseAddr, outRB, width, height, &N );
				N += 1;
			} while( (t = HRTime_Time() - startT) < 5 );
			fprintf( stderr, "%lu Y420toY422_x86_scalar(outRB=%d,width=%d,height=%d) conversions in %gs; %gHz\n",
					N, outRB, width, height, t,
					N / t );
		}
		if( pict.data[0] ){
			free(pict.data[0]);
		}
		if( pict.data[1] ){
			free(pict.data[1]);
		}
		if( pict.data[2] ){
			free(pict.data[2]);
		}
		if( baseAddr ){
			free(baseAddr);
		}
	}
	exit(0);
}
