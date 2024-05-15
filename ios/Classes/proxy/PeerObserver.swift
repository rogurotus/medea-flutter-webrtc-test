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
    do {
    Logger.log("GRHMDA 4 \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 4 \(self.peer!.getId())");

      self.peer!.broadcastEventObserver().onSignalingStateChange(
        state: SignalingState.fromWebRtc(state: stateChanged)
      )
      Logger.log("GRHMDA DEAD 4A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 4 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 4 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 44");
  }

  /// Fires an `onIceConnectionStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceConnectionState
  ) {
    do {
    Logger.log("GRHMDA 5 \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 5 \(self.peer!.getId())");
        self.peer!.broadcastEventObserver().onIceConnectionStateChange(
          state: IceConnectionState.fromWebRtc(state: newState)
        )
        Logger.log("GRHMDA DEAD 5A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 5 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 5 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 55 \(self.peer!.getId())");
  }

  /// Fires an `onConnectionStateChange` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCPeerConnectionState
  ) {
    do {
    Logger.log("GRHMDA 6 \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 6 \(self.peer!.getId())");
      self.peer!.broadcastEventObserver().onConnectionStateChange(
        state: PeerConnectionState.fromWebRtc(state: newState)
      )
      Logger.log("GRHMDA DEAD 6A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 6 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 6 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 66");
  }

  /// Fires an `onIceGatheringStateChange` callback in the
  /// `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didChange newState: RTCIceGatheringState
  ) {
    do {
    Logger.log("GRHMDA 7 \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 7 \(self.peer!.getId())");
      self.peer!.broadcastEventObserver().onIceGatheringStateChange(
        state: IceGatheringState.fromWebRtc(state: newState)
      )
      Logger.log("GRHMDA DEAD 7A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 7 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 7 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 77 \(self.peer!.getId())");
  }

  /// Fires an `onIceCandidate` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didGenerate candidate: RTCIceCandidate
  ) {
          do {
    Logger.log("GRHMDA 8 \(self.peer!.getId())");
    DispatchQueue.main.async {
            do {
      Logger.log("GRHMDA DEAD 8 \(self.peer!.getId())");

      self.peer!.broadcastEventObserver().onIceCandidate(
        candidate: IceCandidate(candidate: candidate)
      )
      Logger.log("GRHMDA DEAD 8A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 8 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 8 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 88 \(self.peer!.getId())");
  }

  /// Fires an `onTrack` callback in the `PeerConnectionProxy`.
  func peerConnection(
    _: RTCPeerConnection, didStartReceivingOn transceiver: RTCRtpTransceiver
  ) {
          do {
    Logger.log("GRHMDA 9 \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD 9 \(self.peer!.getId())");
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
      Logger.log("GRHMDA DEAD 9A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 9 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch { 
      Logger.log("GRHMDA Error 9 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 99 \(self.peer!.getId())");
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
    do {
    Logger.log("GRHMDA 0 \(self.peer!.getId())");
    DispatchQueue.main.async {
      Logger.log("GRHMDA DEAD 0 \(self.peer!.getId())");

      do {
      self.peer!.receiverRemoved(endedReceiver: receiver)
      Logger.log("GRHMDA DEAD 0A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into 0 info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error 0 info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA 00 \(self.peer!.getId())");
  }

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didAdd _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didRemove _: RTCMediaStream) {}

  /// Does nothing.
  func peerConnection(_: RTCPeerConnection, didOpen _: RTCDataChannel) {}

  /// Does nothing.
  func peerConnectionShouldNegotiate(_: RTCPeerConnection) {
    do {
    Logger.log("GRHMDA + \(self.peer!.getId())");
    DispatchQueue.main.async {
      do {
      Logger.log("GRHMDA DEAD + \(self.peer!.getId())");

      self.peer!.broadcastEventObserver().onNegotiationNeeded()
      Logger.log("GRHMDA DEAD +A \(self.peer!.getId())");
      } catch {
        Logger.log("GRHMDA Error into + info: \(error) \(self.peer!.getId())");
      }
    }
    } catch {
      Logger.log("GRHMDA Error + info: \(error) \(self.peer!.getId())");
    }
    Logger.log("GRHMDA ++ \(self.peer!.getId())");
  }

  /// Does nothing.
  func peerConnection(
    _: RTCPeerConnection, didRemove _: [RTCIceCandidate]
  ) {}
}
