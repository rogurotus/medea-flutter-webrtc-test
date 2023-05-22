// ignore_for_file: avoid_print
import 'dart:core';

import 'package:flutter/material.dart';
import 'package:medea_flutter_webrtc/medea_flutter_webrtc.dart';

class Loopback extends StatefulWidget {
  static String tag = 'get_usermedia_sample';

  const Loopback({Key? key}) : super(key: key);

  @override
  State<Loopback> createState() => _LoopbackState();
}

class _LoopbackState extends State<Loopback> {
  List<MediaStreamTrack>? _tracks;

  PeerConnection? _pc1;
  PeerConnection? _pc2;

  List<MediaDeviceInfo>? _mediaDevicesList;
  final _localRenderer = createVideoRenderer();
  final _remoteRenderer = createVideoRenderer();
  bool _inCalling = false;
  bool _mic = true;
  bool _cam = true;
  int _volume = -1;
  bool _microIsAvailable = false;

  RtpTransceiver? _audioTransceiver;

  @override
  void initState() {
    super.initState();
    initRenderers();

    () async {
      _mediaDevicesList = await enumerateDevices();
      if (await microphoneVolumeIsAvailable()) {
        var volume = await microphoneVolume();
        setState(() {
          _volume = volume;
        });
      }
    }();
  }

  @override
  void deactivate() {
    super.deactivate();
    if (_inCalling) {
      _hangUp();
    }
    _localRenderer.dispose();
    _remoteRenderer.dispose();
  }

  void initRenderers() async {
    await _localRenderer.initialize();
    await _remoteRenderer.initialize();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  void _makeCall() async {
    var caps = DeviceConstraints();
    caps.audio.mandatory = AudioConstraints();
    caps.video.mandatory = DeviceVideoConstraints();
    caps.video.mandatory!.width = 640;
    caps.video.mandatory!.height = 480;
    caps.video.mandatory!.fps = 30;

    try {
      _tracks = await getUserMedia(caps);
      await _localRenderer.setSrcObject(
          _tracks!.firstWhere((track) => track.kind() == MediaKind.video));

      var server = IceServer(['stun:stun.l.google.com:19302']);
      _pc1 = await PeerConnection.create(IceTransportType.all, [server]);
      _pc2 = await PeerConnection.create(IceTransportType.all, [server]);

      _pc1?.onIceCandidateError((p0) {
        print(p0.errorText);
      });
      _pc2?.onIceCandidateError((p0) {
        print(p0.errorText);
      });

      _pc2?.onTrack((track, trans) async {
        if (track.kind() == MediaKind.video) {
          await _remoteRenderer.setSrcObject(track);
        }
      });

      var vtrans = await _pc1?.addTransceiver(
          MediaKind.video, RtpTransceiverInit(TransceiverDirection.sendOnly));

      var atrans = await _pc1?.addTransceiver(
          MediaKind.audio, RtpTransceiverInit(TransceiverDirection.sendOnly));
      _audioTransceiver = atrans;

      var offer = await _pc1?.createOffer();
      await _pc1?.setLocalDescription(offer!);
      await _pc2?.setRemoteDescription(offer!);

      var answer = await _pc2?.createAnswer();
      await _pc2?.setLocalDescription(answer!);
      await _pc1?.setRemoteDescription(answer!);

      _pc1?.onIceCandidate((IceCandidate candidate) async {
        print(candidate.candidate.toString());
        await _pc2?.addIceCandidate(candidate);
      });

      _pc2?.onIceCandidate((IceCandidate candidate) async {
        print(candidate.candidate.toString());
        await _pc1?.addIceCandidate(candidate);
      });

      await vtrans?.sender.replaceTrack(
          _tracks!.firstWhere((track) => track.kind() == MediaKind.video));

      await atrans?.sender.replaceTrack(
          _tracks!.firstWhere((track) => track.kind() == MediaKind.audio));
    } catch (e) {
      print(e.toString());
    }
    if (!mounted) return;

    _inCalling = true;
    microphoneVolumeIsAvailable().then((value) {
      setState(() {
        _microIsAvailable = value;
      });
    });
  }

  void _hangUp() async {
    try {
      await _localRenderer.setSrcObject(null);
      await _remoteRenderer.setSrcObject(null);

      for (var track in _tracks!) {
        await track.stop();
        await track.dispose();
      }

      await _pc1?.close();
      await _pc2?.close();

      setState(() {
        _inCalling = false;
        _mic = true;
        _cam = true;
      });
    } catch (e) {
      print(e.toString());
    }
  }

  void _selectAudioInput(String deviceId) async {
    var constraints = DeviceConstraints();
    constraints.audio.mandatory = AudioConstraints();
    constraints.audio.mandatory!.deviceId = deviceId;
    print("selectAudioInput22");
    try {
      var audioTrack = await getUserMedia(constraints);
      print("Audio Tracks count: ${audioTrack.length}");
      for (var t in audioTrack) {
        print("DeviceID of Track: ${t.deviceId()}");
      }
      await _audioTransceiver?.sender.replaceTrack(audioTrack.first);
      print("Requested deviceId: $deviceId");
    } catch (e) {
      print("Exception $e");
    }
    // await _audioTransceiver?.sender.replaceTrack(
    //     _tracks!.firstWhere((track) => track.deviceId() == deviceId));
  }

  void _selectAudioOutput(String deviceId) {
    setOutputAudioId(deviceId);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(
            'GetUserMedia API Test. ${_inCalling ? (_microIsAvailable ? 'Micro volume: $_volume .' : 'Microphone is not available!') : ''}'),
        actions: _inCalling
            ? <Widget>[
          PopupMenuButton<String>(
            onSelected: _selectAudioOutput,
            itemBuilder: (BuildContext context) {
              if (_mediaDevicesList != null) {
                return _mediaDevicesList!
                    .where((device) =>
                device.kind == MediaDeviceKind.audiooutput)
                    .map((device) {
                  return PopupMenuItem<String>(
                    value: device.deviceId,
                    child: Text(device.label),
                  );
                }).toList();
              }
              return [];
            },
            icon: const Icon(Icons.volume_down),
          ),
                IconButton(
                  icon: const Icon(Icons.remove),
                  tooltip: 'Micro lower',
                  onPressed: _microIsAvailable
                      ? () async {
                          setState(() {
                            _volume = _volume >= 10 ? _volume - 10 : 0;
                          });
                          await setMicrophoneVolume(_volume);
                        }
                      : null,
                ),
                PopupMenuButton<String>(
                  onSelected: _selectAudioInput,
                  itemBuilder: (BuildContext context) {
                    if (_mediaDevicesList != null) {
                      return _mediaDevicesList!
                          .where((device) =>
                      device.kind == MediaDeviceKind.audioinput)
                          .map((device) {
                        return PopupMenuItem<String>(
                          value: device.deviceId,
                          child: Text(device.label),
                        );
                      }).toList();
                    }
                    return [];
                  },
                  icon: const Icon(Icons.mic),
                ),
          IconButton(
            icon: const Icon(Icons.add),
            tooltip: 'Micro louder',
                  onPressed: _microIsAvailable
                      ? () async {
                          print("Micro louder");
                          await _audioTransceiver?.getDirection();
                          setState(() {
                            _volume = _volume <= 90 ? _volume + 10 : 100;
                          });
                          await setMicrophoneVolume(_volume);
                        }
                      : null,
                ),
                IconButton(
                  icon:
                      _mic ? const Icon(Icons.mic_off) : const Icon(Icons.mic),
                  tooltip: _mic ? 'Disable audio rec' : 'Enable audio rec',
                  onPressed: () async {
                    setState(() {
                      _mic = !_mic;
                    });
                    _tracks!
                        .firstWhere((track) => track.kind() == MediaKind.audio)
                        .setEnabled(_mic);
                  },
                ),
                IconButton(
                  icon: _cam
                      ? const Icon(Icons.videocam_off)
                      : const Icon(Icons.videocam),
                  tooltip: _cam ? 'Disable video rec' : 'Enable video rec',
                  onPressed: () {
                    setState(() {
                      _cam = !_cam;
                    });
                    _tracks!
                        .firstWhere((track) => track.kind() == MediaKind.video)
                        .setEnabled(_cam);
                  },
                ),
              ]
            : null,
      ),
      body: OrientationBuilder(
        builder: (context, orientation) {
          return Center(
              child: Row(
            children: [
              Container(
                margin: const EdgeInsets.fromLTRB(0.0, 0.0, 0.0, 0.0),
                width: MediaQuery.of(context).size.width / 2,
                height: MediaQuery.of(context).size.height,
                decoration: const BoxDecoration(color: Colors.black54),
                child:
                    VideoView(_localRenderer, mirror: true, autoRotate: false),
              ),
              Container(
                margin: const EdgeInsets.fromLTRB(0.0, 0.0, 0.0, 0.0),
                width: MediaQuery.of(context).size.width / 2,
                height: MediaQuery.of(context).size.height,
                decoration: const BoxDecoration(color: Colors.black54),
                child: VideoView(_remoteRenderer, mirror: true),
              ),
            ],
          ));
        },
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _inCalling ? _hangUp : _makeCall,
        tooltip: _inCalling ? 'Hangup' : 'Call',
        child: Icon(_inCalling ? Icons.call_end : Icons.phone),
      ),
    );
  }
}
