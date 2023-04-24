
#pragma once
#include "custom_audio.h"


// Audio source info.
class AudioSourceInfo {
  public:
  AudioSourceInfo(int id, std::string title) : id(id), title(title) {}
  // Returns audio source id.
  int GetId() const {return id;}
  // Returns audio source title.
  std::string GetTitle() const {return title;}
  private:
  // Audio source id.
  int id;
  // Audio source title.
  std::string title;
};

class SystemModuleInterface;

// System audio source.
class SystemSource : public AudioSource {
 public:
  SystemSource(SystemModuleInterface* module);
  ~SystemSource();

 private:
  // Audio system capture module.
  SystemModuleInterface* module;
  // Used to control the module.
  static int sources_num;
};


// Interface to provide system audio capture.
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

