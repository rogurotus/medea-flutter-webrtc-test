#pragma once

#include "system_audio_module.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/event.h"
#include "rtc_base/synchronization/mutex.h"
#include <base/RTCMacros.h>
//#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include "mac_screen_capture_delegate.h"

RTC_FWD_DECL_OBJC_CLASS(ScreenCaptureDelegate);

class SystemModule : public SystemModuleInterface {
    public: 
    SystemModule();
    ~SystemModule();

    // Initialization and terminate.
    bool Init();
    int32_t Terminate();
    rtc::scoped_refptr<AudioSource> CreateSource();
    void ResetSource();

    // Settings.
    void SetRecordingSource(int id);
    void SetSystemAudioLevel(float level);
    float GetSystemAudioLevel() const;
    int32_t StopRecording();
    int32_t StartRecording();
    int32_t RecordingChannels();

    // Enumerate system audio outputs.
    std::vector<AudioSourceInfo> EnumerateSystemSource() const;

    ScreenCaptureDelegate* screenCaptureDelegate;
};
