This is an updated version of the FFusion QuickTime Component. It's based on RJVB's updated version, which was based on the Perian project's updated version, which was based on the original by Jérôme Corne. I also copied some code from [MaddTheSane's attempt at the same thing](https://github.com/MaddTheSane/perian/compare/updatedFFMpeg) (although he was more ambitious than I was, trying to update all of Perian instead of just FFusion).

FFusion aims to be a decode-only QuickTime component which leverages FFmpeg to do the actual work. Like older versions, this one allows QuickTime to play DIVX, H.263, and a variety of other video formats. Unlike older versions, this one can also play HEVC videos and VP9 streams, because it's based off FFmpeg 3.4.8.

For example, this component allows QuickTime to play 4K video streams downlaoded from Youtube. If you have youtube-dl installed, run:
> youtube-dl **VIDEO-URL-HERE** -f '((bestvideo[height>1080][vcodec!^=av01]/bestvideo[vcodec^=avc1])+bestaudio[ext=m4a])/best' --add-metadata --merge-output-format mp4

...to get a video which will play in Quicktime 10.2 and below when this component is installed.

Snow Leopard, Lion, and Mountain Lion ship with QuickTime Player 10.0, 10.1, and 10.2 respectively. Mavericks comes with QuickTime 10.3, but can be downgraded to [this modified version](https://github.com/Wowfunhappy/QuickTime-Fixer/releases) which I created. Ugly flat versions of macOS are out of luck, sorry!

---

The static libraries for FFmpeg 3.8.7 are included in the repository; to use a different FFmpeg version, you'll need to recompile the libraries. For various reasons, I've been compiling FFmpeg in a Snow Leopard VM. Install libvpx from MacPorts with the +universal flag, and then configure ffmpeg with the following options:

`./configure cc=/opt/local/bin/clang-mp-9.0 --disable-doc --disable-programs --disable-network --disable-avfilter --disable-avdevice --disable-swscale --disable-iconv --disable-everything --enable-decoders --enable-parsers --enable-libvpx --enable-gpl --target-os=darwin --extra-ldflags="-arch i386 -L/opt/local/lib" --extra-cflags="-I/opt/local/include -mmacosx-version-min=10.6 -Dattribute_deprecated= -fvisibility=hidden -w -arch i386"`

My dev environment for FFusion itself was Xcode 5.0.2 running on OS X 10.9.
