#pragma once

#define WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE 1
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include "api/task_queue/task_queue_factory.h"
#include "modules/audio_device/audio_device_impl.h"

#include <memory>

#include "api/media_stream_interface.h"
#include "api/sequence_checker.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/audio_device_generic.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_defines.h"

#include "rtc_base/event.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread.h"
#include "rtc_base/thread_annotations.h"

#include "modules/audio_mixer/audio_mixer_impl.h"

#include "api/audio/audio_mixer.h"

#include "api/audio/audio_frame.h"

#include "custom_audio.h"

#if defined(WEBRTC_USE_X11)
#include <X11/Xlib.h>
#endif

#include "microphone_module.h"
#if defined(WEBRTC_LINUX)
#include "linux_microphone_module.h"
#include "modules/audio_device/linux/audio_mixer_manager_pulse_linux.h"
#include "modules/audio_device/linux/pulseaudiosymboltable_linux.h"
#endif

#if defined(WEBRTC_WIN)
#include "windows_microphone_module.h"
#endif

#if defined(WEBRTC_MAC)
#include "macos_microphone_module.h"
#endif

namespace crl {

using time = std::int64_t;
using profile_time = std::int64_t;

namespace details {

using inner_time_type = std::int64_t;
using inner_profile_type = std::int64_t;

void init();

inner_time_type current_value();
time convert(inner_time_type value);

inner_profile_type current_profile_value();
profile_time convert_profile(inner_profile_type);

}  // namespace details

// Thread-safe.
time now();
profile_time profile();

// Returns true if some adjustment was made.
bool adjust_time();

}  // namespace crl

class AudioSourceManager {
 public:
  // Creates a `AudioSource` from a microphone.
  virtual rtc::scoped_refptr<AudioSource> CreateMicrophoneSource() = 0;
  // Creates a `AudioSource` from a system audio.
  virtual rtc::scoped_refptr<AudioSource> CreateSystemSource() = 0;
  // Adds `AudioSource` to `AudioSourceManager`.
  virtual void AddSource(rtc::scoped_refptr<AudioSource> source) = 0;
  // Removes `AudioSource` to `AudioSourceManager`.
  virtual void RemoveSource(rtc::scoped_refptr<AudioSource> source) = 0;
};

class CustomAudioDeviceModule : public webrtc::AudioDeviceModuleImpl,
                                public AudioSourceManager {
 public:
  CustomAudioDeviceModule(AudioLayer audio_layer,
                          webrtc::TaskQueueFactory* task_queue_factory);
  ~CustomAudioDeviceModule();

  static rtc::scoped_refptr<CustomAudioDeviceModule> Create(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  static rtc::scoped_refptr<CustomAudioDeviceModule> CreateForTest(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  // Mixes source and sends on.
  void RecordProcess();

  // Main initializaton and termination.
  int32_t Init() override;
  int32_t Terminate();
  int32_t StartRecording() override;
  int32_t SetRecordingDevice(uint16_t index) override;
  int32_t InitMicrophone() override;
  bool MicrophoneIsInitialized() const override;

  // Microphone volume controls.
  int32_t MicrophoneVolumeIsAvailable(bool* available) override;
  int32_t SetMicrophoneVolume(uint32_t volume) override;
  int32_t MicrophoneVolume(uint32_t* volume) const override;
  int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;
  int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;

  // AudioSourceManager interface.
  rtc::scoped_refptr<AudioSource> CreateMicrophoneSource() override;
  rtc::scoped_refptr<AudioSource> CreateSystemSource() override;
  void AddSource(rtc::scoped_refptr<AudioSource> source) override;
  void RemoveSource(rtc::scoped_refptr<AudioSource> source) override;

  // Microphone mute control.
  int32_t MicrophoneMuteIsAvailable(bool* available) override;
  int32_t SetMicrophoneMute(bool enable) override;
  int32_t MicrophoneMute(bool* enabled) const override;

  // Playout control.
  int16_t PlayoutDevices() override;
  int32_t SetPlayoutDevice(uint16_t index) override;
  int32_t SetPlayoutDevice(WindowsDeviceType device) override;
  int32_t PlayoutDeviceName(uint16_t index,
                            char name[webrtc::kAdmMaxDeviceNameSize],
                            char guid[webrtc::kAdmMaxGuidSize]) override;
  int32_t InitPlayout() override;
  bool PlayoutIsInitialized() const override;
  int32_t StartPlayout() override;
  int32_t StopPlayout() override;
  bool Playing() const override;
  int32_t InitSpeaker() override;
  bool SpeakerIsInitialized() const override;
  int32_t StereoPlayoutIsAvailable(bool* available) const override;
  int32_t SetStereoPlayout(bool enable) override;
  int32_t StereoPlayout(bool* enabled) const override;
  int32_t PlayoutDelay(uint16_t* delayMS) const override;

  int32_t SpeakerVolumeIsAvailable(bool* available) override;
  int32_t SetSpeakerVolume(uint32_t volume) override;
  int32_t SpeakerVolume(uint32_t* volume) const override;
  int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override;
  int32_t MinSpeakerVolume(uint32_t* minVolume) const override;

  int32_t SpeakerMuteIsAvailable(bool* available) override;
  int32_t SetSpeakerMute(bool enable) override;
  int32_t SpeakerMute(bool* enabled) const override;
  int32_t RegisterAudioCallback(webrtc::AudioTransport* audioCallback) override;

 private:
  struct Data;

  // Mixes `AudioSource` to send.
  rtc::scoped_refptr<webrtc::AudioMixerImpl> mixer =
      webrtc::AudioMixerImpl::Create();

  // `AudioSource` for mixing.
  std::vector<rtc::scoped_refptr<AudioSource>> sources;
  std::mutex source_mutex;

  bool _initialized = false;
  std::unique_ptr<Data> _data;

  // Audio capture module.
  std::unique_ptr<MicrophoneModuleInterface> audio_recorder;

  rtc::PlatformThread recordingThread;
  std::condition_variable cv;
  bool quit = false;

 private:
  int restartPlayout();
  void openPlayoutDevice();

  void startPlayingOnThread();
  void ensureThreadStarted();
  void closePlayoutDevice();
  bool validatePlayoutDeviceId();

  void clearProcessedBuffers();
  bool clearProcessedBuffer();

  void unqueueAllBuffers();

  [[nodiscard]] crl::time countExactQueuedMsForLatency(crl::time now,
                                                       bool playing);

  bool processPlayout();

  // NB! closePlayoutDevice should be called after this, so that next time
  // we start playing, we set the thread local context and event callback.
  void stopPlayingOnThread();

  void handleEvent(ALenum eventType,
                   ALuint object,
                   ALuint param,
                   ALsizei length,
                   const ALchar* message);

  void processPlayoutQueued();

  rtc::Thread* _thread = nullptr;
  std::string _playoutDeviceId;
  bool _playoutInitialized = false;
  bool _playoutFailed = false;
  int _playoutChannels = 2;
  crl::time _playoutLatency = 0;
  bool _speakerInitialized = false;
  ALCcontext* _playoutContext = nullptr;
  ALCdevice* _playoutDevice = nullptr;
};
