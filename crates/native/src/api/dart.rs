use std::{
    sync::{atomic::Ordering, mpsc},
    time::Duration,
};

use crate::{devices, renderer::FrameHandler, FAKE_MEDIA, WEBRTC};

use super::{
    GetMediaResult, MediaDeviceInfo, MediaDisplayInfo, MediaStreamConstraints,
    MediaStreamTrack, MediaType, RtcRtpTransceiver, RtcSessionDescription,
    RtcStats, RtpTransceiverDirection, SdpType, TrackState,
};

#[cfg(feature = "dart_api")]
use super::{PeerConnectionEvent, RtcConfiguration, TrackEvent};

/// Timeout for [`mpsc::Receiver::recv_timeout()`] operations.
pub static RX_TIMEOUT: Duration = Duration::from_secs(5);

#[cfg(feature = "dart_api")]
/// Creates a new [`PeerConnection`] and returns its ID.
#[allow(clippy::needless_pass_by_value)]
pub fn create_peer_connection(
    cb: flutter_rust_bridge::StreamSink<PeerConnectionEvent>,
    configuration: RtcConfiguration,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .create_peer_connection(&(cb.into()), configuration)
}

#[cfg(feature = "dart_api")]
/// Registers an observer to the [`MediaStreamTrack`] events.
pub fn register_track_observer(
    cb: flutter_rust_bridge::StreamSink<TrackEvent>,
    track_id: String,
    kind: MediaType,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .register_track_observer(track_id, kind, cb.into())
}
#[cfg(feature = "dart_api")]
/// Sets the provided [`OnDeviceChangeCallback`] as the callback to be
/// called whenever a set of available media devices changes.
///
/// Only one callback can be set at a time, so the previous one will be
/// dropped, if any.
pub fn set_on_device_changed(
    cb: flutter_rust_bridge::StreamSink<()>,
) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_on_device_changed(cb.into())
}

/// Configures media acquisition to use fake devices instead of actual camera
/// and microphone.
pub fn enable_fake_media() {
    FAKE_MEDIA.store(true, Ordering::Release);
}

/// Indicates whether application is configured to use fake media devices.
pub fn is_fake_media() -> bool {
    FAKE_MEDIA.load(Ordering::Acquire)
}

/// Returns a list of all available media input and output devices, such as
/// microphones, cameras, headsets, and so forth.
pub fn enumerate_devices() -> anyhow::Result<Vec<MediaDeviceInfo>> {
    WEBRTC.lock().unwrap().enumerate_devices()
}

/// Returns a list of all available displays that can be used for screen
/// capturing.
pub fn enumerate_displays() -> Vec<MediaDisplayInfo> {
    devices::enumerate_displays()
}

/// Initiates the creation of an SDP offer for the purpose of starting a new
/// WebRTC connection to a remote peer.
pub fn create_offer(
    peer_id: u64,
    voice_activity_detection: bool,
    ice_restart: bool,
    use_rtp_mux: bool,
) -> anyhow::Result<RtcSessionDescription> {
    let (tx, rx) = mpsc::channel();

    WEBRTC.lock().unwrap().create_offer(
        peer_id,
        voice_activity_detection,
        ice_restart,
        use_rtp_mux,
        tx,
    )?;

    rx.recv_timeout(RX_TIMEOUT)?
}

/// Creates an SDP answer to an offer received from a remote peer during an
/// offer/answer negotiation of a WebRTC connection.
pub fn create_answer(
    peer_id: u64,
    voice_activity_detection: bool,
    ice_restart: bool,
    use_rtp_mux: bool,
) -> anyhow::Result<RtcSessionDescription> {
    let (tx, rx) = mpsc::channel();

    WEBRTC.lock().unwrap().create_answer(
        peer_id,
        voice_activity_detection,
        ice_restart,
        use_rtp_mux,
        tx,
    )?;

    rx.recv_timeout(RX_TIMEOUT)?
}

/// Changes the local description associated with the connection.
pub fn set_local_description(
    peer_id: u64,
    kind: SdpType,
    sdp: String,
) -> anyhow::Result<()> {
    let (tx, rx) = mpsc::channel();

    WEBRTC.lock().unwrap().set_local_description(
        peer_id,
        kind.into(),
        sdp,
        tx,
    )?;

    rx.recv_timeout(RX_TIMEOUT)?
}

/// Sets the specified session description as the remote peer's current offer or
/// answer.
pub fn set_remote_description(
    peer_id: u64,
    kind: SdpType,
    sdp: String,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .set_remote_description(peer_id, kind.into(), sdp)
}

/// Creates a new [`RtcRtpTransceiver`] and adds it to the set of transceivers
/// of the specified [`PeerConnection`].
pub fn add_transceiver(
    peer_id: u64,
    media_type: MediaType,
    direction: RtpTransceiverDirection,
) -> anyhow::Result<RtcRtpTransceiver> {
    WEBRTC.lock().unwrap().add_transceiver(
        peer_id,
        media_type.into(),
        direction.into(),
    )
}

/// Returns a sequence of [`RtcRtpTransceiver`] objects representing the RTP
/// transceivers currently attached to the specified [`PeerConnection`].
pub fn get_transceivers(
    peer_id: u64,
) -> anyhow::Result<Vec<RtcRtpTransceiver>> {
    WEBRTC.lock().unwrap().get_transceivers(peer_id)
}

/// Changes the preferred `direction` of the specified [`RtcRtpTransceiver`].
pub fn set_transceiver_direction(
    peer_id: u64,
    transceiver_index: u32,
    direction: RtpTransceiverDirection,
) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_transceiver_direction(
        peer_id,
        transceiver_index,
        direction,
    )
}

/// Changes the receive direction of the specified [`RtcRtpTransceiver`].
pub fn set_transceiver_recv(
    peer_id: u64,
    transceiver_index: u32,
    recv: bool,
) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_transceiver_recv(
        peer_id,
        transceiver_index,
        recv,
    )
}

/// Changes the send direction of the specified [`RtcRtpTransceiver`].
pub fn set_transceiver_send(
    peer_id: u64,
    transceiver_index: u32,
    send: bool,
) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_transceiver_send(
        peer_id,
        transceiver_index,
        send,
    )
}

/// Returns the [negotiated media ID (mid)][1] of the specified
/// [`RtcRtpTransceiver`].
///
/// [1]: https://w3.org/TR/webrtc#dfn-media-stream-identification-tag
pub fn get_transceiver_mid(
    peer_id: u64,
    transceiver_index: u32,
) -> anyhow::Result<Option<String>> {
    WEBRTC
        .lock()
        .unwrap()
        .get_transceiver_mid(peer_id, transceiver_index)
}

/// Returns the preferred direction of the specified [`RtcRtpTransceiver`].
pub fn get_transceiver_direction(
    peer_id: u64,
    transceiver_index: u32,
) -> anyhow::Result<RtpTransceiverDirection> {
    WEBRTC
        .lock()
        .unwrap()
        .get_transceiver_direction(peer_id, transceiver_index)
        .map(Into::into)
}

/// Returns [`RtcStats`] of the [`PeerConnection`] by its ID.
pub fn get_peer_stats(peer_id: u64) -> anyhow::Result<Vec<RtcStats>> {
    let (tx, rx) = mpsc::channel();

    WEBRTC.lock().unwrap().get_stats(peer_id, tx)?;
    let report = rx.recv_timeout(RX_TIMEOUT)?;

    Ok(report
        .get_stats()?
        .into_iter()
        .map(RtcStats::from)
        .collect())
}

/// Irreversibly marks the specified [`RtcRtpTransceiver`] as stopping, unless
/// it's already stopped.
///
/// This will immediately cause the transceiver's sender to no longer send, and
/// its receiver to no longer receive.
pub fn stop_transceiver(
    peer_id: u64,
    transceiver_index: u32,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .stop_transceiver(peer_id, transceiver_index)
}

/// Replaces the specified [`AudioTrack`] (or [`VideoTrack`]) on the
/// [`sys::Transceiver`]'s `sender`.
pub fn sender_replace_track(
    peer_id: u64,
    transceiver_index: u32,
    track_id: Option<String>,
) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().sender_replace_track(
        peer_id,
        transceiver_index,
        track_id,
    )
}

/// Adds the new ICE `candidate` to the given [`PeerConnection`].
#[allow(clippy::needless_pass_by_value)]
pub fn add_ice_candidate(
    peer_id: u64,
    candidate: String,
    sdp_mid: String,
    sdp_mline_index: i32,
) -> anyhow::Result<()> {
    let (tx, rx) = mpsc::channel();

    WEBRTC.lock().unwrap().add_ice_candidate(
        peer_id,
        candidate,
        sdp_mid,
        sdp_mline_index,
        tx,
    )?;

    rx.recv_timeout(RX_TIMEOUT)?
}

/// Tells the [`PeerConnection`] that ICE should be restarted.
pub fn restart_ice(peer_id: u64) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().restart_ice(peer_id)
}

/// Closes the [`PeerConnection`].
pub fn dispose_peer_connection(peer_id: u64) {
    WEBRTC.lock().unwrap().dispose_peer_connection(peer_id);
}

/// Creates a [`MediaStream`] with tracks according to provided
/// [`MediaStreamConstraints`].
pub fn get_media(constraints: MediaStreamConstraints) -> GetMediaResult {
    match WEBRTC.lock().unwrap().get_media(constraints) {
        Ok(tracks) => GetMediaResult::Ok(tracks),
        Err(err) => GetMediaResult::Err(err),
    }
}

/// Sets the specified `audio playout` device.
pub fn set_audio_playout_device(device_id: String) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_audio_playout_device(device_id)
}

/// Indicates whether the microphone is available to set volume.
pub fn microphone_volume_is_available() -> anyhow::Result<bool> {
    WEBRTC.lock().unwrap().microphone_volume_is_available()
}

/// Sets the microphone system volume according to the specified `level` in
/// percents.
///
/// Valid values range is `[0; 100]`.
pub fn set_microphone_volume(level: u8) -> anyhow::Result<()> {
    WEBRTC.lock().unwrap().set_microphone_volume(level)
}

/// Returns the current level of the microphone volume in `[0; 100]` range.
pub fn microphone_volume() -> anyhow::Result<u32> {
    WEBRTC.lock().unwrap().microphone_volume()
}

/// Disposes the specified [`MediaStreamTrack`].
pub fn dispose_track(track_id: String, kind: MediaType) {
    WEBRTC.lock().unwrap().dispose_track(track_id, kind);
}

/// Returns the [readyState][0] property of the [`MediaStreamTrack`] by its ID
/// and [`MediaType`].
///
/// [0]: https://w3.org/TR/mediacapture-streams#dfn-readystate
pub fn track_state(
    track_id: String,
    kind: MediaType,
) -> anyhow::Result<TrackState> {
    WEBRTC.lock().unwrap().track_state(track_id, kind)
}

/// Changes the [enabled][1] property of the [`MediaStreamTrack`] by its ID and
/// [`MediaType`].
///
/// [1]: https://w3.org/TR/mediacapture-streams#track-enabled
pub fn set_track_enabled(
    track_id: String,
    kind: MediaType,
    enabled: bool,
) -> anyhow::Result<()> {
    WEBRTC
        .lock()
        .unwrap()
        .set_track_enabled(track_id, kind, enabled)
}

/// Clones the specified [`MediaStreamTrack`].
pub fn clone_track(
    track_id: String,
    kind: MediaType,
) -> anyhow::Result<MediaStreamTrack> {
    WEBRTC.lock().unwrap().clone_track(track_id, kind)
}

/// Creates a new [`VideoSink`] attached to the specified video track.
///
/// `callback_ptr` argument should be a pointer to an [`UniquePtr`] pointing to
/// an [`OnFrameCallbackInterface`].
pub fn create_video_sink(
    sink_id: i64,
    track_id: String,
    callback_ptr: u64,
) -> anyhow::Result<()> {
    let handler = FrameHandler::new(callback_ptr as _);

    WEBRTC
        .lock()
        .unwrap()
        .create_video_sink(sink_id, track_id, handler)
}

/// Destroys the [`VideoSink`] by the provided ID.
pub fn dispose_video_sink(sink_id: i64) {
    WEBRTC.lock().unwrap().dispose_video_sink(sink_id);
}
