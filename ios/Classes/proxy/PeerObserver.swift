import Dispatch
import WebRTC

/// Observer for a native `RTCPeerConnectionDelegate`.
class PeerObserver: NSObject, RTCPeerConnectionDelegate {
  /// `PeerConnectionProxy` into which callbacks will be provided.
  var peer: PeerConnectionProxy?

  override init() {}

  /// Sets the underlying `PeerConnectionProxy` for this `PeerObserver`.
  func setPeer(peer: PeerConnectionProxy) {
    self.peer = peer
  }

  /// Fires an `onSignalingStateChange` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange stateChanged: RTCSignalingState
  ) {
    // do {
    // Logger.log("GRHMDA 4");
    // DispatchQueue.main.async {
    //   do {
    //   Logger.log("GRHMDA DEAD 4");

    //   self.peer!.broadcastEventObserver().onSignalingStateChange(
    //     state: SignalingState.fromWebRtc(state: stateChanged)
    //   )
    //   Logger.log("GRHMDA DEAD 4A");
    //   } catch {
    //     Logger.log("GRHMDA Error into 4 info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error 4 info: \(error)");
    // }
    Logger.log("GRHMDA 44");
  }

  /// Fires an `onIceConnectionStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceConnectionState
  ) {
    // do {
    // Logger.log("GRHMDA 5");
    // DispatchQueue.main.async {
    //   do {
    //   Logger.log("GRHMDA DEAD 5");
    //     self.peer!.broadcastEventObserver().onIceConnectionStateChange(
    //       state: IceConnectionState.fromWebRtc(state: newState)
    //     )
    //     Logger.log("GRHMDA DEAD 5A");
    //   } catch {
    //     Logger.log("GRHMDA Error into 5 info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error 5 info: \(error)");
    // }
    Logger.log("GRHMDA 55");
  }

  /// Fires an `onConnectionStateChange` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCPeerConnectionState
  ) {
    // do {
    // Logger.log("GRHMDA 6");
    // DispatchQueue.main.async {
    //   do {
    //   Logger.log("GRHMDA DEAD 6");
    //   self.peer!.broadcastEventObserver().onConnectionStateChange(
    //     state: PeerConnectionState.fromWebRtc(state: newState)
    //   )
    //   Logger.log("GRHMDA DEAD 6A");
    //   } catch {
    //     Logger.log("GRHMDA Error into 6 info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error 6 info: \(error)");
    // }
    Logger.log("GRHMDA 66");
  }

  /// Fires an `onIceGatheringStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceGatheringState
  ) {
    // do {
    // Logger.log("GRHMDA 7");
    // DispatchQueue.main.async {
    //   do {
    //   Logger.log("GRHMDA DEAD 7");
    //   self.peer!.broadcastEventObserver().onIceGatheringStateChange(
    //     state: IceGatheringState.fromWebRtc(state: newState)
    //   )
    //   Logger.log("GRHMDA DEAD 7A");
    //   } catch {
    //     Logger.log("GRHMDA Error into 7 info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error 7 info: \(error)");
    // }
    Logger.log("GRHMDA 77");
  }

  /// Fires an `onIceCandidate` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didGenerate candidate: RTCIceCandidate
  ) {
          do {
    Logger.log("GRHMDA 8");
    DispatchQueue.main.async {
            do {
      Logger.log("GRHMDA DEAD 8");

      self.peer!.broadcastEventObserver().onIceCandidate(
        candidate: IceCandidate(candidate: candidate)
      )
      Logger.log("GRHMDA DEAD 8A");
      } catch {
        Logger.log("GRHMDA Error into 8 info: \(error)");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 8 info: \(error)");
    }
    Logger.log("GRHMDA 88");
  }

  /// Fires an `onTrack` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didStartReceivingOn transceiver: RTCRtpTransceiver
  ) {
          do {
    Logger.log("GRHMDA 9");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 9");
      let receiver = transceiver.receiver
      let track = receiver.track
      if track != nil {
        let transceivers = self.peer!.getTransceivers()
        for trans in transceivers {
          if trans.getReceiver().id() == receiver.receiverId {
            self.peer!.broadcastEventObserver().onTrack(
              track: trans.getReceiver().getTrack(),
              transceiver: trans
            )
          }
        }
      }
      Logger.log("GRHMDA DEAD 9A");
      } catch {
        Logger.log("GRHMDA Error into 9 info: \(error)");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 9 info: \(error)");
    }
    Logger.log("GRHMDA 99");
  }

  /// Does nothing.
  func peerConnection(
    _: RTCPeerConnection, didAdd _: RTCRtpReceiver,
    streams _: [RTCMediaStream]
  ) {}

  /// Does nothing.
  func peerConnection(
    _: RTCPeerConnection, didRemove receiver: RTCRtpReceiver
  ) {
    // do {
    // Logger.log("GRHMDA 0");
    // DispatchQueue.main.async {
    //   Logger.log("GRHMDA DEAD 0");

    //   do {
    //   self.peer!.receiverRemoved(endedReceiver: receiver)
    //   Logger.log("GRHMDA DEAD 0A");
    //   } catch {
    //     Logger.log("GRHMDA Error into 0 info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error 0 info: \(error)");
    // }
    // Logger.log("GRHMDA 00");
  }

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didAdd _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didRemove _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didOpen _: RTCDataChannel) {}

  /// Does nothing.
  func peerConnectionShouldNegotiate(_: RTCPeerConnection) {
    // do {
    // Logger.log("GRHMDA +");
    // DispatchQueue.main.async {
    //   do {
    //   Logger.log("GRHMDA DEAD +");

    //   self.peer!.broadcastEventObserver().onNegotiationNeeded()
    //   Logger.log("GRHMDA DEAD +A");
    //   } catch {
    //     Logger.log("GRHMDA Error into + info: \(error)");
    //   }
    // }
    // } catch {
    //   Logger.log("GRHMDA Error + info: \(error)");
    // }
    // Logger.log("GRHMDA ++");
  }

  /// Does nothing.
  func peerConnection(
    _: RTCPeerConnection, didRemove _: [RTCIceCandidate]
  ) {}
}
