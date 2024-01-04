package com.instrumentisto.medea_flutter_webrtc

import android.annotation.TargetApi
import android.content.Context
import android.media.AudioManager
import com.instrumentisto.medea_flutter_webrtc.utils.EglUtils
import org.webrtc.DefaultVideoDecoderFactory
import org.webrtc.DefaultVideoEncoderFactory

import org.webrtc.HardwareVideoDecoderFactory
import org.webrtc.HardwareVideoEncoderFactory

import org.webrtc.SoftwareVideoDecoderFactory
import org.webrtc.SoftwareVideoEncoderFactory
import org.webrtc.EglBase
import org.webrtc.PeerConnectionFactory
import org.webrtc.audio.JavaAudioDeviceModule
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build
import android.util.Log

/**
 * Global context of the `flutter_webrtc` library.
 *
 * Used for creating tracks, peers, and performing `getUserMedia` requests.
 *
 * @property context Android [Context] used, for example, for `getUserMedia` requests.
 */
class State(private val context: Context) {
  /** Module for the controlling audio devices in context of `libwebrtc`. */
  private var audioDeviceModule: JavaAudioDeviceModule? = null

  /**
   * Factory for producing `PeerConnection`s and `MediaStreamTrack`s.
   *
   * Will be lazily initialized on the first call of [getPeerConnectionFactory].
   */
  private var factory: PeerConnectionFactory? = null

  init {
    PeerConnectionFactory.initialize(
        PeerConnectionFactory.InitializationOptions.builder(context)
            .setEnableInternalTracer(true)
            .createInitializationOptions())
  }



  /** Initializes a new [factory]. */
  @TargetApi(Build.VERSION_CODES.Q)
  private fun initPeerConnectionFactory() {
    var aa = android.media.MediaCodecList(android.media.MediaCodecList.ALL_CODECS);
    var a = aa.getCodecInfos();
    for (i in a) {
      if (true) {
        Log.e("TEST", i.getName() + " " + i.getCanonicalName() + " " + i.isHardwareAccelerated().toString())
      }
      Log.e("TEST", "\n")
    }


//    var bb = android.media.MediaCodecList.getCodecCount();
//    for (i in 0 until bb) {
//      var info = MediaCodecList.getCodecInfoAt(i);
//
//      if (info.isHardwareAccelerated()) {
//        Log.e("BUG", info.getName())
//        Log.e("1BUG", info.getCanonicalName())
//        Log.e("2BUG", info.isHardwareAccelerated().toString())
//      }
//      Log.e("BUG", "\n")
//    }


//    for (int i = 0; i < MediaCodecList.getCodecInfos(); ++i) {
//      System.err.println("HW 4 ");
//      MediaCodecInfo info = null;
//      try {
//        info = MediaCodecList.getCodecInfoAt(i);
//      }
//    }

    val audioModule =
        JavaAudioDeviceModule.builder(context)
            .setUseHardwareAcousticEchoCanceler(true)
            .setUseHardwareNoiseSuppressor(true)
            .createAudioDeviceModule()
    val eglContext: EglBase.Context = EglUtils.rootEglBaseContext!!
    factory =
        PeerConnectionFactory.builder()
            .setOptions(PeerConnectionFactory.Options())
            .setVideoEncoderFactory(DefaultVideoEncoderFactory(eglContext, true, true))
            .setVideoDecoderFactory(DefaultVideoDecoderFactory(eglContext))
            .setAudioDeviceModule(audioModule)
            .createPeerConnectionFactory()
    audioModule.setSpeakerMute(false)
    audioDeviceModule = audioModule
  }

  /**
   * Initializes the [PeerConnectionFactory] if it wasn't initialized before.
   *
   * @return Current [PeerConnectionFactory] of this [State].
   */
  fun getPeerConnectionFactory(): PeerConnectionFactory {
    if (factory == null) {
      initPeerConnectionFactory()
    }
    return factory!!
  }

  /** @return Android SDK [Context]. */
  fun getAppContext(): Context {
    return context
  }

  /** @return [AudioManager] system service. */
  fun getAudioManager(): AudioManager {
    return context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
  }
}
