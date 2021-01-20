This is an updated version of the FFusion QuickTime Component. It's based on RJVB's updated version, which was based on the Perian project's updated version, which was based on the original by Jérôme Corne. I also copied some code from [MaddTheSane's attempt at the same thing](https://github.com/MaddTheSane/perian/compare/updatedFFMpeg) (although he was more ambitious than I was, trying to update all of Perian instead of just FFusion).

FFusion aims to be a decode-only QuickTime component which leverages FFmpeg to do the actual work. Like older versions, this one allows QuickTime to play DIVX, H.263, and a variety of other video formats. Unlike older versions, this one can also play 4K VP9 streams downloded from Youtube, because it's based off of FFmpeg 2.8.17.

If you have youtube-dl installed, run:
> youtube-dl **VIDEO-URL-HERE** -f bestvideo[vcodec!^=av01]+bestaudio[ext=m4a]/best --add-metadata --merge-output-format mp4
...with a video that's available in 4K. With this component installed in Library/QuickTime/, QuickTime Player 10.0–10.2 should be able to play the downloaded file.

Snow Leopard, Lion, and Mountain Lion ship with QuickTime Player 10.0, 10.1, and 10.2 respectively. Mavericks comes with QuickTime 10.3, but can be downgraded to [this modified version](https://github.com/Wowfunhappy/QuickTime-Fixer/releases/tag/2021.01.19) which I created. Ugly flat versions of macOS are out of luck, sorry!

Notably absent from this project is HEVC support. Please see http://ffmpeg.org/pipermail/libav-user/2021-January/012644.html, and get in touch if you have any ideas!

---

The static libraries for FFmpeg 2.8.7 are included in the repository; to use a different FFmpeg version, you'll need to recompile the libraries. I was only able to do via the script included in Perian, and I was only able to to do so with libvpx enabled from within a Snow Leopard VM.

Install libvpx from MacPorts with the +universal flag, clone the Perian repo, copy the source code for your desired FFmpeg version into Perian's FFmpeg folder, and edit Perian's createstaticlibs.sh script to use the following FFmpeg compilaion options:

`configureflags="--cc=$CC --disable-amd3dnow --disable-doc --disable-encoders --enable-ffprobe --disable-ffserver --enable-muxers --disable-network --enable-libvpx --extra-cflags=-I/opt/local/include --extra-ldflags=-L/opt/local/lib --enable-swscale --enable-avfilter --disable-avdevice --enable-ffmpeg --disable-ffplay --disable-iconv --target-os=darwin"`

Then, build Perian. It will fail, but it should build the libraries you need. Those libraries can then be copied to a newer (not new) machine, so you don't have to compile FFusion itself on Snow Leopard. By the way, my dev environment for FFusion itself was OS X 10.9 and Xcode 5.0.2.

I'm well aware that this is terribly convoluted. It may be trivial to fix (or may not, I suppose), but it wasn't where I wanted to invest my effort. If anyone actually wants to compile this, and can't get it working, _please_ feel free to open an issue!
