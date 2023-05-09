#include "media_stream_track_interface.h"
#include "libwebrtc-sys/src/bridge.rs.h"

namespace bridge {

// Returns the `kind` of the provided `MediaStreamTrackInterface`.
std::unique_ptr<std::string> media_stream_track_kind(
    const MediaStreamTrackInterface& track) {
  return std::make_unique<std::string>(track->kind());
}

// Returns the `id` of the provided `MediaStreamTrackInterface`.
std::unique_ptr<std::string> media_stream_track_id(
    const MediaStreamTrackInterface& track) {
  return std::make_unique<std::string>(track->id());
}

// Returns the `state` of the provided `MediaStreamTrackInterface`.
TrackState media_stream_track_state(const MediaStreamTrackInterface& track) {
  return track->state();
}

// Returns the `enabled` property of the provided `MediaStreamTrackInterface`.
bool media_stream_track_enabled(const MediaStreamTrackInterface& track) {
  return track->enabled();
}

// Downcasts the provided `MediaStreamTrackInterface` to a
// `VideoTrackInterface`.
std::unique_ptr<VideoTrackInterface>
media_stream_track_interface_downcast_video_track(
    std::unique_ptr<MediaStreamTrackInterface> track) {
  return std::make_unique<VideoTrackInterface>(
      static_cast<webrtc::VideoTrackInterface*>(track.release()->release()));
}

// Downcasts the provided `MediaStreamTrackInterface` to an
// `AudioTrackInterface`.
std::unique_ptr<AudioTrackInterface>
media_stream_track_interface_downcast_audio_track(
    std::unique_ptr<MediaStreamTrackInterface> track) {
  return std::make_unique<AudioTrackInterface>(
      static_cast<webrtc::AudioTrackInterface*>(track.release()->release()));
}

// Adds `AudioTrackSinkInterface` to an `AudioTrackInterface`.
void add_audio_track_sink(AudioTrackInterface& track,
                          const AudioTrackSinkInterface& sink) {
  track->AddSink(sink.get());
}

// Removes `AudioTrackSinkInterface` from a `AudioTrackInterface`.
void remove_audio_track_sink(AudioTrackInterface& track,
                             const AudioTrackSinkInterface& sink) {
  track->RemoveSink(sink.get());
}

// Creates a new `AudioTrackSinkInterface` for the given `DynAudioSinkCallback`.
std::unique_ptr<AudioTrackSinkInterface> create_audio_sink(
    rust::Box<bridge::DynAudioSinkCallback> cb) {
  rtc::scoped_refptr<AudioSink> sink(new AudioSink(std::move(cb)));
  return std::make_unique<AudioTrackSinkInterface>(sink);
}

// Creates a new `AudioSink`.
AudioSink::AudioSink(rust::Box<bridge::DynAudioSinkCallback> cb)
    : audio_level_cb(std::move(cb)) {}

// Destruct a `AudioSink`.
AudioSink::~AudioSink() {}

// Called when an audio data is received.
void AudioSink::OnData(const void* audio_data,
                       int bits_per_sample,
                       int sample_rate,
                       size_t number_of_channels,
                       size_t number_of_frames) {
  int16_t* audio_data_cast = (int16_t*)audio_data;
  rust::Vec<int16_t> data;
  for (int i = 0; i < number_of_frames * number_of_channels; ++i) {
    data.push_back(audio_data_cast[i]);
  }

  bridge::audio_sink_on_data(*audio_level_cb, std::move(data), bits_per_sample,
                             sample_rate, number_of_channels, number_of_frames);
}

}  // namespace bridge
