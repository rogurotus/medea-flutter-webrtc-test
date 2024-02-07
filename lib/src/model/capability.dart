import 'package:medea_flutter_webrtc/medea_flutter_webrtc.dart';
import '/src/api/bridge.g.dart' as ffi;

/// RtpCapabilities is used to represent the static capabilities of an endpoint.
/// An application can use these capabilities to construct an RtpParameters.
class RtpCapabilities {
  RtpCapabilities(this.codecs, this.headerExtensions);

  static RtpCapabilities fromFFI(ffi.RtpCapabilities capabilities) {
    return RtpCapabilities(
        capabilities.codecs.map((v) => RtpCodecCapability.fromFFI(v)).toList(),
        capabilities.headerExtensions
            .map((v) => RtpHeaderExtensionCapability.fromFFI(v))
            .toList());
  }

  static RtpCapabilities fromMap(dynamic map) {
    var codecs = (map['codecs'] as List<Object?>)
        .where((element) => element != null)
        .map((c) => RtpCodecCapability.fromMap(c))
        .toList();

    var headerExtensions = (map['headerExtensions'] as List<Object?>)
        .where((element) => element != null)
        .map((h) => RtpHeaderExtensionCapability.fromMap(h))
        .toList();
    return RtpCapabilities(codecs, headerExtensions);
  }

  /// Supported codecs.
  List<RtpCodecCapability> codecs;

  /// Supported RTP header extensions.
  List<RtpHeaderExtensionCapability> headerExtensions;
}

/// Used in RtpCapabilities header extensions query and setup methods:
/// represents the capabilities/preferences of an implementation for a header extension.
class RtpHeaderExtensionCapability {
  RtpHeaderExtensionCapability(this.uri, this.direction);

  static RtpHeaderExtensionCapability fromFFI(
      ffi.RtpHeaderExtensionCapability headerExtension) {
    return RtpHeaderExtensionCapability(headerExtension.uri,
        TransceiverDirection.values[headerExtension.direction.index]);
  }

  static RtpHeaderExtensionCapability fromMap(dynamic map) {
    var direction = map['direction'] == null
        ? TransceiverDirection.sendRecv
        : TransceiverDirection.values[map['direction']];
    return RtpHeaderExtensionCapability(map['uri'], direction);
  }

  /// URI of this extension, as defined in RFC8285.
  String uri;

  /// The direction of the extension.
  TransceiverDirection direction;
}

/// RtpCodecCapability is to RtpCodecParameters as RtpCapabilities is to
/// RtpParameters. This represents the static capabilities of an endpoint's
/// implementation of a codec.
class RtpCodecCapability {
  RtpCodecCapability(this.mimeType, this.clockRate, this.parameters, this.kind,
      this.name, this.numChannels, this.preferredPayloadType);

  static RtpCodecCapability fromFFI(ffi.RtpCodecCapability capability) {
    Map<String, String> parameters = {};
    for (var element in capability.parameters) {
      parameters.addAll({element.$1: element.$2});
    }

    return RtpCodecCapability(
        capability.mimeType,
        capability.clockRate,
        parameters,
        MediaKind.values[capability.kind.index],
        capability.name,
        capability.numChannels,
        capability.preferredPayloadType);
  }

  static RtpCodecCapability fromMap(dynamic map) {
    Map<String, String> parameters = {};
    for (var e in (map['parameters'] as Map<Object?, Object?>).entries) {
      if (e.key != null) {
        parameters.addAll({e.key! as String: (e.value as String?) ?? ''});
      }
    }
    return RtpCodecCapability(
        map['mimeType'],
        map['clockRate'],
        parameters,
        MediaKind.values[map['kind']],
        map['name'],
        map['numChannels'],
        map['preferredPayloadType']);
  }

  Map<String, dynamic> toMap() {
    return {
      'mimeType': mimeType,
      'clockRate': clockRate ?? 0,
      'parameters': parameters,
      'preferredPayloadType': preferredPayloadType ?? 0,
      'name': name,
      'kind': kind.index,
      'numChannels': numChannels
    };
  }

  /// Build MIME "type/subtype" string from `name` and `kind`.
  String mimeType;

  /// If unset, the implementation default is used.
  int? clockRate;

  /// Default payload type for this codec. Mainly needed for codecs that have
  /// statically assigned payload types.
  int? preferredPayloadType;

  /// Used to identify the codec. Equivalent to MIME subtype.
  String name;

  /// The media type of this codec. Equivalent to MIME top-level type.
  MediaKind kind;

  /// The number of audio channels used. Unset for video codecs. If unset for
  /// audio, the implementation default is used.
  int? numChannels;

  /// Codec-specific parameters that must be signaled to the remote party.
  ///
  /// Corresponds to "a=fmtp" parameters in SDP.
  ///
  /// Contrary to ORTC, these parameters are named using all lowercase strings.
  /// This helps make the mapping to SDP simpler, if an application is using SDP.
  /// Boolean values are represented by the string "1".
  Map<String, String> parameters;
}
