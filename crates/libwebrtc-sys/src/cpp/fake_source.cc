#include "fake_source.h"

  WavFileReader::WavFileReader(absl::string_view filename,
                int sampling_frequency_in_hz,
                int num_channels,
                bool repeat)
      : WavFileReader(std::make_unique<webrtc::WavReader>(filename),
                      sampling_frequency_in_hz,
                      num_channels,
                      repeat) {}

  webrtc::AudioMixer::Source::AudioFrameInfo WavFileReader::GetAudioFrameWithInfo(
      int sample_rate_hz,
      webrtc::AudioFrame* audio_frame) {
    mutex_.Lock();
    auto* b = new int16_t[(sample_rate_hz * frame.num_channels_) / 100];
    if (frame.sample_rate_hz() != sample_rate_hz) {
      render_resampler_.InitializeIfNeeded(frame.sample_rate_hz(),
                                           sample_rate_hz, frame.num_channels_);
      render_resampler_.Resample(
          frame.data(), frame.samples_per_channel_ * frame.num_channels_, b,
          webrtc::AudioFrame::kMaxDataSizeSamples);
    }
    audio_frame->UpdateFrame(
        0, (const int16_t*)b, sample_rate_hz / 100, sample_rate_hz,
        webrtc::AudioFrame::SpeechType::kNormalSpeech,
        webrtc::AudioFrame::VADActivity::kVadActive, frame.num_channels_);
    return webrtc::AudioMixer::Source::AudioFrameInfo::kNormal;
  };

  // A way for a mixer implementation to distinguish participants.
  int WavFileReader::Ssrc() const { return -1; };

  // A way for this source to say that GetAudioFrameWithInfo called
  // with this sample rate or higher will not cause quality loss.
  int WavFileReader::PreferredSampleRate() const { return 44100; };

  int WavFileReader::SamplingFrequency() const { return sampling_frequency_in_hz_; }

  int WavFileReader::NumChannels() const { return num_channels_; }

  bool WavFileReader::Capture(rtc::BufferT<int16_t>* buffer) {
    buffer->SetData(
        webrtc::TestAudioDeviceModule::SamplesPerFrame(
            sampling_frequency_in_hz_) *
            num_channels_,
        [&](rtc::ArrayView<int16_t> data) {
          size_t read = wav_reader_->ReadSamples(data.size(), data.data());
          if (read < data.size() && repeat_) {
            do {
              wav_reader_->Reset();
              size_t delta = wav_reader_->ReadSamples(
                  data.size() - read, data.subview(read).data());
              RTC_CHECK_GT(delta, 0) << "No new data read from file";
              read += delta;
            } while (read < data.size());
          }
          return read;
        });
    frame.UpdateFrame(0, (const int16_t*)buffer->data(), 441, 44100,
                      webrtc::AudioFrame::SpeechType::kNormalSpeech,
                      webrtc::AudioFrame::VADActivity::kVadActive, 1);
    mutex_.Unlock();
    return buffer->size() > 0;
  }

  WavFileReader::WavFileReader(std::unique_ptr<webrtc::WavReader> wav_reader,
                int sampling_frequency_in_hz,
                int num_channels,
                bool repeat)
      : sampling_frequency_in_hz_(sampling_frequency_in_hz),
        num_channels_(num_channels),
        wav_reader_(std::move(wav_reader)),
        repeat_(repeat) {
    RTC_CHECK_EQ(wav_reader_->sample_rate(), sampling_frequency_in_hz);
    RTC_CHECK_EQ(wav_reader_->num_channels(), num_channels);
  }