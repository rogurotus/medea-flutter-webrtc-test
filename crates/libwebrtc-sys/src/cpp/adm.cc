/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <iostream>

#include <chrono>
#include <thread>
#include "adm.h"
#include "api/make_ref_counted.h"
#include "common_audio/wav_file.h"
#include "fake_source.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "modules/audio_device/linux/latebindingsymboltable_linux.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"

// Main initializaton and termination
int32_t ADM::Init() {
  if (webrtc::AudioDeviceModuleImpl::Init() != 0) {
    return -1;
  }
  return audio_recorder.Init();
};

// todo
int32_t ADM::StartRecording() {
  return 0;
};

rtc::scoped_refptr<ADM> ADM::Create(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
  return ADM::CreateForTest(audio_layer, task_queue_factory);
}

int32_t ADM::SetRecordingDevice(uint16_t index) {
  return audio_recorder.SetRecordingDevice(index);
}

int32_t ADM::InitMicrophone() {
  return audio_recorder.InitMicrophone();
}

bool ADM::MicrophoneIsInitialized() const {
  return audio_recorder.MicrophoneIsInitialized();
}

webrtc::AudioMixer::Source* ADM::CreateSystemSource() {
  std::cout << "ADDD" << std::endl;
  auto system = new WavFileReader("./test3.wav", 44100, 1, true);
  auto th = std::thread([=] {
    while (true) {
      system->Capture(&(system->bufffer));
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
  th.detach();
  mixer->AddSource(system);
  return system;
}

webrtc::AudioMixer::Source* ADM::CreateMicroSource() {
  audio_recorder.InitRecording();
  audio_recorder.StartRecording();

  auto microphone = audio_recorder.createSource();
  mixer->AddSource(microphone);
  return microphone;
}

void ADM::AddSource(webrtc::AudioMixer::Source* source) {
  mixer->AddSource(source);
}

void ADM::RemoveSource(webrtc::AudioMixer::Source* source) {
  mixer->RemoveSource(source);
}

int32_t ADM::MicrophoneVolumeIsAvailable(bool* available) {
  return audio_recorder.MicrophoneVolumeIsAvailable(available);
}
int32_t ADM::SetMicrophoneVolume(uint32_t volume) {
  return audio_recorder.SetMicrophoneVolume(volume);
}
int32_t ADM::MicrophoneVolume(uint32_t* volume) const {
  return audio_recorder.MicrophoneVolume(volume);
}
int32_t ADM::MaxMicrophoneVolume(uint32_t* maxVolume) const {
  return audio_recorder.MaxMicrophoneVolume(maxVolume);
}
int32_t ADM::MinMicrophoneVolume(uint32_t* minVolume) const {
  return audio_recorder.MinMicrophoneVolume(minVolume);
}

// Microphone mute control
int32_t ADM::MicrophoneMuteIsAvailable(bool* available) {
  return audio_recorder.MicrophoneMuteIsAvailable(available);
}
int32_t ADM::SetMicrophoneMute(bool enable) {
  return audio_recorder.SetMicrophoneMute(enable);
}
int32_t ADM::MicrophoneMute(bool* enabled) const {
  return audio_recorder.MicrophoneMute(enabled);
}

// static
rtc::scoped_refptr<ADM> ADM::CreateForTest(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
  // The "AudioDeviceModule::kWindowsCoreAudio2" audio layer has its own
  // dedicated factory method which should be used instead.
  if (audio_layer == AudioDeviceModule::kWindowsCoreAudio2) {
    return nullptr;
  }

  // Create the generic reference counted (platform independent) implementation.
  auto audio_device =
      rtc::make_ref_counted<ADM>(audio_layer, task_queue_factory);

  // Ensure that the current platform is supported.
  if (audio_device->CheckPlatform() == -1) {
    return nullptr;
  }

  // Create the platform-dependent implementation.
  if (audio_device->CreatePlatformSpecificObjects() == -1) {
    return nullptr;
  }

  // Ensure that the generic audio buffer can communicate with the platform
  // specific parts.
  if (audio_device->AttachAudioBuffer() == -1) {
    return nullptr;
  }

  //todo
  audio_device->RecordProcess();
  return audio_device;
}

ADM::ADM(AudioLayer audio_layer, webrtc::TaskQueueFactory* task_queue_factory)
    : webrtc::AudioDeviceModuleImpl(audio_layer, task_queue_factory) {
  audio_recorder.cb = GetAudioDeviceBuffer();
}

void ADM::RecordProcess() {
  const auto attributes =
      rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime);
  _ptrThreadRec = rtc::PlatformThread::SpawnJoinable(
      [this] {
        while (true) {
          webrtc::AudioFrame fr;
          mixer->Mix(1, &fr);
          auto cb = GetAudioDeviceBuffer();
          cb->SetRecordingChannels(fr.num_channels());
          cb->SetRecordingSampleRate(fr.sample_rate_hz());
          cb->SetRecordedBuffer(fr.data(), fr.sample_rate_hz() / 100);
          // cb->SetTypingStatus(da->KeyPressed());
          cb->DeliverRecordedData();
        }
      },
      "webrtc_audio_module_rec_thread", attributes);
}
