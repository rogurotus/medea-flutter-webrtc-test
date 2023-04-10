// #pragma once


// #include <iostream>

// #include <chrono>
// #include <thread>
// #include "adm.h"
// #include "api/make_ref_counted.h"
// #include "common_audio/wav_file.h"
// #include "modules/audio_device/include/test_audio_device.h"
// #include "modules/audio_device/linux/latebindingsymboltable_linux.h"
// #include "rtc_base/checks.h"
// #include "rtc_base/logging.h"
// #include "rtc_base/platform_thread.h"

// class WavFileReader final : public webrtc::TestAudioDeviceModule::Capturer,
//                             public webrtc::AudioMixer::Source {
//  public:
//   WavFileReader(absl::string_view filename,
//                 int sampling_frequency_in_hz,
//                 int num_channels,
//                 bool repeat);

//   webrtc::AudioFrame frame;
//   rtc::BufferT<int16_t> bufffer;
//   webrtc::Mutex mutex_;
//   webrtc::PushResampler<int16_t> render_resampler_;

//   webrtc::AudioMixer::Source::AudioFrameInfo GetAudioFrameWithInfo(
//       int sample_rate_hz,
//       webrtc::AudioFrame* audio_frame);

//   // A way for a mixer implementation to distinguish participants.
//   int Ssrc() const override;

//   // A way for this source to say that GetAudioFrameWithInfo called
//   // with this sample rate or higher will not cause quality loss.
//   int PreferredSampleRate() const override;

//   int SamplingFrequency() const override;

//   int NumChannels() const override;

//   bool Capture(rtc::BufferT<int16_t>* buffer) override;

//  private:
//   WavFileReader(std::unique_ptr<webrtc::WavReader> wav_reader,
//                 int sampling_frequency_in_hz,
//                 int num_channels,
//                 bool repeat);

//   const int sampling_frequency_in_hz_;
//   const int num_channels_;
//   std::unique_ptr<webrtc::WavReader> wav_reader_;
//   const bool repeat_;
// };