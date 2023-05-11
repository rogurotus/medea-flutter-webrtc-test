// This is a slightly tweaked version of
// https://github.com/shiguredo/momo/blob/b81b51da8e2b823090d6a7f966fc517e047237e6/src/rtc/screen_video_capturer.h
//
// Copyright 2015-2021, tnoho (Original Author)
// Copyright 2018-2021, Shiguredo Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SCREEN_VIDEO_CAPTURE_H
#define SCREEN_VIDEO_CAPTURE_H


#include "media/base/adapted_video_track_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/platform_thread.h"

// `VideoTrackSourceInterface` capturing frames from a user's display.
class ScreenVideoCapturer : public rtc::AdaptedVideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                            public webrtc::DesktopCapturer::Callback {
 public:

  // Fills the provided `SourceList` with all the available screens that can be
  // used by this `ScreenVideoCapturer`.
  static bool GetSourceList(webrtc::DesktopCapturer::SourceList* sources);

  // Creates a new `ScreenVideoCapturer` with the specified constraints.
  ScreenVideoCapturer(webrtc::DesktopCapturer::SourceId source_id,
                      size_t max_width,
                      size_t max_height,
                      size_t target_fps);
  ~ScreenVideoCapturer();

 private:
  // Captures a `webrtc::DesktopFrame`.
  bool CaptureProcess();

  // Callback for `webrtc::DesktopCapturer::CaptureFrame`.
  //
  // Converts a `DesktopFrame` to a `VideoFrame` that is forwarded to
  // `ScreenVideoCapturer::OnFrame`.
  void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                       std::unique_ptr<webrtc::DesktopFrame> frame) override;

  // `VideoSinkInterface` implementation.
  void OnFrame(const webrtc::VideoFrame& frame) override;

  // Indicates that parameters suitable for screencast should be automatically
  // applied to `RtpSender`s.
  bool is_screencast() const override;

  // Indicates whether the encoder should denoise video before encoding it.
  //
  // If it's not set, the default configuration is used which differs depending
  // on a video codec.
  absl::optional<bool> needs_denoising() const override;

  // Returns state of this `ScreenVideoCapturer`.
  webrtc::MediaSourceInterface::SourceState state() const override;

  // Always returns `false` since `ScreenVideoCapturer` is used to capture local
  // display surface.
  bool remote() const override;

  // Max width of the captured `VideoFrame`.
  size_t max_width_;

  // Max height of the captured `VideoFrame`.
  size_t max_height_;

  // Target frame capturing interval.
  int requested_frame_duration_;

  // Width of the captured `DesktopFrame`.
  size_t capture_width_;

  // Height of the captured `DesktopFrame`.
  size_t capture_height_;

  // Size of the previous captured `DesktopFrame`.
  webrtc::DesktopSize previous_frame_size_;

  // Last captured `DesktopFrame`.
  std::unique_ptr<webrtc::DesktopFrame> output_frame_;

  // `PlatformThread` performing the actual frames capturing.
  rtc::PlatformThread capture_thread_;

  // `webrtc::DesktopCapturer` used to capture frames.
  std::unique_ptr<webrtc::DesktopCapturer> capturer_;

  // Flag signaling the `capture_thread_` to stop.
  std::atomic<bool> quit_;
};

#endif