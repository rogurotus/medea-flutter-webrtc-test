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
    Logger.log("GRHMDA 5");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD 5");

      self.peer!.broadcastEventObserver().onSignalingStateChange(
        state: SignalingState.fromWebRtc(state: stateChanged)
      )
      Logger.log("GRHMDA DEAD 5A");
    }
  }

  /// Fires an `onIceConnectionStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceConnectionState
  ) {
    Logger.log("GRHMDA 3");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD 3");

      self.peer!.broadcastEventObserver().onIceConnectionStateChange(
        state: IceConnectionState.fromWebRtc(state: newState)
      )
      Logger.log("GRHMDA DEAD 3A");

    }
  }

  /// Fires an `onConnectionStateChange` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCPeerConnectionState
  ) {
    Logger.log("GRHMDA 1");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD");
      self.peer!.broadcastEventObserver().onConnectionStateChange(
        state: PeerConnectionState.fromWebRtc(state: newState)
      )
      Logger.log("GRHMDA DEAD a");
    }
  }

  /// Fires an `onIceGatheringStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceGatheringState
  ) {
    DispatchQueue.main.async {
      self.peer!.broadcastEventObserver().onIceGatheringStateChange(
        state: IceGatheringState.fromWebRtc(state: newState)
      )
    }
  }

  /// Fires an `onIceCandidate` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didGenerate candidate: RTCIceCandidate
  ) {
    Logger.log("GRHMDA 2");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD 2");

      self.peer!.broadcastEventObserver().onIceCandidate(
        candidate: IceCandidate(candidate: candidate)
      )
      Logger.log("GRHMDA DEAD 2A");
    }
  }

  /// Fires an `onTrack` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didStartReceivingOn transceiver: RTCRtpTransceiver
  ) {
    DispatchQueue.main.async {
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
    }
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
    DispatchQueue.main.async {
      self.peer!.receiverRemoved(endedReceiver: receiver)
    }
  }

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didAdd _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didRemove _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didOpen _: RTCDataChannel) {}

  /// Does nothing.
  func peerConnectionShouldNegotiate(_: RTCPeerConnection) {
    Logger.log("GRHMDA 4");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD 4");

      self.peer!.broadcastEventObserver().onNegotiationNeeded()
      Logger.log("GRHMDA DEAD 4A");
    }
  }

  /// Does nothing.
  func peerConnection(
    _: RTCPeerConnection, didRemove _: [RTCIceCandidate]
  ) {}
}
