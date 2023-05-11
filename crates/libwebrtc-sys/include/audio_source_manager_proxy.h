#ifndef AUDIO_SOURCE_MANAGER_PROXY_H
#define AUDIO_SOURCE_MANAGER_PROXY_H

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
  std::vector<AudioSourceInfo> EnumerateSystemSource() const override;
  void AddSource(rtc::scoped_refptr<AudioSource> source) override;
  void RemoveSource(rtc::scoped_refptr<AudioSource> source) override;
  void SetRecordingSource(int id) override;
  void SetSystemAudioVolume(float level) override;
  float GetSystemAudioVolume() const override;
  void SetAudioLevelCallBack(rust::Box<bridge::DynAudioLevelCallback> cb) override;

 private:
  rtc::scoped_refptr<CustomAudioDeviceModule> adm;
  rtc::Thread* primary_thread_;
};
#endif