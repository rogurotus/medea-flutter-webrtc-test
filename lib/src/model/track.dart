
/// Representation of a `MediaStreamTrack` readiness.
enum MediaStreamTrackState {
  /// Indicates that an input is connected and does its best-effort in the
  /// providing real-time data.
  live,

  /// Indicates that the input is not giving any more data and will never
  /// provide a new data.
  ended,
}

/// Kind of a media.
enum MediaKind {
  /// Audio data.
  audio,

  /// Video data.
  video
}

/// Directions in which a camera may produce a video.
enum FacingMode {
  /// Indicates that video source is facing toward the user (this includes, for
  /// example, the front-facing camera on a smartphone).
  user,

  /// Indicates that video source is facing away from the user, thereby viewing
  /// their environment (the back camera on a smartphone).
  environment,
}

/// Representation of a `MediaStreamTrack` settings.
abstract class MediaTrackSettings {
  MediaTrackSettings();
}

/// Representation of a `MediaStreamTrack` audio settings.
class AudioMediaTrackSettings extends MediaTrackSettings {
  AudioMediaTrackSettings();

  static AudioMediaTrackSettings fromFFI() {
    return AudioMediaTrackSettings();
  }

  static AudioMediaTrackSettings fromMap(dynamic settings) {
    return AudioMediaTrackSettings();
  }
}

/// Representation of a `MediaStreamTrack` video settings.
class VideoMediaTrackSettings extends MediaTrackSettings {
  FacingMode? facingMode;

  VideoMediaTrackSettings(this.facingMode);

  static VideoMediaTrackSettings fromFFI() {
    return VideoMediaTrackSettings(null);
  }

  static VideoMediaTrackSettings fromMap(dynamic settings) {
    return VideoMediaTrackSettings(
      FacingMode.values[settings['kind']['facingMode']],
    );
  }
}
