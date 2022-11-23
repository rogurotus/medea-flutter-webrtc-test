use std::sync::mpsc;

pub use super::{
    AudioConstraints, BundlePolicy, CandidateType, GetMediaError,
    GetMediaResult, IceCandidateStats, IceConnectionState, IceGatheringState,
    IceRole, IceTransportsType, MediaDeviceInfo, MediaDeviceKind,
    MediaDisplayInfo, MediaStreamConstraints, MediaStreamTrack, MediaType,
    PeerConnectionEvent, PeerConnectionState, Protocol, RtcConfiguration,
    RtcIceCandidateStats, RtcIceServer, RtcInboundRtpStreamMediaType,
    RtcMediaSourceStatsMediaType, RtcOutboundRtpStreamStatsMediaType,
    RtcRtpTransceiver, RtcSessionDescription, RtcStats,
    RtcStatsIceCandidatePairState, RtcStatsType, RtcTrackEvent,
    RtpTransceiverDirection, SdpType, SignalingState, TrackEvent, TrackKind,
    TrackState, VideoConstraints,
};

use crate::stream_sink::Sink;

use super::WEBRTC;

/// Sets the provided [`OnDeviceChangeCallback`] as the callback to be called
/// whenever a set of available media devices changes.
///
/// Only one callback can be set at a time, so the previous one will be dropped,
/// if any.
pub fn set_on_device_changed(cb: mpsc::Sender<()>) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_on_device_changed(cb.into())
}

/// Registers an observer to the [`MediaStreamTrack`] events.
pub fn register_track_observer(
    cb: mpsc::Sender<TrackEvent>,
    track_id: String,
    kind: MediaType,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .register_track_observer(track_id, kind, cb.into())
}

pub fn create_peer_connection(
    cb: mpsc::Sender<PeerConnectionEvent>,
    configuration: RtcConfiguration,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .create_peer_connection(Sink::from(cb), configuration)
}
