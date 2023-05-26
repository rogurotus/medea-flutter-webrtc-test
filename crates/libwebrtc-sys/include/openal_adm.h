//
// Created by relmay on 5/24/23.
//

#ifndef FLUTTER_WEBRTC_OPENAL_ADM_H
#define FLUTTER_WEBRTC_OPENAL_ADM_H

#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/audio_device_buffer.h>
#include "modules/audio_device/audio_device_impl.h"
#include "modules/audio_device/audio_device_generic.h"
#include "modules/audio_device/include/audio_device_defines.h"

#include <al.h>
#include <alc.h>
#include <atomic>
#include "rtc_base/platform_thread.h"
#include <rtc_base/thread.h>

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

} // namespace details

// Thread-safe.
time now();
profile_time profile();

// Returns true if some adjustment was made.
bool adjust_time();

} // namespace crl

class AudioDeviceOpenAL : public webrtc::AudioDeviceModule {
 public:
  AudioDeviceOpenAL(webrtc::TaskQueueFactory *taskQueueFactory);
  ~AudioDeviceOpenAL();
  int32_t Init();

 private:
  bool _initialized = false;
  webrtc::AudioDeviceBuffer _audioDeviceBuffer;
};

#endif  // FLUTTER_WEBRTC_OPENAL_ADM_H
