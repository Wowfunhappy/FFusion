This is an updated version of the FFusion QuickTime Component. It's based on RJVB's updated version, which was based on the Perian project's updated version, which was based on the original by Jérôme Corne. I also copied some code from [MaddTheSane's attempt at updating Perian](https://github.com/MaddTheSane/perian/compare/updatedFFMpeg) in 2013.

FFusion makes it possible to play HEVC, VP9, and AV1 videos in QuickTime 10.2 or below on OS X 10.6 – 10.9. When combined with my tweaked copy of Perian component, the xiph-qt component, and the flip4mac components, it's possible to play virtually every mainstream video format. It has also been reported to mostly work in QuickTime 7 on versions of macOS as recent as 10.14.

For technical reasons (which in this instance is a euphemism for "I couldn't figure out what was wrong"), the master branch builds a component that decodes AV1 and VP9, and the "HEVC" branch builds a component that decodes HEVC. Build and install both components so QuickTime can play all three formats.

A word of warning: code quality (by which I mean my code, not that of the original projects) is terrible. I am not a C developer, _especially_ as of when I first started working on this in 2021. I did whatever I could to make this thing work by sheer force of will.

My dev environment is Xcode 5.0.2 (!) running on OS X 10.9.

To build ffmpeg for the HEVC branch:
> ./configure cc=/opt/local/bin/clang-mp-18 --disable-doc --disable-programs --disable-network --disable-avfilter --disable-avdevice --disable-swscale --disable-iconv --disable-everything --enable-decoder=hevc --enable-parser=hevc --target-os=darwin --disable-debug --enable-fast-unaligned --disable-safe-bitstream-reader --extra-ldflags="-arch i386 -L/opt/local/lib" --extra-cflags="-I/opt/local/include -mmacosx-version-min=10.6 -Dattribute_deprecated= -fvisibility=hidden -w -arch i386 -Ofast -fomit-frame-pointer -fno-stack-protector"

Building ffmpeg for the AV1/VP9 (master) branch is more complicated:
1. Install clang-18 and pkgconfig from MacPorts.
2. Modify the dav1d portfile:
  configure.args-append \
                    -Denable_tests=false --default-library=both
  macosx_deployment_target	10.6
  configure.optflags		-Ofast
3. Install the portfile 32bit: `sudo port install build_arch=i386`
4. ./configure cc=/opt/local/bin/clang-mp-18 --disable-doc --disable-programs --disable-network --disable-avfilter --disable-avdevice --disable-swscale --disable-iconv --disable-everything --enable-decoders --enable-parsers --enable-libdav1d --target-os=darwin --disable-debug --enable-fast-unaligned --disable-safe-bitstream-reader --extra-ldflags="-arch i386 -L/opt/local/lib" --extra-cflags="-I/opt/local/include -mmacosx-version-min=10.6 -Dattribute_deprecated= -fvisibility=hidden -w -arch i386 -Ofast -fomit-frame-pointer -fno-stack-protector" --extra-libs="-ldav1d"

You'll notice the ffmpeg is configured `--enable-decoders` `--enable-parsers` even though we only use VP9 and AV1. When I enable only vp9 and av1, the component crashes during the ffusion preflight function. I concluded life is too short to spend time debugging this.
