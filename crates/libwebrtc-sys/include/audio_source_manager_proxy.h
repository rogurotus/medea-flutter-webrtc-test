#pragma once

#include "adm.h"
#include "pc/proxy.h"

class AudioSourceManagerProxy : AudioSourceManager {
  AudioSourceManagerProxy(rtc::Thread* primary_thread,
                          rtc::scoped_refptr<CustomAudioDeviceModule> c)
      : adm(c), primary_thread_(primary_thread) {}

 public:
  static std::unique_ptr<AudioSourceManager> Create(
      rtc::Thread* primary_thread,
      rtc::scoped_refptr<CustomAudioDeviceModule> c) {
    return std::unique_ptr<AudioSourceManager>(
        new AudioSourceManagerProxy(primary_thread, std::move(c)));
  }

  rtc::scoped_refptr<AudioSource> CreateMicrophoneSource() override {
    TRACE_BOILERPLATE(CreateMicrophoneSource);
    webrtc::MethodCall<AudioSourceManager, rtc::scoped_refptr<AudioSource>>
        call(adm.get(), &AudioSourceManager::CreateMicrophoneSource);
    return call.Marshal(primary_thread_);
  };

  rtc::scoped_refptr<AudioSource> CreateSystemSource() override {
    TRACE_BOILERPLATE(CreateSystemSource);
    webrtc::MethodCall<AudioSourceManager, rtc::scoped_refptr<AudioSource>>
        call(adm.get(), &AudioSourceManager::CreateSystemSource);
    return call.Marshal(primary_thread_);
  }

  void AddSource(rtc::scoped_refptr<AudioSource> source) override {
    TRACE_BOILERPLATE(AddSource);
    webrtc::MethodCall<AudioSourceManager, void,
                       rtc::scoped_refptr<AudioSource>>
        call(adm.get(), &AudioSourceManager::AddSource, std::move(source));
    return call.Marshal(primary_thread_);
  }

  void RemoveSource(rtc::scoped_refptr<AudioSource> source) override {
    TRACE_BOILERPLATE(RemoveSource);
    webrtc::MethodCall<AudioSourceManager, void,
                       rtc::scoped_refptr<AudioSource>>
        call(adm.get(), &AudioSourceManager::RemoveSource, std::move(source));
    return call.Marshal(primary_thread_);
  }

 private:
  rtc::scoped_refptr<CustomAudioDeviceModule> adm;
  rtc::Thread* primary_thread_;
};