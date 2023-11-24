package com.instrumentisto.medea_flutter_webrtc.model

/**
 * Representation of [org.webrtc.RtpCapabilities.HeaderExtensionCapability].
 *
 * @property uri `uri` of these [HeaderExtensionCapability].
 * @property preferredId `preferredId` of these [HeaderExtensionCapability].
 * @property preferredEncrypted `preferredEncrypted` of these [HeaderExtensionCapability].
 */
data class HeaderExtensionCapability(
    val uri: String,
    val preferredId: Int,
    val preferredEncrypted: Boolean,
) {
  companion object {
    /**
     * Converts the provided [org.webrtc.RtpCapabilities.HeaderExtensionCapability] into [RtcStats].
     *
     * @return [HeaderExtensionCapability] created based on the provided [org.webrtc.RtpCapabilities.HeaderExtensionCapability]].
     */
    fun fromWebRtc(
        header: org.webrtc.RtpCapabilities.HeaderExtensionCapability
    ): HeaderExtensionCapability {
      return HeaderExtensionCapability(header.uri, header.preferredId, header.preferredEncrypted)
    }
  }

  /** Converts these [HeaderExtensionCapability] into a [Map] which can be returned to the Flutter side. */
  fun asFlutterResult(): Map<String, Any> {
    return mapOf(
        "uri" to uri, "preferredId" to preferredId, "preferredEncrypted" to preferredEncrypted)
  }
}

/**
 * Representation of [org.webrtc.RtpCapabilities.CodecCapability].
 *
 * @property preferredPayloadType `preferredPayloadType` of these [CodecCapability].
 * @property name `name` of these [CodecCapability].
 * @property kind `kind` of these [CodecCapability].
 * @property clockRate `clockRate` of these [CodecCapability].
 * @property numChannels `numChannels` of these [CodecCapability].
 * @property parameters `parameters` of these [CodecCapability].
 * @property mimeType `mimeType` of these [CodecCapability].
 */
data class CodecCapability(
    val preferredPayloadType: Int,
    val name: String,
    val kind: MediaType,
    val clockRate: Int,
    val numChannels: Int?,
    val parameters: Map<String, String>,
    val mimeType: String,
) {
  companion object {
    /**
     * Converts the provided [org.webrtc.RtpCapabilities.CodecCapability] into [CodecCapability].
     *
     * @return [CodecCapability] created based on the provided [org.webrtc.RtpCapabilities.CodecCapability].
     */
    fun fromWebRtc(codec: org.webrtc.RtpCapabilities.CodecCapability): CodecCapability {
      var parameters = codec.parameters
      return CodecCapability(
          codec.preferredPayloadType,
          codec.name,
          MediaType.fromInt(codec.kind.ordinal),
          codec.clockRate,
          codec.numChannels,
          codec.parameters,
          codec.mimeType)
    }
  }

  /** Converts these [CodecCapability] into a [Map] which can be returned to the Flutter side. */
  fun asFlutterResult(): Map<String, Any?> {
    return mapOf(
        "preferredPayloadType" to preferredPayloadType,
        "name" to name,
        "kind" to kind.value,
        "clockRate" to clockRate,
        "numChannels" to numChannels as Any?,
        "parameters" to parameters,
        "mimeType" to mimeType)
  }
}

/**
 * Representation of [org.webrtc.RtpCapabilities].
 *
 * @property codecs `codecs` of these [RtpCapabilities].
 * @property headerExtensions `headerExtensions` of these [RtpCapabilities].
 */
data class RtpCapabilities(
    val codecs: List<CodecCapability>,
    val headerExtensions: List<HeaderExtensionCapability>,
) {
  companion object {
    /**
     * Converts the provided [org.webrtc.RtpCapabilities] into [RtpCapabilities].
     *
     * @return [RtpCapabilities] created based on the provided [org.webrtc.RtpCapabilities].
     */
    fun fromWebRtc(capability: org.webrtc.RtpCapabilities): RtpCapabilities {
      val codecs = capability.codecs.map { CodecCapability.fromWebRtc(it) }
      val header = capability.headerExtensions.map { HeaderExtensionCapability.fromWebRtc(it) }
      return RtpCapabilities(codecs, header)
    }
  }

  /** Converts these [RtpCapabilities] into a [Map] which can be returned to the Flutter side. */
  fun asFlutterResult(): Map<String, Any> {
    return mapOf(
        "codecs" to codecs.map { it.asFlutterResult() },
        "headerExtensions" to headerExtensions.map { it.asFlutterResult() })
  }
}
