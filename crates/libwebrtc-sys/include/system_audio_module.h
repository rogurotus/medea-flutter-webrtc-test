
#pragma once
#include "custom_audio.h"


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
virtual int32_t Init() = 0;
virtual int32_t Terminate() = 0;

// System control.
// virtual int32_t SetSystemMute(bool enable) = 0;
// virtual int32_t SystemMute(bool* enabled) const = 0;
// virtual bool SystemIsInitialized () = 0;
// virtual int32_t SetSystemVolume(uint32_t volume) = 0;
// virtual int32_t SystemVolume(uint32_t* volume) const = 0;

// Settings.
virtual int32_t SetRecordingDevice(uint16_t index) = 0;

// enumerate outputs
virtual int32_t EnumerateDevice(uint16_t index) = 0;

// System source.
virtual rtc::scoped_refptr<AudioSource> CreateSource() = 0;
virtual void ResetSource() = 0;
virtual int32_t StopRecording() = 0;
virtual int32_t StartRecording() = 0;
virtual int32_t RecordingChannels() = 0;

rtc::scoped_refptr<SystemSource> source = nullptr;
};

