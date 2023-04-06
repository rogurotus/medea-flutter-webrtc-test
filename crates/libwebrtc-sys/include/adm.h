#pragma once


#define WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE 1
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
#include "modules/audio_device/linux/audio_mixer_manager_pulse_linux.h"
#include "modules/audio_device/linux/pulseaudiosymboltable_linux.h"
#include "rtc_base/event.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"

#include "modules/audio_mixer/audio_mixer_impl.h"

#include "api/audio/audio_mixer.h"

#include "api/audio/audio_frame.h"

#include "custom_audio.h"

#if defined(WEBRTC_USE_X11)
#include <X11/Xlib.h>
#endif

#include "linux_micro.h"

class SourceManager {
  public:
  virtual rtc::scoped_refptr<CustomAudioSource> CreateMicroSource() = 0;
  virtual rtc::scoped_refptr<CustomAudioSource>  CreateSystemSource() = 0;
  virtual void AddSource(rtc::scoped_refptr<CustomAudioSource>  source) = 0;
  virtual void RemoveSource(rtc::scoped_refptr<CustomAudioSource>  source) = 0;
};


class ADM : public webrtc::AudioDeviceModuleImpl, public SourceManager {
 public:
  ADM(AudioLayer audio_layer, webrtc::TaskQueueFactory* task_queue_factory);
  int32_t StartRecording() override;

  static rtc::scoped_refptr<ADM> Create(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  static rtc::scoped_refptr<ADM> CreateForTest(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  //todo
  void RecordProcess();

  // Main initializaton and termination
  int32_t Init() override;
  int32_t SetRecordingDevice(uint16_t index) override ;
  int32_t InitMicrophone() override;
  bool MicrophoneIsInitialized() const override;

  // Microphone volume controls
  int32_t MicrophoneVolumeIsAvailable(bool* available)  override;
  int32_t SetMicrophoneVolume(uint32_t volume) override;
  int32_t MicrophoneVolume(uint32_t* volume) const override;
  int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;
  int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;

  rtc::scoped_refptr<CustomAudioSource> CreateMicroSource() override;
  rtc::scoped_refptr<CustomAudioSource>  CreateSystemSource() override;
  void AddSource(rtc::scoped_refptr<CustomAudioSource>  source) override;
  void RemoveSource(rtc::scoped_refptr<CustomAudioSource>  source) override;

  // Microphone mute control
  int32_t MicrophoneMuteIsAvailable(bool* available) override;
  int32_t SetMicrophoneMute(bool enable) override;
  int32_t MicrophoneMute(bool* enabled) const override;

  rtc::scoped_refptr<webrtc::AudioMixerImpl> mixer = webrtc::AudioMixerImpl::Create();
  std::vector<rtc::scoped_refptr<CustomAudioSource>> sources;
  
  MicrophoneModule audio_recorder;
  rtc::PlatformThread _ptrThreadRec;
};