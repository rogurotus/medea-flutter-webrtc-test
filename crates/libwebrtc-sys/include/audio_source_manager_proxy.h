#pragma once

#include "adm.h"
#include "pc/proxy.h"

class AudioSourceManagerProxy : AudioSourceManager {
  AudioSourceManagerProxy(rtc::Thread* primary_thread,
                          rtc::scoped_refptr<CustomAudioDeviceModule> c);
 public:
  static std::unique_ptr<AudioSourceManager> Create(
      rtc::Thread* primary_thread,
      rtc::scoped_refptr<CustomAudioDeviceModule> c);
  rtc::scoped_refptr<AudioSource> CreateMicrophoneSource() override;
  rtc::scoped_refptr<AudioSource> CreateSystemSource() override;
  std::unique_ptr<std::vector<AudioSourceInfo>> EnumerateWindows() const override;
  void AddSource(rtc::scoped_refptr<AudioSource> source) override;
  void RemoveSource(rtc::scoped_refptr<AudioSource> source) override;
  void SetRecordingSource(int id) override;
 private:
  rtc::scoped_refptr<CustomAudioDeviceModule> adm;
  rtc::Thread* primary_thread_;
};