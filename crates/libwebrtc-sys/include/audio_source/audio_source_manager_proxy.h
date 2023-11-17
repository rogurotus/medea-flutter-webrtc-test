#pragma once

#include "adm.h"
#include "pc/proxy.h"

class AudioSourceManagerProxy : AudioSourceManager {
  AudioSourceManagerProxy(rtc::Thread* primary_thread,
                          AudioSourceManager* source_manager)
      : source_manager(source_manager), primary_thread_(primary_thread) {}

 public:
  static std::unique_ptr<AudioSourceManager> Create(
      rtc::Thread* primary_thread,
      AudioSourceManager* source_manager) {
    return std::unique_ptr<AudioSourceManager>(
        new AudioSourceManagerProxy(primary_thread, source_manager));
  }

  rtc::scoped_refptr<AudioSource> CreateMicrophoneSource() {
    TRACE_BOILERPLATE(CreateMicrophoneSource);
    webrtc::MethodCall<AudioSourceManager, rtc::scoped_refptr<AudioSource>>
        call(source_manager, &AudioSourceManager::CreateMicrophoneSource);
    return call.Marshal(primary_thread_);
  };

  rtc::scoped_refptr<AudioSource> CreateSystemSource() {
    TRACE_BOILERPLATE(CreateSystemSource);
    webrtc::MethodCall<AudioSourceManager, rtc::scoped_refptr<AudioSource>>
        call(source_manager, &AudioSourceManager::CreateSystemSource);
    return call.Marshal(primary_thread_);
  }

  void AddSource(rtc::scoped_refptr<AudioSource> source) {
    TRACE_BOILERPLATE(AddSource);
    webrtc::MethodCall<AudioSourceManager, void,
                       rtc::scoped_refptr<AudioSource>>
        call(source_manager, &AudioSourceManager::AddSource, std::move(source));
    return call.Marshal(primary_thread_);
  }

  void RemoveSource(rtc::scoped_refptr<AudioSource> source) {
    TRACE_BOILERPLATE(RemoveSource);
    webrtc::MethodCall<AudioSourceManager, void,
                       rtc::scoped_refptr<AudioSource>>
        call(source_manager, &AudioSourceManager::RemoveSource, std::move(source));
    call.Marshal(primary_thread_);
    return;
  }

 private:
  AudioSourceManager* source_manager;
  rtc::Thread* primary_thread_;
};