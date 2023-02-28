package com.instrumentisto.medea_flutter_webrtc.model

data class MediaTrackSettings(val deviceId: String, val kind: Map<String, Any>) {
  /** Converts these [RtcStats] into a [Map] which can be returned to the Flutter side. */
  fun asFlutterResult(): Map<String, Any> {
    return mapOf("deviceId" to deviceId, "kind" to kind)
  }
}
