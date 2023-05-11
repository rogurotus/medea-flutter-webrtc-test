#ifndef MEDIA_STREAM_TRACK_INTERFACE_H
#define MEDIA_STREAM_TRACK_INTERFACE_H

#include "bridge.h"
#include "rust/cxx.h"
#include <optional>
#include "rtc_base/ref_counted_object.h"

namespace bridge {
struct DynAudioSinkCallback;

// Returns the `kind` of the provided `MediaStreamTrackInterface`.
std::unique_ptr<std::string> media_stream_track_kind(
    const MediaStreamTrackInterface& track);

// Returns the `id` of the provided `MediaStreamTrackInterface`.
std::unique_ptr<std::string> media_stream_track_id(
    const MediaStreamTrackInterface& track);

// Returns the `state` of the provided `MediaStreamTrackInterface`.
TrackState media_stream_track_state(const MediaStreamTrackInterface& track);

// Returns the `enabled` property of the provided `MediaStreamTrackInterface`.
bool media_stream_track_enabled(const MediaStreamTrackInterface& track);

// Downcasts the provided `MediaStreamTrackInterface` to a
// `VideoTrackInterface`.
std::unique_ptr<VideoTrackInterface>
media_stream_track_interface_downcast_video_track(
    std::unique_ptr<MediaStreamTrackInterface> track);

// Downcasts the provided `MediaStreamTrackInterface` to an
// `AudioTrackInterface`.
std::unique_ptr<AudioTrackInterface>
media_stream_track_interface_downcast_audio_track(
    std::unique_ptr<MediaStreamTrackInterface> track);

// Adds `AudioTrackSinkInterface` to an `AudioTrackInterface`.
void add_audio_track_sink(
    AudioTrackInterface& track, const AudioTrackSinkInterface& sink);

// Removes `AudioTrackSinkInterface` from a `AudioTrackInterface`.
void remove_audio_track_sink(
    AudioTrackInterface& track, const AudioTrackSinkInterface& sink);

// Creates a new `AudioTrackSinkInterface` for the given `DynAudioSinkCallback`.
std::unique_ptr<AudioTrackSinkInterface> create_audio_sink(rust::Box<bridge::DynAudioSinkCallback> cb);

// `AudioSink` propagating `On_data` to the Rust side.
class AudioSink : public rtc::RefCountedObject<RefCountedAudioSink> {
    public:
    // Creates a new `AudioSink`.
    AudioSink(rust::Box<bridge::DynAudioSinkCallback> cb);
    // Destruct a `AudioSink`.
    ~AudioSink();
    // Called when an audio data is received.
    void OnData(const void* audio_data,
                      int bits_per_sample,
                      int sample_rate,
                      size_t number_of_channels,
                      size_t number_of_frames) override;
    private:
    // Rust side callback.
    rust::Box<bridge::DynAudioSinkCallback> audio_level_cb;
};

}  // namespace bridge

#endif