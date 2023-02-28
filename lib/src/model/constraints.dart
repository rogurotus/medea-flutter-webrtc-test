import 'package:medea_flutter_webrtc/src/model/track.dart';

/// Device audio and video constraints data.
class DisplayConstraints {
  /// Desired processing options to be applied to the video track rendition of
  /// the display surface chosen by the user.
  DeviceConstraintMap<AudioConstraints> audio = DeviceConstraintMap();

  /// Desired processing options to be applied to the audio track.
  DeviceConstraintMap<DeviceVideoConstraints> video = DeviceConstraintMap();

  /// Converts this model to the [Map] expected by Flutter.
  Map<String, Map<String, dynamic>> toMap() {
    return {
      'audio': audio.toMap(),
      'video': video.toMap(),
    };
  }
}

/// Device audio and video constraints data.
class DeviceConstraints {
  /// Optional constraints to lookup audio devices with.
  DeviceConstraintMap<AudioConstraints> audio = DeviceConstraintMap();

  /// Optional constraints to lookup video devices with.
  DeviceConstraintMap<DeviceVideoConstraints> video = DeviceConstraintMap();

  /// Converts this model to the [Map] expected by Flutter.
  Map<String, Map<String, dynamic>> toMap() {
    return {
      'audio': audio.toMap(),
      'video': video.toMap(),
    };
  }
}

/// Abstract device constraint property.
class DeviceConstraintMap<T extends DeviceMediaConstraints> {
  /// Storage for the mandatory constraint.
  T? mandatory;

  /// Storage for the optional constraint.
  T? optional;

  /// Converts this model to the [Map] expected by Flutter.
  Map<String, dynamic> toMap() {
    var map = <String, dynamic>{};
    if (mandatory != null) {
      map['mandatory'] = mandatory?.toMap();
    }
    if (optional != null) {
      map['optional'] = optional?.toMap();
    }
    return map;
  }
}

/// Some device abstract constraints.
abstract class DeviceMediaConstraints {
  /// Converts [DeviceMediaConstraints] to the [Map] expected by Flutter.
  Map<String, dynamic> toMap();
}

/// [DeviceMediaConstraints] for audio devices.
class AudioConstraints implements DeviceMediaConstraints {
  String? deviceId;

  /// Converts this model to the [Map] expected by Flutter.
  @override
  Map<String, dynamic> toMap() {
    var map = <String, dynamic>{};
    if (deviceId != null) {
      map['deviceId'] = deviceId;
    }
    return map;
  }
}

/// Device constraints related to a video.
class DeviceVideoConstraints implements DeviceMediaConstraints {
  /// Constraint to search for a device with some concrete device ID.
  String? deviceId;

  /// Constraint to search for a device with some [FacingMode].
  FacingMode? facingMode;

  /// Constraint to search for a device with a concrete height.
  int? height;

  /// Constraint to search for a device with a concrete width.
  int? width;

  /// Constraint to search for a device with a concrete FPS.
  int? fps;

  /// Converts this model to the [Map] expected by Flutter.
  @override
  Map<String, dynamic> toMap() {
    var map = <String, dynamic>{};
    if (deviceId != null) {
      map['deviceId'] = deviceId;
    }
    if (facingMode != null) {
      map['facingMode'] = facingMode!.index;
    }
    if (height != null) {
      map['height'] = height;
    }
    if (width != null) {
      map['width'] = width;
    }
    if (fps != null) {
      map['fps'] = fps;
    }
    return map;
  }
}
