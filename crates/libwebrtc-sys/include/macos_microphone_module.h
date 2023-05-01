#pragma once

#include "custom_audio.h"
#include "microphone_module.h"
#include "modules/audio_device/mac/audio_device_mac.h"
#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/semaphore.h>
#include "rtc_base/logging.h"

class MicrophoneModule : public MicrophoneModuleInterface {
 public:
  MicrophoneModule();
  ~MicrophoneModule();

  // MicrophoneModuleInterface

  // Initialization and terminate.
  int32_t Init();
  int32_t Terminate();
  int32_t InitMicrophone();

  // Microphone control.
  int32_t MicrophoneMuteIsAvailable(bool* available);
  int32_t SetMicrophoneMute(bool enable);
  int32_t MicrophoneMute(bool* enabled) const;
  bool MicrophoneIsInitialized();
  int32_t MicrophoneVolumeIsAvailable(bool* available);
  int32_t SetMicrophoneVolume(uint32_t volume);
  int32_t MicrophoneVolume(uint32_t* volume) const;
  int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const;
  int32_t MinMicrophoneVolume(uint32_t* minVolume) const;

  // Settings.
  int32_t SetRecordingDevice(uint16_t index);

  // Microphone source.
  rtc::scoped_refptr<AudioSource> CreateSource();
  void ResetSource();
  int32_t StopRecording();
  int32_t StartRecording();
  int32_t RecordingChannels();

  // END MicrophoneModuleInterface

 private:
  static void logCAMsg(rtc::LoggingSeverity sev,
                       const char* msg,
                       const char* err);

  static OSStatus objectListenerProc(
      AudioObjectID objectId,
      UInt32 numberAddresses,
      const AudioObjectPropertyAddress addresses[],
      void* clientData);

  OSStatus implObjectListenerProc(AudioObjectID objectId,
                                  UInt32 numberAddresses,
                                  const AudioObjectPropertyAddress addresses[]);

  int32_t HandleDeviceChange();
  int32_t HandleStreamFormatChange(AudioObjectID objectId,
                                   AudioObjectPropertyAddress propertyAddress);
  int32_t HandleDataSourceChange(AudioObjectID objectId,
                                 AudioObjectPropertyAddress propertyAddress);
  int32_t HandleProcessorOverload(AudioObjectPropertyAddress propertyAddress);

  int32_t InitMicrophoneLocked() RTC_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  int32_t InitDevice(uint16_t userDeviceIndex,
                     AudioDeviceID& deviceId,
                     bool isInput);

  int32_t GetNumberDevices(AudioObjectPropertyScope scope,
                           AudioDeviceID scopedDeviceIds[],
                           uint32_t deviceListLength);

  int32_t InitRecording() RTC_LOCKS_EXCLUDED(mutex_);

  bool CaptureWorkerThread();

  bool SpeakerIsInitialized() const;

  static OSStatus inDeviceIOProc(AudioDeviceID device,
                                 const AudioTimeStamp* now,
                                 const AudioBufferList* inputData,
                                 const AudioTimeStamp* inputTime,
                                 AudioBufferList* outputData,
                                 const AudioTimeStamp* outputTime,
                                 void* clientData);

  OSStatus implInDeviceIOProc(const AudioBufferList* inputData,
                              const AudioTimeStamp* inputTime)
      RTC_LOCKS_EXCLUDED(mutex_);

  static OSStatus deviceIOProc(AudioDeviceID device,
                               const AudioTimeStamp* now,
                               const AudioBufferList* inputData,
                               const AudioTimeStamp* inputTime,
                               AudioBufferList* outputData,
                               const AudioTimeStamp* outputTime,
                               void* clientData);


  OSStatus implDeviceIOProc(const AudioBufferList* inputData,
                            const AudioTimeStamp* inputTime,
                            AudioBufferList* outputData,
                            const AudioTimeStamp* outputTime)
      RTC_LOCKS_EXCLUDED(mutex_);

  static OSStatus outConverterProc(
      AudioConverterRef audioConverter,
      UInt32* numberDataPackets,
      AudioBufferList* data,
      AudioStreamPacketDescription** dataPacketDescription,
      void* userData);

  OSStatus implOutConverterProc(UInt32* numberDataPackets,
                                AudioBufferList* data);

  int32_t SpeakerIsAvailableLocked(bool& available)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  int32_t InitSpeakerLocked() RTC_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  static OSStatus inConverterProc(
      AudioConverterRef audioConverter,
      UInt32* numberDataPackets,
      AudioBufferList* data,
      AudioStreamPacketDescription** dataPacketDescription,
      void* inUserData);

  OSStatus implInConverterProc(UInt32* numberDataPackets,
                               AudioBufferList* data);

  bool KeyPressed();

  // Always work with our preferred playout format inside VoE.
  // Then convert the output to the OS setting using an AudioConverter.
  OSStatus SetDesiredPlayoutFormat();

  webrtc::AudioDeviceBuffer* _ptrAudioBuffer;
  webrtc::Mutex mutex_;
  rtc::Event _stopEventRec;
  rtc::Event _stopEvent;
  // Only valid/running between calls to StartRecording and StopRecording.
  rtc::PlatformThread capture_worker_thread_;
  // Only valid/running between calls to StartPlayout and StopPlayout.
  rtc::PlatformThread render_worker_thread_;
  webrtc::AudioMixerManagerMac _mixerManager;
  uint16_t _inputDeviceIndex;
  uint16_t _outputDeviceIndex;
  AudioDeviceID _inputDeviceID;
  AudioDeviceID _outputDeviceID;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
  AudioDeviceIOProcID _inDeviceIOProcID;
  AudioDeviceIOProcID _deviceIOProcID;
#endif
  bool _inputDeviceIsSpecified;
  bool _outputDeviceIsSpecified;
  uint8_t _recChannels;
  uint8_t _playChannels;
  Float32* _captureBufData;
  SInt16* _renderBufData;
  SInt16 _renderConvertData[webrtc::PLAY_BUF_SIZE_IN_SAMPLES];
  bool _initialized;
  bool _isShutDown;
  bool _recording;
  bool _playing;
  bool _recIsInitialized;
  bool _playIsInitialized;
  // Atomically set varaibles
  std::atomic<int32_t> _renderDeviceIsAlive;
  std::atomic<int32_t> _captureDeviceIsAlive;
  bool _twoDevices;
  bool _doStop;  // For play if not shared device or play+rec if shared device
  bool _doStopRec;  // For rec if not shared device
  bool _macBookPro;
  bool _macBookProPanRight;
  AudioConverterRef _captureConverter;
  AudioConverterRef _renderConverter;
  AudioStreamBasicDescription _outStreamFormat;
  AudioStreamBasicDescription _outDesiredFormat;
  AudioStreamBasicDescription _inStreamFormat;
  AudioStreamBasicDescription _inDesiredFormat;
  uint32_t _captureLatencyUs;
  uint32_t _renderLatencyUs;
  // Atomically set variables
  mutable std::atomic<int32_t> _captureDelayUs;
  mutable std::atomic<int32_t> _renderDelayUs;
  int32_t _renderDelayOffsetSamples;
  PaUtilRingBuffer* _paCaptureBuffer;
  PaUtilRingBuffer* _paRenderBuffer;
  semaphore_t _renderSemaphore;
  semaphore_t _captureSemaphore;
  int _captureBufSizeSamples;
  int _renderBufSizeSamples;
  // Typing detection
  // 0x5c is key "9", after that comes function keys.
  bool prev_key_state_[0x5d];
};