#pragma once

#include "api/audio/audio_mixer.h"
#include "api/audio/audio_frame.h"
#include "common_audio/resampler/include/push_resampler.h"
#include "rtc_base/synchronization/mutex.h"
#include <condition_variable>
#include <mutex>

class RefCountedAudioSource : public webrtc::AudioMixer::Source, public rtc::RefCountInterface {};

class AudioSource : public rtc::RefCountedObject<RefCountedAudioSource> {
  public:

  AudioSource() {}

  webrtc::AudioMixer::Source::AudioFrameInfo GetAudioFrameWithInfo(
      int sample_rate_hz,
      webrtc::AudioFrame* audio_frame) override;

    // A way for a mixer implementation to distinguish participants.
  int Ssrc() const override;

  // A way for this source to say that GetAudioFrameWithInfo called
  // with this sample rate or higher will not cause quality loss.
  int PreferredSampleRate() const;
  void UpdateFrame(const int16_t* source, int size, int sample_rate);

  private:
  webrtc::AudioFrame frame_;
  webrtc::PushResampler<int16_t> render_resampler_;
  int16_t resample_buffer[webrtc::AudioFrame::kMaxDataSizeSamples];

  std::mutex mutex_;
  std::condition_variable cv_;
  bool frame_available_ = false;
};