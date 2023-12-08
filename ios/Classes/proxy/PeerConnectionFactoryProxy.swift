import WebRTC

/// Creator of new `PeerConnectionProxy`s.
class PeerConnectionFactoryProxy {
  /// Counter for generating new [PeerConnectionProxy] IDs.
  private var lastPeerConnectionId: Int = 0

  /// All the `PeerObserver`s created by this `PeerConnectionFactoryProxy`.
  ///
  /// `PeerObserver`s will be removed on a `PeerConnectionProxy` disposal.
  private var peerObservers: [Int: PeerObserver] = [:]

  /// Underlying native factory object of this factory.
  public var factory: RTCPeerConnectionFactory

  /// Initializes a new `PeerConnectionFactoryProxy` based on the provided
  /// `State`.
  init(state: State) {
    self.factory = state.getPeerFactory()
  }

  /// Creates a new `PeerConnectionProxy` based on the provided
  /// `PeerConnectionConfiguration`. 
  func create(conf: PeerConnectionConfiguration) -> PeerConnectionProxy {
    print("SBUG 1")
    let id = self.nextId()
    print("SBUG 2")

    let config = conf.intoWebRtc()
    print("SBUG 3")
    let peerObserver = PeerObserver()
    print("SBUG 4")
    let peer = self.factory.peerConnection(
      with: config,
      constraints: RTCMediaConstraints(
        mandatoryConstraints: [:],
        optionalConstraints: [:]
      ),
      delegate: peerObserver
    )
    print("SBUG 5")
    let peerProxy = PeerConnectionProxy(id: id, peer: peer!)
    print("SBUG 6")
    peerObserver.setPeer(peer: peerProxy)
    print("SBUG 7")

    self.peerObservers[id] = peerObserver
    print("SBUG 8")

    return peerProxy
  }

  /// Removes the specified [PeerObserver] from the [peerObservers].
  private func remotePeerObserver(id: Int) {
    self.peerObservers.removeValue(forKey: id)
  }

  /// Generates the next track ID.
  private func nextId() -> Int {
    self.lastPeerConnectionId += 1
    return self.lastPeerConnectionId
  }
}
