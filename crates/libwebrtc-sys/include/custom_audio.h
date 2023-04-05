#pragma once

#include "api/audio/audio_mixer.h"
#include "api/audio/audio_frame.h"
#include "api/media_stream_interface.h"
#include "common_audio/resampler/include/push_resampler.h"
#include "rtc_base/synchronization/mutex.h"

class CustomAudioSource : public webrtc::AudioMixer::Source, rtc::RefCountedObject<webrtc::AudioSourceInterface> {
  public:

  CustomAudioSource() {}

  webrtc::AudioMixer::Source::AudioFrameInfo GetAudioFrameWithInfo(
      int sample_rate_hz,
      webrtc::AudioFrame* audio_frame) override;

    // A way for a mixer implementation to distinguish participants.
  int Ssrc() const override {return -1;};

  // A way for this source to say that GetAudioFrameWithInfo called
  // with this sample rate or higher will not cause quality loss.
  int PreferredSampleRate() const override {return sample_rate;};



  // Sets the volume of the source. `volume` is in  the range of [0, 10].
  // TODO(tommi): This method should be on the track and ideally volume should
  // be applied in the track in a way that does not affect clones of the track.
  void SetVolume(double volume) {}

  // Registers/unregisters observers to the audio source.
  void RegisterAudioObserver(AudioObserver* observer) {}
  void UnregisterAudioObserver(AudioObserver* observer) {}

  // TODO(tommi): Make pure virtual.
  void AddSink(webrtc::AudioTrackSinkInterface* sink) {}
  void RemoveSink(webrtc::AudioTrackSinkInterface* sink) {}

  // Returns options for the AudioSource.
  // (for some of the settings this approach is broken, e.g. setting
  // audio network adaptation on the source is the wrong layer of abstraction).
  const cricket::AudioOptions options() const {return cricket::AudioOptions();};


  webrtc::MediaSourceInterface::SourceState state() const override {return _state;};
  bool remote() const {return false;};


  void RegisterObserver(webrtc::ObserverInterface* observer) {};
  void UnregisterObserver(webrtc::ObserverInterface* observer) {};

  webrtc::MediaSourceInterface::SourceState _state = webrtc::MediaSourceInterface::SourceState::kInitializing;
  webrtc::AudioFrame frame;
  int sample_rate = 48000;
  webrtc::PushResampler<int16_t> render_resampler_;
  webrtc::Mutex mutex_;
};