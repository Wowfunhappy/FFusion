#!/bin/bash -x
# this script (C) RJVB is meant to build Perian's FFmpeg version for osx86-shared
# it is supposed to be run on the CLI after building FFmpeg for OS X (i.e. building Perian with XCode)

if [ "$1" = "-check" ] ;then
	TEST="-n"
	shift
else
	TEST=""
fi 

if [ "$1" = "-remake" ] ;then
	NOCONFIGURE="yes"
	shift
else
	NOCONFIGURE=""
fi

x86tune="generic"
x86flags=""

if [ -e /opt/local/bin/gcc-mp-4.7 ]; then
	CC="gcc-mp-4.7"
	x86tune="core2"
	x86flags="--param max-completely-peel-times=2"
elif [ -e /usr/bin/gcc-4.2 ]; then
	CC="gcc-4.2"
	x86tune="core2"
	x86flags="--param max-completely-peel-times=2"
fi

if [ "$MACOSX_DEPLOYMENT_TARGET" = "" ]; then
	MACOSX_DEPLOYMENT_TARGET="10.6"
fi

configureflags="--cc=$CC --disable-amd3dnow --disable-doc --disable-encoders \
     --enable-ffprobe --disable-ffserver --enable-muxers --disable-network --enable-libopenjpeg \
	--extra-cflags=-I/opt/local/include --extra-ldflags=-L/opt/local/lib \
     --enable-swscale --enable-avfilter --disable-avdevice --enable-ffmpeg --disable-ffplay --target-os=darwin"

cflags="-mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET \
        -D__DARWIN_UNIX03=0 -Dattribute_deprecated= -w -msse3 -msse4 -mssse3"

if [ "$BUILD_STYLE" = "Development" ] ; then
    configureflags="$configureflags --disable-optimizations --disable-asm"
	buildid_ffmpeg="${buildid_ffmpeg}Dev"
else
    optcflags="-fweb -fstrict-aliasing -finline-limit=1000 -freorder-blocks -O3 -finline-functions -funroll-loops -fno-tree-vectorize -ftracer"
	buildid_ffmpeg="${buildid_ffmpeg}Dep"
fi

if [ "$SRCROOT" = "" ] ;then
	cd "`dirname $0`"
	SRCROOT="`pwd`"
fi
if [ "$BUILT_PRODUCTS_DIR" = "" ] ;then
	BUILT_PRODUCTS_DIR="${SRCROOT}/build"
fi
if [ "$SYMROOT" = "" ] ;then
	SYMROOT="${BUILT_PRODUCTS_DIR}/osx86.build"
fi
if [ "${DESTDIR}" = "" ] ;then
	DESTDIR="${SRCROOT}/FFmpeg/osx86"
fi

PATH="${PATH}:${DESTDIR}" ; export PATH

BUILD_ID_FILE="${DESTDIR}/buildid"

if [[ -e "$BUILD_ID_FILE" ]] ; then
    oldbuildid_ffmpeg=`cat "$BUILD_ID_FILE"`
else
    oldbuildid_ffmpeg="buildme"
fi

QUICKBUILD="$BUILT_PRODUCTS_DIR/Universal/quickbuild"
if [[ -e "$QUICKBUILD" ]] ; then
    oldbuildid_ffmpeg="quick"
    rm "$QUICKBUILD"
fi

   buildi386=1

mkdir -p "$BUILT_PRODUCTS_DIR"
mkdir -p "$SYMROOT/osx86"

BUILDDIR="$BUILT_PRODUCTS_DIR/osx86"
mkdir -p "$BUILDDIR"

do_exit () {
	if [ $1 != 0 ] ;then
		${CC} --version 2>&1 | line | fgrep '4.7.' > /dev/null
		if [ $? = 0 ] ;then
			echo "Hand-Compile dsputil_mmx.c using an earlier gcc version before calling '$0 -remake', using something like"
			echo "(cd ${BUILDDIR} ; CC -I. -I"/Volumes/Debian/Users/bertin/work/src/MacOSX/FFusion-124R/FFmpeg/source" -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DHAVE_AV_CONFIG_H -arch i386 -mmacosx-version-min=10.6 -D__DARWIN_UNIX03=0 -Dattribute_deprecated= -w -msse3 -msse4 -mssse3 -fweb -fstrict-aliasing -finline-limit=1000 -freorder-blocks -O3 -finline-functions -funroll-loops -fno-tree-vectorize -ftracer -mdynamic-no-pic -mtune=core2 --param max-completely-peel-times=2 -I/opt/local/include -march=core2 -std=c99 -mdynamic-no-pic -fomit-frame-pointer -g -Wdeclaration-after-statement -Wall -Wno-parentheses -Wno-switch -Wdisabled-optimization -Wpointer-arith -Wredundant-decls -Wno-pointer-sign -Wcast-qual -Wwrite-strings -Wundef -Wmissing-prototypes -O3 -fno-math-errno -Werror=implicit-function-declaration -Werror=missing-prototypes -MMD -MF libavcodec/x86/dsputil_mmx.d -MT libavcodec/x86/dsputil_mmx.o -c -o libavcodec/x86/dsputil_mmx.o /Volumes/Debian/Users/bertin/work/src/MacOSX/FFusion-124R/FFmpeg/source/libavcodec/x86/dsputil_mmx.c )"
		fi
	fi
	exit $1
}

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
    echo "Shared ffmpeg libs are up-to-date ; not rebuilding"
    else
    echo "Shared ffmpeg libs are out-of-date ; reconfiguring"
    
    echo -n "Configuring "

    # files we'd like to keep frame pointers in for in-the-wild debugging
    fptargets="libavformat/avformat.dll libavutil/avutil.dll libavcodec/utils.o"

	configureflags="--prefix=${DESTDIR} ${configureflags}"
		if [ "$BUILD_STYLE" != "Development" ] ; then
        	optcflags_i386="$optcflags -mdynamic-no-pic -mtune=$x86tune $x86flags" 
        fi

        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" -a "${NOCONFIGURE}" = "" ] ; then
            if [ "$arch" = ppc ] ; then
                "$SRCROOT/FFmpeg/source/configure" --enable-cross-compile --arch=i386 \
                --cpu=core2 --extra-ldflags="$cflags -arch i386" \
                --extra-cflags="-arch i386 $cflags $optcflags_i386" $configureflags || exit 1
            else
                "$SRCROOT/FFmpeg/source/configure" --extra-ldflags="$cflags -arch i386" \
                --cpu=core2 --extra-cflags="-arch i386 $cflags $optcflags_i386" \
                $configureflags || exit 1
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
	cd "${SRCROOT}"
        
       	fi
    
        cd "$BUILDDIR"
        fpcflags=`grep -m 1 CFLAGS= "$BUILDDIR"/config.mak | sed -e s/CFLAGS=// -e s/-fomit-frame-pointer//` 

make -j3 CFLAGS="$fpcflags" $fptargets ${TEST}
make -j3 ${TEST} || do_exit 1
make -wk install ${TEST}

	echo -n "$buildid_ffmpeg" > $BUILD_ID_FILE

FINAL_BUILD_ID_FILE="$SYMROOT/osx86/buildid"
if [[ -e "$FINAL_BUILD_ID_FILE" ]] ; then
    oldbuildid_ffmpeg=`cat "$FINAL_BUILD_ID_FILE"`
else
    oldbuildid_ffmpeg="buildme"
fi

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
    echo "Final static ffmpeg libs are up-to-date ; not copying"
else
	cd "${SRCROOT}"
	gcp -p "${BUILDDIR}/config.h" "${DESTDIR}/include/config.h"
	gcp -puv "${BUILDDIR}"/*/*.def "${DESTDIR}/lib"
	gcp -puv "${BUILDDIR}"/*/*.lib "${DESTDIR}/lib"
fi
