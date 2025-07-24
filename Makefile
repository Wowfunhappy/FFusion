# FFusion Makefile - Bypass Xcode linking issues
# Uses clang-mp-18 with LTO for maximum performance

# Compiler and tools
CC = /opt/local/bin/clang-mp-18
CXX = /opt/local/bin/clang++-mp-18
REZ = /Applications/Xcode.app/Contents/Developer/Tools/Rez

# Paths
SRCDIR = .
FFMPEG_LIB_DIR = FFmpeg/osx86/lib
FFMPEG_INC_DIR = FFmpeg/osx86/include
BUILD_DIR = build
COMPONENT_DIR = $(BUILD_DIR)/FFusion.component
MACOS_DIR = $(COMPONENT_DIR)/Contents/MacOS
RESOURCES_DIR = $(COMPONENT_DIR)/Contents/Resources

# Target
TARGET = $(MACOS_DIR)/FFusion
PLIST = $(COMPONENT_DIR)/Contents/Info.plist
REZ_FILE = $(RESOURCES_DIR)/FFusionMacOSXResources.rsrc

# Source files (excluding version_check.c which is for separate target)
SOURCES = FFusionCodec.c ff_private.c Codecprintf.c CommonUtils.c \
          ColorConversions.c FFmpegUtils.c FrameBuffer.c codecID.c \
          bitstream_info.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# FFmpeg libraries (LTO bitcode)
FFMPEG_LIBS = $(FFMPEG_LIB_DIR)/libavcodec.a \
              $(FFMPEG_LIB_DIR)/libavformat.a \
              $(FFMPEG_LIB_DIR)/libavutil.a \
              $(FFMPEG_LIB_DIR)/libswresample.a \
              $(FFMPEG_LIB_DIR)/libdav1d.a

# Compiler flags
ARCH = -arch i386
DEPLOYMENT_TARGET = -mmacosx-version-min=10.6
ISYSROOT = -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk

# Optimization flags - Ofast for maximum performance
OPT_FLAGS = -Ofast

# Include paths
INCLUDES = -I$(FFMPEG_INC_DIR) -I$(FFMPEG_INC_DIR)/libavcodec \
           -I$(FFMPEG_INC_DIR)/libavformat -I$(FFMPEG_INC_DIR)/libavutil \
           -I$(FFMPEG_INC_DIR)/libswresample -I$(SRCDIR) \
           -IFFmpeg/source

# Preprocessor definitions
DEFINES = -DFFUSION_CODEC_ONLY -DHAVE_AV_CONFIG_H

# Complete compile flags
CFLAGS = $(ARCH) $(DEPLOYMENT_TARGET) $(ISYSROOT) $(OPT_FLAGS) \
         $(INCLUDES) $(DEFINES) -std=gnu99 -fno-common

# Linker flags - no LTO at link time to avoid symbol issues
LDFLAGS = $(ARCH) $(DEPLOYMENT_TARGET) $(ISYSROOT) -Ofast \
          -bundle -undefined dynamic_lookup \
          -dead_strip -no_dead_strip_inits_and_terms \
          -read_only_relocs suppress

# System frameworks and libraries
FRAMEWORKS = -framework AudioToolbox -framework CoreVideo \
             -framework VideoDecodeAcceleration -framework QuickTime \
             -framework Foundation -framework CoreFoundation \
             -framework Cocoa -framework SystemConfiguration

# FFmpeg libraries (regular object files)
FFMPEG_LIBS = $(FFMPEG_LIB_DIR)/libavcodec.a \
              $(FFMPEG_LIB_DIR)/libavformat.a \
              $(FFMPEG_LIB_DIR)/libavutil.a \
              $(FFMPEG_LIB_DIR)/libswresample.a \
              $(FFMPEG_LIB_DIR)/libdav1d.a

# Build rules
.PHONY: all clean component install

all: component

component: $(TARGET) $(PLIST) $(REZ_FILE)
	@echo "FFusion.component built successfully with LTO!"

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(COMPONENT_DIR):
	mkdir -p $(MACOS_DIR) $(RESOURCES_DIR)

# Compile source files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $< with clang-mp-18..."
	$(CC) $(CFLAGS) -c $< -o $@

# Link the main target
$(TARGET): $(OBJECTS) $(FFMPEG_LIBS) | $(COMPONENT_DIR)
	@echo "Linking FFusion component..."
	$(CC) $(LDFLAGS) $(OBJECTS) \
		$(FFMPEG_LIBS) \
		$(FRAMEWORKS) \
		-o $@

# Copy Info.plist
$(PLIST): Plists/FFusion-Info.plist | $(COMPONENT_DIR)
	cp Plists/FFusion-Info.plist $@

# Compile resource file
$(REZ_FILE): Resources/FFusionMacOSXResources.r | $(COMPONENT_DIR)
	$(REZ) -o $@ -useDF -d i386_YES=1 -i $(SRCDIR) Resources/FFusionMacOSXResources.r

# Install to QuickTime components directory
install: component
	@echo "Installing FFusion.component..."
	sudo cp -R $(COMPONENT_DIR) /Library/QuickTime/
	@echo "Installation complete. You may need to restart applications."

# Copy phase (like Xcode's copy files phase)
copy: component
	cp -R $(COMPONENT_DIR) ~/Library/QuickTime/

# Xcode's original copy files phase destination
xcode-copy: component
	cp -R $(COMPONENT_DIR) /Volumes/Jonathan/Library/QuickTime/

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Debug info
debug:
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "FFmpeg libs: $(FFMPEG_LIBS)"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"