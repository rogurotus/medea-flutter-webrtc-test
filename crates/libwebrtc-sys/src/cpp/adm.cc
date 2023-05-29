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
#include "modules/audio_device/include/test_audio_device.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"
#ifdef WEBRTC_WIN
#include "webrtc/win/webrtc_loopback_adm_win.h"
#endif // WEBRTC_WIN

constexpr auto kRecordingFrequency = 48000;
constexpr auto kPlayoutFrequency = 48000;
constexpr auto kRecordingChannels = 1;
constexpr auto kBufferSizeMs = crl::time(10);
constexpr auto kPlayoutPart = (kPlayoutFrequency * kBufferSizeMs + 999)
                              / 1000;
constexpr auto kRecordingPart = (kRecordingFrequency * kBufferSizeMs + 999)
                                / 1000;
constexpr auto kRecordingBufferSize = kRecordingPart * sizeof(int16_t)
                                      * kRecordingChannels;
constexpr auto kRestartAfterEmptyData = 50; // Half a second with no data.
constexpr auto kProcessInterval = crl::time(10);

constexpr auto kBuffersFullCount = 7;
constexpr auto kBuffersKeepReadyCount = 5;

constexpr auto kDefaultRecordingLatency = crl::time(20);
constexpr auto kDefaultPlayoutLatency = crl::time(20);
constexpr auto kQueryExactTimeEach = 20;

constexpr auto kALMaxValues = 6;
auto kAL_EVENT_CALLBACK_FUNCTION_SOFT = ALenum();
auto kAL_EVENT_CALLBACK_USER_PARAM_SOFT = ALenum();
auto kAL_EVENT_TYPE_BUFFER_COMPLETED_SOFT = ALenum();
auto kAL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT = ALenum();
auto kAL_EVENT_TYPE_DISCONNECTED_SOFT = ALenum();
auto kAL_SAMPLE_OFFSET_CLOCK_SOFT = ALenum();
auto kAL_SAMPLE_OFFSET_CLOCK_EXACT_SOFT = ALenum();

auto kALC_DEVICE_LATENCY_SOFT = ALenum();

using AL_INT64_TYPE = std::int64_t;

using ALEVENTPROCSOFT = void(*)(
    ALenum eventType,
    ALuint object,
    ALuint param,
    ALsizei length,
    const ALchar *message,
    void *userParam);
using ALEVENTCALLBACKSOFT = void(*)(
    ALEVENTPROCSOFT callback,
    void *userParam);
using ALCSETTHREADCONTEXT = ALCboolean(*)(ALCcontext *context);
using ALGETSOURCEI64VSOFT = void(*)(
    ALuint source,
    ALenum param,
    AL_INT64_TYPE *values);
using ALCGETINTEGER64VSOFT = void(*)(
    ALCdevice *device,
    ALCenum pname,
    ALsizei size,
    AL_INT64_TYPE *values);

ALEVENTCALLBACKSOFT alEventCallbackSOFT/* = nullptr*/;
ALCSETTHREADCONTEXT alcSetThreadContext/* = nullptr*/;
ALGETSOURCEI64VSOFT alGetSourcei64vSOFT/* = nullptr*/;
ALCGETINTEGER64VSOFT alcGetInteger64vSOFT/* = nullptr*/;

// Main initializaton and termination
int32_t CustomAudioDeviceModule::Init() {
  RTC_LOG(LS_ERROR) << "Initializing AudioDeviceModule 1";
  if (webrtc::AudioDeviceModuleImpl::Init() != 0) {
    return -1;
  }

  if (_initialized) {
    return 0;
  }
  alcSetThreadContext = (ALCSETTHREADCONTEXT)alcGetProcAddress(
      nullptr,
      "alcSetThreadContext");
  if (!alcSetThreadContext) {
    return -1;
  }
  alEventCallbackSOFT = (ALEVENTCALLBACKSOFT)alcGetProcAddress(
      nullptr,
      "alEventCallbackSOFT");

  alGetSourcei64vSOFT = (ALGETSOURCEI64VSOFT)alcGetProcAddress(
      nullptr,
      "alGetSourcei64vSOFT");

  alcGetInteger64vSOFT = (ALCGETINTEGER64VSOFT)alcGetProcAddress(
      nullptr,
      "alcGetInteger64vSOFT");

#define RESOLVE_ENUM(ENUM) k##ENUM = alcGetEnumValue(nullptr, #ENUM)
  RESOLVE_ENUM(AL_EVENT_CALLBACK_FUNCTION_SOFT);
  RESOLVE_ENUM(AL_EVENT_CALLBACK_FUNCTION_SOFT);
  RESOLVE_ENUM(AL_EVENT_CALLBACK_USER_PARAM_SOFT);
  RESOLVE_ENUM(AL_EVENT_TYPE_BUFFER_COMPLETED_SOFT);
  RESOLVE_ENUM(AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT);
  RESOLVE_ENUM(AL_EVENT_TYPE_DISCONNECTED_SOFT);
  RESOLVE_ENUM(AL_SAMPLE_OFFSET_CLOCK_SOFT);
  RESOLVE_ENUM(AL_SAMPLE_OFFSET_CLOCK_EXACT_SOFT);
  RESOLVE_ENUM(ALC_DEVICE_LATENCY_SOFT);
#undef RESOLVE_ENUM
  RTC_LOG(LS_ERROR) << "Initializing AudioDeviceModule 2";

  _initialized = true;
  RTC_LOG(LS_ERROR) << "Initializing AudioDeviceModule 3";

  return audio_recorder->Init();
};

int32_t CustomAudioDeviceModule::Terminate() {
  quit = true;
  return 0;
}

CustomAudioDeviceModule::~CustomAudioDeviceModule() {
  Terminate();
}

rtc::scoped_refptr<CustomAudioDeviceModule> CustomAudioDeviceModule::Create(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
  return CustomAudioDeviceModule::CreateForTest(audio_layer,
                                                task_queue_factory);
}

int32_t CustomAudioDeviceModule::SetRecordingDevice(uint16_t index) {
  return audio_recorder->SetRecordingDevice(index);
}

int32_t CustomAudioDeviceModule::InitMicrophone() {
  return audio_recorder->InitMicrophone();
}

bool CustomAudioDeviceModule::MicrophoneIsInitialized() const {
  return audio_recorder->MicrophoneIsInitialized();
}

rtc::scoped_refptr<AudioSource> CustomAudioDeviceModule::CreateSystemSource() {
  // TODO implement system sound capture.
  return nullptr;
}

rtc::scoped_refptr<AudioSource>
CustomAudioDeviceModule::CreateMicrophoneSource() {
  auto microphone = audio_recorder->CreateSource();
  return microphone;
}

void CustomAudioDeviceModule::AddSource(
    rtc::scoped_refptr<AudioSource> source) {
  {
    std::unique_lock<std::mutex> lock(source_mutex);
    sources.push_back(source);
  }
  cv.notify_all();
  mixer->AddSource(source.get());
}

void CustomAudioDeviceModule::RemoveSource(
    rtc::scoped_refptr<AudioSource> source) {
  {
    std::unique_lock<std::mutex> lock(source_mutex);
    for (int i = 0; i < sources.size(); ++i) {
      if (sources[i] == source) {
        sources.erase(sources.begin() + i);
        break;
      }
    }
  }
  mixer->RemoveSource(source.get());
}

// Microphone mute control
int32_t CustomAudioDeviceModule::MicrophoneVolumeIsAvailable(bool* available) {
  return audio_recorder->MicrophoneVolumeIsAvailable(available);
}
int32_t CustomAudioDeviceModule::SetMicrophoneVolume(uint32_t volume) {
  return audio_recorder->SetMicrophoneVolume(volume);
}
int32_t CustomAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const {
  return audio_recorder->MicrophoneVolume(volume);
}
int32_t CustomAudioDeviceModule::MaxMicrophoneVolume(
    uint32_t* maxVolume) const {
  return audio_recorder->MaxMicrophoneVolume(maxVolume);
}
int32_t CustomAudioDeviceModule::MinMicrophoneVolume(
    uint32_t* minVolume) const {
  return audio_recorder->MinMicrophoneVolume(minVolume);
}
int32_t CustomAudioDeviceModule::MicrophoneMuteIsAvailable(bool* available) {
  return audio_recorder->MicrophoneMuteIsAvailable(available);
}
int32_t CustomAudioDeviceModule::SetMicrophoneMute(bool enable) {
  return audio_recorder->SetMicrophoneMute(enable);
}
int32_t CustomAudioDeviceModule::MicrophoneMute(bool* enabled) const {
  return audio_recorder->MicrophoneMute(enabled);
}

// Audio device module delegates StartRecording to `audio_recorder`.
int32_t CustomAudioDeviceModule::StartRecording() {
  return 0;
}

rtc::scoped_refptr<CustomAudioDeviceModule>
CustomAudioDeviceModule::CreateForTest(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
  // The "AudioDeviceModule::kWindowsCoreAudio2" audio layer has its own
  // dedicated factory method which should be used instead.
  if (audio_layer == AudioDeviceModule::kWindowsCoreAudio2) {
    return nullptr;
  }

  // Create the generic reference counted (platform independent) implementation.
  auto audio_device = rtc::make_ref_counted<CustomAudioDeviceModule>(
      audio_layer, task_queue_factory);

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

  audio_device->RecordProcess();

  return audio_device;
}

CustomAudioDeviceModule::CustomAudioDeviceModule(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory)
    : webrtc::AudioDeviceModuleImpl(audio_layer, task_queue_factory),
      _audioDeviceBuffer(task_queue_factory),
      audio_recorder(std::move(std::unique_ptr<MicrophoneModuleInterface>(new MicrophoneModule()))) {
  _audioDeviceBuffer.SetRecordingSampleRate(kRecordingFrequency);
  _audioDeviceBuffer.SetRecordingChannels(kRecordingChannels);
}

void CustomAudioDeviceModule::RecordProcess() {
  const auto attributes =
      rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime);
  ptrThreadRec = rtc::PlatformThread::SpawnJoinable(
      [this] {
        webrtc::AudioFrame frame;
        auto cb = GetAudioDeviceBuffer();
        while (!quit) {
          {
            std::unique_lock<std::mutex> lock(source_mutex);
            cv.wait(lock, [&]() { return sources.size() > 0; });
          }

          mixer->Mix(audio_recorder->RecordingChannels(), &frame);
          cb->SetRecordedBuffer(frame.data(), frame.sample_rate_hz() / 100);
          cb->DeliverRecordedData();
        }
      },
      "audio_device_module_rec_thread", attributes);
}

template <typename Callback>
void EnumerateDevices(ALCenum specifier, Callback &&callback) {
  auto devices = alcGetString(nullptr, specifier);
  while (*devices != 0) {
    callback(devices);
    while (*devices != 0) {
      ++devices;
    }
    ++devices;
  }
}

[[nodiscard]] int DevicesCount(ALCenum specifier) {
  auto result = 0;
  EnumerateDevices(specifier, [&](const char *device) {
    ++result;
  });
  return result;
}

[[nodiscard]] std::string ComputeDefaultDeviceId(ALCenum specifier) {
    const auto device = alcGetString(nullptr, specifier);
    return device ? std::string(device) : std::string();
}

[[nodiscard]] int DeviceName(
    ALCenum specifier,
    int index,
    std::string *name,
    std::string *guid) {
  EnumerateDevices(specifier, [&](const char *device) {
    if (index < 0) {
      return;
    } else if (index > 0) {
      --index;
      return;
    }

    auto string = std::string(device);
    if (name) {
      if (guid) {
        *guid = string;
      }
      const auto prefix = std::string("OpenAL Soft on ");
      if (string.rfind(prefix, 0) == 0) {
        string = string.substr(prefix.size());
      }
      *name = std::move(string);
    } else if (guid) {
      *guid = std::move(string);
    }
    index = -1;
  });
  return (index > 0) ? -1 : 0;
}

void SetStringToArray(const std::string &string, char *array, int size) {
  const auto length = std::min(int(string.size()), size - 1);
  if (length > 0) {
    memcpy(array, string.data(), length);
  }
  array[length] = 0;
}

[[nodiscard]] int DeviceName(
    ALCenum specifier,
    int index,
    char name[webrtc::kAdmMaxDeviceNameSize],
    char guid[webrtc::kAdmMaxGuidSize]) {
    auto sname = std::string();
    auto sguid = std::string();
    const auto result = DeviceName(specifier, index, &sname, &sguid);
    if (result) {
      return result;
    }
    SetStringToArray(sname, name, webrtc::kAdmMaxDeviceNameSize);
    SetStringToArray(sguid, guid, webrtc::kAdmMaxGuidSize);
  return 0;
}

int32_t CustomAudioDeviceModule::SetPlayoutDevice(uint16_t index) {
  const auto result = DeviceName(
      ALC_ALL_DEVICES_SPECIFIER,
      index,
      nullptr,
      &_playoutDeviceId);
  return result ? result : restartPlayout();
}

int32_t CustomAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType /*device*/) {
  _playoutDeviceId = ComputeDefaultDeviceId(ALC_DEFAULT_DEVICE_SPECIFIER);
  return _playoutDeviceId.empty() ? -1 : restartPlayout();
}

// TODO:
int CustomAudioDeviceModule::restartPlayout() {
//  if (!_data || !_data->playing) {
//    return 0;
//  }
//  stopPlayingOnThread();
//  closePlayoutDevice();
//  if (!validatePlayoutDeviceId()) {
//    sync([&] {
//      _data->playing = true;
//      _playoutFailed = true;
//    });
//    return 0;
//  }
//  _playoutFailed = false;
////  openPlayoutDevice();
////  startPlayingOnThread();
  return 0;
}

int16_t CustomAudioDeviceModule::PlayoutDevices() {
  return DevicesCount(ALC_ALL_DEVICES_SPECIFIER);
}

int32_t CustomAudioDeviceModule::PlayoutDeviceName(
    uint16_t index,
    char name[webrtc::kAdmMaxDeviceNameSize],
    char guid[webrtc::kAdmMaxGuidSize]) {
  return DeviceName(ALC_ALL_DEVICES_SPECIFIER, index, name, guid);
}

// TODO:
int32_t CustomAudioDeviceModule::InitPlayout() {
  if (!_initialized) {
    return -1;
  } else if (_playoutInitialized) {
    return 0;
  }
  _playoutInitialized = true;
//  ensureThreadStarted();
//  openPlayoutDevice();
  return 0;
}

bool CustomAudioDeviceModule::PlayoutIsInitialized() const {
  return _playoutInitialized;
}

// TODO:
int32_t CustomAudioDeviceModule::StartPlayout() {
  if (!_playoutInitialized) {
    return -1;
  } else if (Playing()) {
    return 0;
  }
  if (_playoutFailed) {
    _playoutFailed = false;
//    TODO: openPlayoutDevice();
  }
  _audioDeviceBuffer.SetPlayoutSampleRate(kPlayoutFrequency);
  _audioDeviceBuffer.SetPlayoutChannels(_playoutChannels);
  _audioDeviceBuffer.StartPlayout();
//  TODO: startPlayingOnThread();
  return 0;
}

// TODO:
int32_t CustomAudioDeviceModule::StopPlayout() {
//  if (_data) {
//    stopPlayingOnThread();
    _audioDeviceBuffer.StopPlayout();
//    if (!_data->recording) {
//      _data->thread.quit();
//      _data->thread.wait();
//      _data = nullptr;
//    }
//  }
//  closePlayoutDevice();
  _playoutInitialized = false;
  return 0;
}

// TODO
bool CustomAudioDeviceModule::Playing() const {
  return false;
//  return _data && _data->playing;
}

int32_t CustomAudioDeviceModule::InitSpeaker() {
  _speakerInitialized = true;
  return 0;
}

bool CustomAudioDeviceModule::SpeakerIsInitialized() const {
  return _speakerInitialized;
}

int32_t CustomAudioDeviceModule::StereoPlayoutIsAvailable(bool *available) const {
  if (available) {
    *available = true;
  }
  return 0;
}

int32_t CustomAudioDeviceModule::SetStereoPlayout(bool enable) {
  if (Playing()) {
    return -1;
  }
  _playoutChannels = enable ? 2 : 1;
  return 0;
}

int32_t CustomAudioDeviceModule::StereoPlayout(bool *enabled) const {
  if (enabled) {
    *enabled = (_playoutChannels == 2);
  }
  return 0;
}

int32_t CustomAudioDeviceModule::PlayoutDelay(uint16_t *delayMS) const {
  if (delayMS) {
    *delayMS = 0;
  }
  return 0;
}

int32_t CustomAudioDeviceModule::SpeakerVolumeIsAvailable(bool *available) {
  if (available) {
    *available = false;
  }
  return 0;
}

int32_t CustomAudioDeviceModule::SetSpeakerVolume(uint32_t volume) {
  return -1;
}

int32_t CustomAudioDeviceModule::SpeakerVolume(uint32_t *volume) const {
  return -1;
}

int32_t CustomAudioDeviceModule::MaxSpeakerVolume(uint32_t *maxVolume) const {
  return -1;
}

int32_t CustomAudioDeviceModule::MinSpeakerVolume(uint32_t *minVolume) const {
  return -1;
}

int32_t CustomAudioDeviceModule::SpeakerMuteIsAvailable(bool *available) {
  if (available) {
    *available = false;
  }
  return 0;
}

int32_t CustomAudioDeviceModule::SetSpeakerMute(bool enable) {
  return -1;
}

int32_t CustomAudioDeviceModule::SpeakerMute(bool *enabled) const {
  if (enabled) {
    *enabled = false;
  }
  return 0;
}
