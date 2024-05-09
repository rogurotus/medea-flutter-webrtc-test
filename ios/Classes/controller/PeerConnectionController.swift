import Flutter
class Logger {

    static var logFile: URL? {
        guard let documentsDirectory = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first else { return nil }
        let fileName = "LOGLOG.log"
        return documentsDirectory.appendingPathComponent(fileName)
    }

    static func log(_ message: String) {
        guard let logFile = logFile else {
            return
        }

        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        let timestamp = formatter.string(from: Date())
        guard let data = (timestamp + ": " + message + "\n").data(using: String.Encoding.utf8) else { return }

        if FileManager.default.fileExists(atPath: logFile.path) {
            if let fileHandle = try? FileHandle(forWritingTo: logFile) {
                fileHandle.seekToEndOfFile()
                fileHandle.write(data)
                fileHandle.closeFile()
            }
        } else {
            try? data.write(to: logFile, options: .atomicWrite)
        }
    }
}

/// Controller of a `PeerConnection`.
class PeerConnectionController {
  /// Flutter messenger for creating channels.
  private var messenger: FlutterBinaryMessenger

  /// Instance of the `PeerConnection`'s proxy'.
  private var peer: PeerConnectionProxy

  /// ID of the channel created for this controller.
  private var channelId: Int = ChannelNameGenerator.nextId()

  /// Controller of the `eventChannel` management.
  private var eventController: EventController

  /// Event channel for communicating with Flutter side.
  private var eventChannel: FlutterEventChannel

  /// Method channel for communicating with Flutter side.
  private var channel: FlutterMethodChannel

  /// Indicator whether this controller is disposed.
  private var isDisposed: Bool = false

  private var semaphore = DispatchSemaphore(value: 1)

  /// Initializes a new `PeerConnectionController` for the provided
  /// `PeerConnectionProxy`.
  init(messenger: FlutterBinaryMessenger, peer: PeerConnectionProxy) {
    let channelName = ChannelNameGenerator.name(
      name: "PeerConnection",
      id: self.channelId
    )
    self.eventController = EventController()
    self.messenger = messenger
    self.peer = peer
    self.peer.addEventObserver(
      eventObserver: PeerEventController(
        messenger: self.messenger, eventController: self.eventController
      )
    )
    self.channel = FlutterMethodChannel(
      name: channelName,
      binaryMessenger: messenger
    )
    self.eventChannel = FlutterEventChannel(
      name: ChannelNameGenerator.name(
        name: "PeerConnectionEvent",
        id: self.channelId
      ),
      binaryMessenger: messenger
    )
    self.channel.setMethodCallHandler { call, result in
      self.onMethodCall(call: call, result: result)
    }
    self.eventChannel.setStreamHandler(self.eventController)
  }

  /// Sends the provided response to the provided result.
  ///
  /// Checks whether `FlutterMethodChannel` is not disposed before sending data.
  /// If it's disposed, then does nothing.
  func sendResultFromTask(_ result: @escaping FlutterResult, _ response: Any?) {
    semaphore.wait()
    if !self.isDisposed {
      result(response)
    }
    semaphore.signal()
  }

  /// Handles all the supported Flutter method calls for the controlled
  /// `PeerConnectionProxy`.
  func onMethodCall(call: FlutterMethodCall, result: @escaping FlutterResult) {
    let argsMap = call.arguments as? [String: Any]
    switch call.method {
    case "createOffer":
      Task {
      Logger.log("DEBA26");

        do {
      Logger.log("DEBA27");

          let sdp = try await self.peer.createOffer()
      Logger.log("DEBA28");

          self.sendResultFromTask(result, sdp.asFlutterResult())
      Logger.log("DEBA29");

        } catch {
      Logger.log("DEBA30");

          self.sendResultFromTask(result, getFlutterError(error))
      Logger.log("DEBA31");

        }
      }
    case "createAnswer":
      Task {
      Logger.log("DEBA24");

        let sdp = try! await self.peer.createAnswer()
      Logger.log("DEBA25");

        self.sendResultFromTask(result, sdp.asFlutterResult())

      }
    case "setLocalDescription":
      Logger.log("Hello Swift 1")
      let description = argsMap!["description"] as? [String: Any]
      let type = description!["type"] as? Int
      let sdp = description!["description"] as? String
      Logger.log("Hello Swift 2")

      Task {
        do {
      Logger.log("Hello Swift 3")

          var desc: SessionDescription?
          if sdp == nil {
      Logger.log("Hello Swift 4")

            desc = nil
          } else {
      Logger.log("Hello Swift 5")

            desc = SessionDescription(
              type: SessionDescriptionType(rawValue: type!)!, description: sdp!
            )
      Logger.log("Hello Swift 6")

          }
      Logger.log("Hello Swift 7")
      print("Hello Swift 7")

        do {
          try await self.peer.setLocalDescription(description: desc)
      Logger.log("Hello Swift 8")

          self.sendResultFromTask(result, nil)
      Logger.log("Hello Swift 9")

        } catch {
      Logger.log("Hello Swift 10")

          self.sendResultFromTask(result, getFlutterError(error))
      Logger.log("Hello Swift 11")

        }

        } catch {
          Logger.log("HOOH");
        }
      }
    case "setRemoteDescription":
      Logger.log("DEBA18");

      let descriptionMap = argsMap!["description"] as? [String: Any]
      let type = descriptionMap!["type"] as? Int
      let sdp = descriptionMap!["description"] as? String
      Logger.log("DEBA19");

      Task {
        do {
      Logger.log("DEBA20");

          try await self.peer.setRemoteDescription(
            description: SessionDescription(
              type: SessionDescriptionType(rawValue: type!)!,
              description: sdp!
            )
          )
      Logger.log("DEBA21");

          self.sendResultFromTask(result, nil)
      Logger.log("DEBA22");

        } catch {
      Logger.log("DEBA23");

          self.sendResultFromTask(result, getFlutterError(error))
        }
      }
    case "addIceCandidate":
      Logger.log("DEBA13");

      let candidateMap = argsMap!["candidate"] as? [String: Any]
      let sdpMid = candidateMap!["sdpMid"] as? String
      let sdpMLineIndex = candidateMap!["sdpMLineIndex"] as? Int
      let candidate = candidateMap!["candidate"] as? String
      Logger.log("DEBA14");

      Task {
        do {
      Logger.log("DEBA15");

          try await self.peer.addIceCandidate(
            candidate: IceCandidate(
              sdpMid: sdpMid!, sdpMLineIndex: sdpMLineIndex!,
              candidate: candidate!
            )
          )
      Logger.log("DEBA16");

          self.sendResultFromTask(result, nil)
        } catch {
      Logger.log("DEBA17");

          self.sendResultFromTask(result, getFlutterError(error))
        }
      }
    case "addTransceiver":
      Logger.log("DEBA9");

      let mediaType = argsMap!["mediaType"] as? Int
      let initArgs = argsMap!["init"] as? [String: Any]
      let direction = initArgs!["direction"] as? Int
      let argEncodings = initArgs!["sendEncodings"] as? [[String: Any]]
      var sendEncodings: [Encoding] = []

      Logger.log("DEBA10");

      for e in argEncodings! {
      Logger.log("DEBA11");

        let rid = e["rid"] as? String
        let active = e["active"] as? Bool
        let maxBitrate = e["maxBitrate"] as? Int
        let maxFramerate = e["maxFramerate"] as? Double
        let scaleResolutionDownBy = e["scaleResolutionDownBy"] as? Double
        let scalabilityMode = e["scalabilityMode"] as? String

      Logger.log("DEBA12");

        sendEncodings.append(Encoding(
          rid: rid!,
          active: active!,
          maxBitrate: maxBitrate,
          maxFramerate: maxFramerate,
          scaleResolutionDownBy: scaleResolutionDownBy,
          scalabilityMode: scalabilityMode
        ))
      }

      let transceiverInit =
        TransceiverInit(
          direction: TransceiverDirection(rawValue: direction!)!,
          encodings: sendEncodings
        )

      do {
        let transceiver = try RtpTransceiverController(
          messenger: self.messenger,
          transceiver: self.peer.addTransceiver(
            mediaType: MediaType(rawValue: mediaType!)!,
            transceiverInit: transceiverInit
          )
        )
        result(transceiver.asFlutterResult())
      } catch {
        result(getFlutterError(error))
      }
    case "getTransceivers":
      Logger.log("DEBA8");

      result(
        self.peer.getTransceivers().map {
          RtpTransceiverController(messenger: self.messenger, transceiver: $0)
            .asFlutterResult()
        }
      )
    case "restartIce":
      Logger.log("DEBA5");

      self.peer.restartIce()
      Logger.log("DEBA6");
      result(nil)
    case "dispose":
      Logger.log("DEBA");
      var err = 1/0;
      semaphore.wait()
      isDisposed = true
      semaphore.signal()
      Logger.log("DEBA2");
      self.peer.dispose()
      Logger.log("FIXX???");

      self.channel.setMethodCallHandler(nil)
      Logger.log("DEBA3");
      Logger.log("DEBA4");
      result(nil)
    default:
      Logger.log("DEBA77");

      result(FlutterMethodNotImplemented)
    }
  }

  /// Converts this controller into a Flutter method call response.
  func asFlutterResult() -> [String: Any] {
    [
      "channelId": self.channelId,
      "id": self.peer.getId(),
    ]
  }
}
