
#pragma once
#include "custom_audio.h"

class AudioSourceInfo {
  public:
  AudioSourceInfo(int id, std::string title);
  int GetId() const;
  std::string GetTitle() const;
  private:
  int id;
  std::string title;
};

class SystemModuleInterface;

class SystemSource : public AudioSource {
 public:
  SystemSource(SystemModuleInterface* module);
  ~SystemSource();

 private:
  SystemModuleInterface* module;
  static int sources_num;
};

class SystemModuleInterface {
public:

// Initialization and terminate.
virtual bool Init() = 0;
virtual int32_t Terminate() = 0;
virtual rtc::scoped_refptr<AudioSource> CreateSource() = 0;
virtual void ResetSource() = 0;

// Settings.
virtual void SetRecordingSource(int id) = 0;
virtual void SetSystemAudioLevel(float level) = 0;
virtual float GetSystemAudioLevel() const = 0;
virtual int32_t StopRecording() = 0;
virtual int32_t StartRecording() = 0;
virtual int32_t RecordingChannels() = 0;

// Enumerate system audio outputs.
virtual std::vector<AudioSourceInfo> EnumerateSystemSource() const = 0;

// System source.
rtc::scoped_refptr<SystemSource> source = nullptr;
};

