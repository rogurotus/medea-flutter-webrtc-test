#include "api/media_stream_interface.h"


class DesktopAudioSource : public rtc::RefCountedObject<webrtc::AudioSourceInterface> {
    public:
DesktopAudioSource();
  // TODO(deadbeef): Makes all the interfaces pure virtual after they're
  // implemented in chromium.

  // Sets the volume of the source. `volume` is in  the range of [0, 10].
  // TODO(tommi): This method should be on the track and ideally volume should
  // be applied in the track in a way that does not affect clones of the track.
  void SetVolume(double volume) override;

  // Registers/unregisters observers to the audio source.
  void RegisterAudioObserver(AudioObserver* observer) override;
  void UnregisterAudioObserver(AudioObserver* observer) override;

  // TODO(tommi): Make pure virtual.
  void AddSink(webrtc::AudioTrackSinkInterface* sink) override;
  void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override;

  void RegisterObserver(webrtc::ObserverInterface* observer) override;
void UnregisterObserver(webrtc::ObserverInterface* observer) override;

  webrtc::MediaSourceInterface::SourceState state() const override;

  bool remote() const override;


  // Returns options for the AudioSource.
  // (for some of the settings this approach is broken, e.g. setting
  // audio network adaptation on the source is the wrong layer of abstraction).
  const cricket::AudioOptions options() const override;
  private: 
  std::vector<webrtc::AudioTrackSinkInterface*> vec; 
};