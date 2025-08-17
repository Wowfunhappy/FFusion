I asked Claude to look into what was going on with the two different branches (one for HEVC and one for VP9/AV1), mostly because I was curious. Here is what it said:

---

# FFusion Codec Branch Analysis

## Overview
FFusion has two separate branches for modern codec support:
- **Master branch**: Supports VP9 and AV1
- **HEVC branch**: Supports HEVC/H.265

This document explains why these codecs cannot currently coexist in a single component and what would be needed to unify them.

## The Core Problem

### HEVC in Master Branch
When HEVC is enabled in the master branch, it exhibits a "2 frames forward, 1 frame back" playback pattern. This is a classic symptom of B-frame reordering issues.

### VP9/AV1 in HEVC Branch
When VP9 or AV1 are enabled in the HEVC branch, they display only black screens, indicating the frame data is invalid.

## The Frame Request Pattern (Why HEVC Breaks)

### How QuickTime Requests Frames

QuickTime doesn't always request frames in simple sequential order. Especially with HEVC's complex B-frame structure, the request pattern might look like:

1. Request frame 0 (I-frame) → Decode and display
2. Request frame 1 (P-frame) → Decode and display
3. Request frame 2 (B-frame) → Decode and display
4. Request frame 1 again (needed as reference) → **This is where things break!**

### HEVC Branch Behavior
```
QuickTime: "Give me frame #1"
HEVC Branch: "Do you have frame #1 cached? If yes, use it. If no, decode it."
Result: Frame #1 is found in cache and reused correctly ✓
```

### Master Branch Behavior
```
QuickTime: "Give me frame #1"
Master Branch: "Frame already decoded? OK, moving on..." 
(but doesn't actually retrieve the cached frame)
Result: Shows wrong frame or re-decodes, causing the 2-forward-1-back pattern ✗
```

## The Critical Code Difference

### HEVC Branch - BeginBand Function
```c
if(myDrp->decoded)
{
    int i;
    myDrp->frameNumber = p->frameNumber;
    // ACTIVELY SEARCHES for the cached frame by number
    for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
        if (glob->buffers[i].retainCount && 
            glob->buffers[i].frameNumber == myDrp->frameNumber) {
            // Found it! Reuse this frame
            myDrp->buffer = retainBuffer(glob, &glob->buffers[i]);
            break;
        }
    }
    return noErr;
}
```

### Master Branch - BeginBand Function
```c
if(myDrp->decoded)
{
    myDrp->frameNumber = p->frameNumber;
    // Just sets the frame number and moves on
    // Comment says: "Frame caching is handled per-DecompressRecord in DecodeBand/DrawBand"
    // But it never actually retrieves the cached frame!
    return noErr;
}
```

## Why VP9/AV1 Show Black in HEVC Branch

### The AVPicture Memory Problem

The HEVC branch uses a deprecated structure called `AVPicture` alongside the modern `AVFrame`. Here's what happens:

#### Step 1: FFmpeg Decodes a Frame
```
FFmpeg: "Here's your decoded frame in my internal buffer at memory address 0x1234"
```

#### Step 2: HEVC Branch "Saves" the Frame
```c
// This is the fatal flaw - only copying POINTERS, not the actual pixel data!
memcpy(glob->buffers[i].picture.data, pic->data, sizeof(...));
// picture.data[0] now points to 0x1234 (FFmpeg's internal buffer)
```

#### Step 3: FFmpeg Recycles Its Buffer
```
FFmpeg: "I'm done with that frame, I'll reuse buffer 0x1234 for the next frame"
VP9/AV1 decoders are aggressive about this!
```

#### Step 4: QuickTime Tries to Display
```
QuickTime: "Show me that frame"
HEVC Branch: "Sure, it's at 0x1234"
But 0x1234 now contains different data or garbage!
Result: Black screen
```

### Why HEVC Survives This Problem

HEVC's decoder happens to keep its internal buffers valid longer:
- Less aggressive buffer recycling
- Different internal memory management
- The data at those pointer addresses remains valid "by luck"

### What Master Branch Does Right

```c
// Master branch properly copies/references the frame data
if (myDrp->buffer->frame) {
    av_frame_unref(myDrp->buffer->frame);
    av_frame_ref(myDrp->buffer->frame, &tempFrame);  
    // This properly references the frame, keeping it valid!
}
```

## The Complete Picture: Why Branches Can't Merge

### Attempt 1: Use HEVC Branch for Everything
- ✓ HEVC works (has frame caching it needs)
- ✗ VP9/AV1 show black (AVPicture pointer problem)

### Attempt 2: Use Master Branch for Everything  
- ✓ VP9/AV1 work (proper AVFrame handling)
- ✗ HEVC stutters (missing frame cache)

### Attempt 3: Add HEVC's Caching to Master?
Would need to:
1. Restore frame number lookup in BeginBand
2. Keep the AVFrame reference system
3. But this was removed for a reason (simplified VP9/AV1 path)

### Attempt 4: Fix AVPicture in HEVC Branch?
Would need to:
1. Replace AVPicture with AVFrame throughout
2. Update all color conversion functions
3. Essentially... recreate what master branch already did

## A Detailed Look at Frame Lifecycle

### In HEVC Branch (Working for HEVC)

1. **Frame Request Arrives**
   ```
   QuickTime → BeginBand: "I need frame #5"
   ```

2. **Check Cache**
   ```c
   for each buffer:
       if buffer.frameNumber == 5 && buffer.retainCount > 0:
           return this buffer  // Found it!
   ```

3. **If Not Cached, Decode**
   ```
   BeginBand → DecodeBand: "Decode frame #5"
   DecodeBand → FFmpeg: "Decode this data"
   FFmpeg → Buffer[i]: "Here's the decoded frame"
   ```

4. **Save with Frame Number**
   ```c
   buffer[i].frameNumber = 5;  // Tag it!
   buffer[i].retainCount = 1;  // Keep it alive!
   ```

5. **Later Request for Same Frame**
   ```
   QuickTime → BeginBand: "I need frame #5 again"
   BeginBand: "Found it in buffer[i]!" ✓
   ```

### In Master Branch (Broken for HEVC)

1. **Frame Request Arrives**
   ```
   QuickTime → BeginBand: "I need frame #5"
   ```

2. **Check if Decoded**
   ```c
   if (already_decoded) {
       // Just return, assuming someone else handles it
       return noErr;
   }
   ```

3. **Problem: Nobody Actually Gets the Frame!**
   ```
   DrawBand: "Where's my frame data?"
   System: "¯\_(ツ)_/¯"
   Result: Wrong frame displayed or re-decode attempt
   ```

## The Right Solution (Detailed Implementation)

### Unified Buffer Structure
```c
typedef struct {
    AVFrame *frame;           // Modern, refcounted frame
    int frameNumber;          // QuickTime's request number
    int64_t pts;             // Presentation timestamp
    int64_t dts;             // Decode timestamp
    int displayOrder;         // For B-frame reordering
    int retainCount;          // Our own reference count
    bool isKeyFrame;          // For seeking optimization
} FFusionBuffer;
```

### Codec Detection and Configuration
```c
// In Preflight
switch(codecID) {
    case AV_CODEC_ID_HEVC:
        // HEVC: Complex B-frame pyramids, needs caching
        glob->needsFrameReordering = true;
        glob->maxCachedFrames = 16;  // HEVC can reference many frames
        glob->cacheStrategy = CACHE_BY_FRAME_NUMBER;
        break;
        
    case AV_CODEC_ID_VP9:
        // VP9: Simpler reference structure
        glob->needsFrameReordering = false;
        glob->maxCachedFrames = 3;   // Only need a few refs
        glob->cacheStrategy = CACHE_BY_PTS;
        break;
        
    case AV_CODEC_ID_AV1:
        // AV1: Has its own reference management
        glob->needsFrameReordering = false;
        glob->maxCachedFrames = 8;   // Film grain synthesis needs refs
        glob->cacheStrategy = CACHE_BY_DISPLAY_ORDER;
        break;
}
```

### Smart Frame Lookup
```c
// In BeginBand
if (myDrp->decoded) {
    FFusionBuffer *cached = NULL;
    
    switch(glob->cacheStrategy) {
        case CACHE_BY_FRAME_NUMBER:
            // HEVC path: "Do you have frame #N?"
            for (int i = 0; i < FFUSION_MAX_BUFFERS; i++) {
                if (glob->buffers[i].frameNumber == p->frameNumber &&
                    glob->buffers[i].retainCount > 0) {
                    cached = &glob->buffers[i];
                    break;
                }
            }
            break;
            
        case CACHE_BY_PTS:
            // VP9 path: "Do you have the frame for this timestamp?"
            int64_t target_pts = FrameTimeToPTS(p->frameTime);
            cached = FindBufferByPTS(glob, target_pts);
            break;
            
        case CACHE_BY_DISPLAY_ORDER:
            // AV1 path: "Do you have display frame N?"
            cached = FindBufferByDisplayOrder(glob, p->displayOrder);
            break;
    }
    
    if (cached) {
        myDrp->buffer = retainBuffer(glob, cached);
        // Found cached frame - we're done!
        return noErr;
    } else {
        // Need to decode this frame
        myDrp->decoded = false;
    }
}
```

### Proper Frame Storage After Decode
```c
// In DecodeBand, after FFmpeg decodes
if (err == noErr) {
    FFusionBuffer *buf = GetAvailableBuffer(glob);
    
    // Properly reference the frame (not just copy pointers!)
    if (!buf->frame) {
        buf->frame = av_frame_alloc();
    }
    av_frame_unref(buf->frame);        // Clear old data
    av_frame_ref(buf->frame, tempFrame); // Reference new frame
    
    // Store metadata for caching
    buf->frameNumber = myDrp->frameNumber;
    buf->pts = tempFrame->pts;
    buf->dts = tempFrame->dts;
    buf->displayOrder = CalculateDisplayOrder(tempFrame);
    buf->isKeyFrame = (tempFrame->key_frame == 1);
    buf->retainCount = 1;
    
    // Make this buffer findable
    myDrp->buffer = buf;
    glob->lastDecodedFrameNum = myDrp->frameNumber;
}
```

## Why Neither Branch Got It Completely Right

### HEVC Branch
- ✓ Has frame caching logic
- ✓ Handles B-frame reordering
- ✗ Uses deprecated AVPicture
- ✗ Only copies pointers, not frame data
- ✗ Vulnerable to FFmpeg recycling buffers

### Master Branch  
- ✓ Uses modern AVFrame with proper references
- ✓ Memory management is correct
- ✗ Removed frame caching entirely
- ✗ Can't handle HEVC's complex reordering
- ✗ "Simplified" too much

Each branch has half the solution:
- HEVC branch: Right algorithm, wrong memory management
- Master branch: Right memory management, wrong algorithm

## Conclusion

The branches exist because of a fundamental conflict:
- HEVC needs complex frame caching for B-frame reordering
- VP9/AV1 need proper memory management with AVFrame
- The HEVC branch's AVPicture approach breaks modern codecs
- The master branch's simplified approach breaks HEVC

A proper solution would combine:
1. Master branch's AVFrame reference counting
2. HEVC branch's frame number caching
3. Codec-specific configuration flags
4. Smart cache lookup strategies

Until this is implemented, the two-branch approach is actually a pragmatic solution - each branch does what it needs to do for its specific codecs, even if it means building and installing two separate components.