//
// Created by relmay on 5/24/23.
//

#include "openal_adm.h"
#include <rtc_base/logging.h>
#include <rtc_base/thread.h>
#include "rtc_base/thread_annotations.h"

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

AudioDeviceOpenAL::AudioDeviceOpenAL(
    webrtc::TaskQueueFactory *taskQueueFactory)
    : _audioDeviceBuffer(taskQueueFactory) {
  _audioDeviceBuffer.SetRecordingSampleRate(kRecordingFrequency);
  _audioDeviceBuffer.SetRecordingChannels(kRecordingChannels);
}


AudioDeviceOpenAL::~AudioDeviceOpenAL() {

}

int32_t AudioDeviceOpenAL::Init() {
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

  _initialized = true;
  return 0;
}
