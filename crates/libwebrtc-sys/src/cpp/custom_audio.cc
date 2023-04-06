#include "custom_audio.h"
#include <iostream>

webrtc::AudioMixer::Source::AudioFrameInfo
AudioSource::GetAudioFrameWithInfo(int sample_rate_hz,
                                         webrtc::AudioFrame* audio_frame) {
  mutex_.Lock();
  auto* source = frame.data();
  if (frame.sample_rate_hz() != sample_rate_hz) {
    render_resampler_.InitializeIfNeeded(frame.sample_rate_hz(), sample_rate_hz,
                                         frame.num_channels_);
    render_resampler_.Resample(
        frame.data(), frame.samples_per_channel_ * frame.num_channels_,
        resample_buffer, webrtc::AudioFrame::kMaxDataSizeSamples);
    source = resample_buffer;
  }

  audio_frame->UpdateFrame(0, (const int16_t*)source, sample_rate_hz / 100,
                           sample_rate_hz,
                           webrtc::AudioFrame::SpeechType::kNormalSpeech,
                           webrtc::AudioFrame::VADActivity::kVadActive);

  return webrtc::AudioMixer::Source::AudioFrameInfo::kNormal;
};

void AudioSource::UpdateFrame(const int16_t* source,
                                    int size,
                                    int sample_rate) {
  frame.UpdateFrame(0, source, size, sample_rate,
                    webrtc::AudioFrame::SpeechType::kNormalSpeech,
                    webrtc::AudioFrame::VADActivity::kVadActive);
  mutex_.Unlock();
}

// A way for a mixer implementation to distinguish participants.
int AudioSource::Ssrc() const {
  return -1;
};

// A way for this source to say that GetAudioFrameWithInfo called
// with this sample rate or higher will not cause quality loss.
int AudioSource::PreferredSampleRate() const {
  return frame.sample_rate_hz();
};