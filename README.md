This is an updated version of the FFusion QuickTime Component. It's based on RJVB's updated version, which was based on the Perian project's updated version, which was based on the original by Jérôme Corne. I also copied some code from [MaddTheSane's attempt at updating Perian](https://github.com/MaddTheSane/perian/compare/updatedFFMpeg) in 2013.

FFusion makes it possible to play HEVC, VP9, and AV1 videos in QuickTime 10.2 or below on OS X 10.6 – 10.9. When combined with my tweaked copy of Perian component, the xiph-qt component, and the flip4mac components, it's possible to play virtually every mainstream video format.

For technical reasons (slash I couldn't figure out what was wrong), the master branch builds a component that decodes AV1 and VP9, and the "HEVC" branch builds a component that decodes HEVC. Build and install both components so QuickTime can play all three formats.

My dev environment for FFusion itself is Xcode 5.0.2 (!) running on OS X 10.9. The static libraries for FFmpeg 4.4.6, which are included in the repository, were compiled on Snow Leopard.

To build ffmpeg for the HEVC branch:
> ./configure cc=/opt/local/bin/clang-mp-9.0 --disable-doc --disable-programs --disable-network --disable-avfilter --disable-avdevice --enable-swscale --disable-iconv --disable-everything --enable-decoder=hevc --enable-parser=hevc --target-os=darwin --extra-ldflags="-arch i386 -L/opt/local/lib" --extra-cflags="-I/opt/local/include -mmacosx-version-min=10.6 -Dattribute_deprecated= -fvisibility=hidden -w -arch i386"

And to build it for the master branch (VP9 and AV1):
  - Install dav1d from MacPorts with the i386 architecture. As of July 2025, this port does not install the static library by default, so you need to edit the portfile to add `--default-library=both` to configure.args. Then:
> ./configure cc=/opt/local/bin/clang-mp-9.0 --disable-doc --disable-programs --disable-network --disable-avfilter --disable-avdevice --disable-swscale --disable-iconv --disable-everything --enable-decoders --enable-parsers --enable-libvpx --enable-gpl --target-os=darwin --extra-ldflags="-arch i386 -L/opt/local/lib" --extra-cflags="-I/opt/local/include -mmacosx-version-min=10.6 -Dattribute_deprecated= -fvisibility=hidden -w -arch i386"
