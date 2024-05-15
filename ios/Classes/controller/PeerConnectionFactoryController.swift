import Flutter

/// Controller of a `PeerConnection` factory management.
class PeerConnectionFactoryController {
  /// Flutter messenger for creating channels.
  private var messenger: FlutterBinaryMessenger

  /// Instance of the `PeerConnection` factory manager.
  private var peerFactory: PeerConnectionFactoryProxy

  /// Method channel for communicating with Flutter side.
  private var channel: FlutterMethodChannel

  /// Initializes a new `PeerConnectionFactoryController` and
  /// `PeerConnectionFactoryProxy` based on the provided `State`.
  init(messenger: FlutterBinaryMessenger, state: State) {
    Logger.log("KPRF1");
    let channelName = ChannelNameGenerator.name(
      name: "PeerConnectionFactory",
      id: 0
    )
    self.messenger = messenger
    self.peerFactory = PeerConnectionFactoryProxy(state: state)
    self.channel = FlutterMethodChannel(
      name: channelName,
      binaryMessenger: messenger
    )
    self.channel.setMethodCallHandler { call, result in
      Logger.log("KPRF2");
      self.onMethodCall(call: call, result: result)
    }
    Logger.log("KPRF3");
  }

  /// Handles all the supported Flutter method calls for the controlled
  /// `PeerConnectionFactoryProxy`.
  func onMethodCall(call: FlutterMethodCall, result: FlutterResult) {
    let argsMap = call.arguments as? [String: Any]
    switch call.method {
    case "create":
      Logger.log("NLUD1");
      let iceTransportTypeArg = argsMap!["iceTransportType"] as? Int
      let iceTransportType = IceTransportType(rawValue: iceTransportTypeArg!)!
      let iceServersArg = argsMap!["iceServers"] as? [Any]

      let iceServers = iceServersArg!.map { iceServerArg -> IceServer in
        let iceServer = iceServerArg as? [String: Any]
        let urlsArg = iceServer!["urls"] as? [String]
        let username = iceServer!["username"] as? String
        let password = iceServer!["password"] as? String

        return IceServer(urls: urlsArg!, username: username, password: password)
      }
      Logger.log("NLUD2");


      let conf = PeerConnectionConfiguration(
        iceServers: iceServers, iceTransportType: iceTransportType
      )

      var p = self.peerFactory.create(conf: conf);
      var a = p.signalingThread();
      Logger.log("NLUD2 \(a.size())");
      
      let peer = PeerConnectionController(
        messenger: self.messenger, peer: p
      )
      Logger.log("NLUD3");

      result(peer.asFlutterResult())
    case "getRtpSenderCapabilities":
      Logger.log("NAV1");
      let kind = argsMap!["kind"] as? Int
      let mediaKind = MediaType(rawValue: kind!)!
      Logger.log("NAV2");

      let capabilities = self
        .peerFactory
        .rtpSenderCapabilities(kind: mediaKind.intoWebRtc())
      Logger.log("NAV3");

      result(capabilities.asFlutterResult())
    case "videoEncoders":
      Logger.log("ER1");

      let res = [
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.VP8
        ),
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.VP9
        ),
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.AV1
        ),
        VideoCodecInfo(
          isHardwareAccelerated: true,
          codec: VideoCodec.H264
        ),
      ].map {
        $0.asFlutterResult()
      }
      Logger.log("ER2");
      result(res)
    case "videoDecoders":
      Logger.log("SV1");
      let res = [
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.VP8
        ),
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.VP9
        ),
        VideoCodecInfo(
          isHardwareAccelerated: false,
          codec: VideoCodec.AV1
        ),
        VideoCodecInfo(
          isHardwareAccelerated: true,
          codec: VideoCodec.H264
        ),
      ].map {
        $0.asFlutterResult()
      }
      Logger.log("SV2");
      result(res)
    case "dispose":
      do {
        Logger.log("LDPR1");
        self.channel.setMethodCallHandler(nil)
        Logger.log("LDPR2");
        result(nil)
        Logger.log("LDPR3");
      } catch {
        Logger.log("LDPR4");
      }
    default:
      result(FlutterMethodNotImplemented)
    }
  }
}
