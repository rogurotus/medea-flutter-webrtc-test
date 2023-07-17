#if __OBJC__
#pragma once

#include <Cocoa/Cocoa.h>
#include <IOSurface/IOSurface.h>
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>

enum speaker_layout {
  SPEAKERS_UNKNOWN,     /**< Unknown setting, fallback is stereo. */
  SPEAKERS_MONO,        /**< Channels: MONO */
  SPEAKERS_STEREO,      /**< Channels: FL, FR */
  SPEAKERS_2POINT1,     /**< Channels: FL, FR, LFE */
  SPEAKERS_4POINT0,     /**< Channels: FL, FR, FC, RC */
  SPEAKERS_4POINT1,     /**< Channels: FL, FR, FC, LFE, RC */
  SPEAKERS_5POINT1,     /**< Channels: FL, FR, FC, LFE, RL, RR */
  SPEAKERS_7POINT1 = 8, /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
};

struct obs_audio_info {
  uint32_t samples_per_sec;
  enum speaker_layout speakers;
};

typedef enum {
  ScreenCaptureDisplayStream = 0,
  ScreenCaptureWindowStream = 1,
  ScreenCaptureApplicationStream = 2,
} ScreenCaptureStreamType;

typedef enum {
  ScreenCaptureAudioDesktopStream = 0,
  ScreenCaptureAudioApplicationStream = 1,
} ScreenCaptureAudioStreamType;

@interface ScreenCaptureDelegate : NSObject <SCStreamOutput, SCStreamDelegate>

@property struct screen_capture *sc;

@end

struct screen_capture {
  NSRect frame;
  bool hide_cursor;
  bool hide_obs;
  bool show_hidden_windows;
  bool show_empty_names;
  bool audio_only;

  SCStream *disp;
  SCStreamConfiguration *stream_properties;
  SCShareableContent *shareable_content;
  ScreenCaptureDelegate *capture_delegate;

//  os_event_t *disp_finished;
//  os_event_t *stream_start_completed;
//  os_sem_t *shareable_content_available;
  IOSurfaceRef current, prev;
  bool capture_failed;

  pthread_mutex_t mutex;

  ScreenCaptureStreamType capture_type;
  ScreenCaptureAudioStreamType audio_capture_type;
  CGDirectDisplayID display;
  CGWindowID window;
  NSString *application_id;
};

#endif