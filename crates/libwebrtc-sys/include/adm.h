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

#if defined(WEBRTC_USE_X11)
#include <X11/Xlib.h>
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

class OpenALPLayoutADM : public webrtc::AudioDeviceModuleImpl {
 public:
  OpenALPLayoutADM(AudioLayer audio_layer,
                          webrtc::TaskQueueFactory* task_queue_factory);
  ~OpenALPLayoutADM();

  static rtc::scoped_refptr<OpenALPLayoutADM> Create(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  static rtc::scoped_refptr<OpenALPLayoutADM> CreateForTest(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  // Main initializaton and termination.
  int32_t Init() override;

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

  bool _initialized = false;
  std::unique_ptr<Data> _data;

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
