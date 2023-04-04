#include "custom_audio.h"
#include <iostream>

webrtc::AudioMixer::Source::AudioFrameInfo
CustomAudioSource::GetAudioFrameWithInfo(int sample_rate_hz,
                                         webrtc::AudioFrame* audio_frame) {

mutex_.Lock();
auto* b = new int16_t[sample_rate_hz/100];
  if (frame.sample_rate_hz() != sample_rate_hz) {
    render_resampler_.InitializeIfNeeded(frame.sample_rate_hz(), sample_rate_hz, frame.num_channels_);
    render_resampler_.Resample(
        frame.data(), frame.samples_per_channel_ * frame.num_channels_,
        b, webrtc::AudioFrame::kMaxDataSizeSamples);
  }
  audio_frame->UpdateFrame(0, (const int16_t*)b,
                           sample_rate_hz / 100, sample_rate_hz,
                           webrtc::AudioFrame::SpeechType::kNormalSpeech,
                           webrtc::AudioFrame::VADActivity::kVadActive);
  return webrtc::AudioMixer::Source::AudioFrameInfo::kNormal;
};
