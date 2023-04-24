#include "custom_audio.h"
#include <iostream>

// Overwrites `audio_frame`. The data_ field is overwritten with
// 10 ms of new audio (either 1 or 2 interleaved channels) at
// `sample_rate_hz`. All fields in `audio_frame` must be updated.
webrtc::AudioMixer::Source::AudioFrameInfo AudioSource::GetAudioFrameWithInfo(
    int sample_rate_hz,
    webrtc::AudioFrame* audio_frame) {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [&]() { return frame_available_.load() || mute_.load(); });
  if (frame_available_.load()) {
    auto* source = frame_.data();
    if (frame_.sample_rate_hz() != sample_rate_hz) {
      render_resampler_.InitializeIfNeeded(
          frame_.sample_rate_hz(), sample_rate_hz, frame_.num_channels_);

      render_resampler_.Resample(
          frame_.data(), frame_.samples_per_channel_ * frame_.num_channels_,
          resample_buffer, webrtc::AudioFrame::kMaxDataSizeSamples);
      source = resample_buffer;
    }

    audio_frame->UpdateFrame(0, (const int16_t*)source, sample_rate_hz / 100,
                             sample_rate_hz,
                             webrtc::AudioFrame::SpeechType::kNormalSpeech,
                             webrtc::AudioFrame::VADActivity::kVadActive);

    frame_available_.store(false);
    mute_.store(false);
    pre_mute_.store(false);
    return webrtc::AudioMixer::Source::AudioFrameInfo::kNormal;
  } else {
    return webrtc::AudioMixer::Source::AudioFrameInfo::kMuted;
  }
};

// Updates the audio frame data.
void AudioSource::UpdateFrame(const int16_t* source,
                              int size,
                              int sample_rate,
                              int channels) {
  frame_.UpdateFrame(0, source, size, sample_rate,
                     webrtc::AudioFrame::SpeechType::kNormalSpeech,
                     webrtc::AudioFrame::VADActivity::kVadActive, channels);
  mute_.store(false);
  frame_available_.store(true);
  cv_.notify_all();
}

// A way for a mixer implementation to distinguish participants.
int AudioSource::Ssrc() const {
  return -1;
};

// A way for this source to say that GetAudioFrameWithInfo called
// with this sample rate or higher will not cause quality loss.
int AudioSource::PreferredSampleRate() const {
  return frame_.sample_rate_hz();
};

// Mutes the source until the next frame.
void AudioSource::Mute() {
  if (pre_mute_.load()) {
    if ((std::chrono::system_clock::now() - mute_clock_) >=
        std::chrono::milliseconds(10)) {
      mute_.store(true);
      cv_.notify_all();
    }
  } else {
    pre_mute_.store(true);
    mute_clock_ = std::chrono::system_clock::now();
  }
}