This is an updated version of the FFusion QuickTime Component, based on RJVB's updated version, based on the Perian project's updated version, based on the original by Jérôme Corne. It also makes heavy use of code copied from [MaddTheSane's attempt at the same thing](https://github.com/MaddTheSane/perian/compare/updatedFFMpeg) (although he was more ambitious than I was, trying to update all of Perian instead of just FFusion).

FFusion aims to be a decode-only QuickTime component, which leverages FFmpeg to do the actual work. Older versions allowed QuickTime to play DIVX, H.263, and a variety of other formats. However, because this version is based off of FFmpeg 2.8.17, it can also play 4K streams downloaded from Youtube.

If you have youtube-dl installed, run:
> youtube-dl **VIDEO-URL-HERE** -f bestvideo[vcodec!^=av01]+bestaudio[ext=m4a]/best --add-metadata --merge-output-format mp4

If your video is available in 4K, the downloaded copy should be playable in QuickTime 10.2 or below, provided you've installed this component. It needs to be copied to Library/QuickTime.

Snow Leopard, Lion, and Mountain Lion ship with QuickTime 10.0, 10.1, and 10.2 respectively. On Mavericks, you'll need to downgrade your copy of QuickTime to [this modified version](https://github.com/Wowfunhappy/QuickTime-Fixer/releases/tag/2021.01.19). (Newer versions of macOS are out of luck. Sorry!)

Notably absent from this project is HEVC support. It is supported by our version of FFmpeg, but avcodec_decode_video2 always returns AVERROR_INVALIDDATA. Please get in touch if you have any ideas!

---

The files for FFmpeg 2.8.7 are included in the repository, but to use a different version, you'll need to re-compile the static libraries. I was only able to compile these libraries via the script included in Perian, and I was only able to to do so with libvpx enabled from within Snow Leopard (I used a VM).

I installed libvpx from MacPorts with the +universal flag, and edited createstaticlibs.sh in the Perian source code to use the following compilation options:

`configureflags="--cc=$CC --disable-amd3dnow --disable-doc --disable-encoders --enable-ffprobe --disable-ffserver --enable-muxers --disable-network --enable-libvpx --extra-cflags=-I/opt/local/include --extra-ldflags=-L/opt/local/lib --enable-swscale --enable-avfilter --disable-avdevice --enable-ffmpeg --disable-ffplay --disable-iconv --target-os=darwin"`

I'm aware this is terribly convoluted. It would probably be trivial to fix, too, but it wasn't where I wanted to invest my effort. If anyone actually wants to compile this, and can't get it working, _please_ feel free to open an issue!
