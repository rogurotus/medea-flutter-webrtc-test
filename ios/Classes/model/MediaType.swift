import WebRTC

/// Representation of an `RTCRtpMediaType`.
enum MediaType: Int {
  /// Audio media.
  case audio

  /// Video media.
  case video

  static func fromWebRtc(direction: RTCRtpMediaType)
    -> MediaType
  {
    switch direction {
    case .RTCRtpMediaTypeAudio:
      return MediaType.audio
    case .RTCRtpMediaTypeVideo:
      return MediaType.video
    }
  }


  /// Converts this `MediaType` into an `RTCRTPMediaType`.
  func intoWebRtc() -> RTCRtpMediaType {
    switch self {
    case .audio:
      return RTCRtpMediaType.audio
    case .video:
      return RTCRtpMediaType.video
    }
  }
}
