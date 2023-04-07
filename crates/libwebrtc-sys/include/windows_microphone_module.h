#pragma once

WEBRTC_WINDOWS_CORE_AUDIO_BUILD

#include "modules/audio_device/win/audio_device_core_win.h"
#include "custom_audio.h"

namespace webrtc {
class MicrophoneModule;

class MicrophoneSource : public AudioSource {
  public:
  MicrophoneSource(MicrophoneModule* module);

  private:
  ~MicrophoneSource();
  MicrophoneModule* module;
  static int sources_num;
};


class MicrophoneModule {
 public:

  int32_t Init();
  int32_t SetRecordingDevice(int index);
  int32_t InitMicrophone();
  int32_t MicrophoneIsInitialized();
  int32_t MicrophoneVolumeIsAvailable(bool* available);
  int32_t SetMicrophoneVolume(int volume);
  int32_t MicrophoneVolume(int* volume);
  int32_t MaxMicrophoneVolume(int* maxVolume);
  int32_t MinMicrophoneVolume(int* minVolume);
  int32_t MicrophoneMuteIsAvailable(bool* available);
  int32_t SetMicrophoneMute(bool enable);
  int32_t MicrophoneMute(bool* enabled);
  int32_t RecordingChannels();
  rtc::scoped_refptr<AudioSource> CreateSource();

  MicrophoneModule(webrtc::AudioDeviceBuffer* buffer);
  ~MicrophoneModule();

  public:
  
  webrtc::ScopedCOMInitializer _comInit;
  webrtc::AudioDeviceBuffer* _ptrAudioBuffer;
  mutable webrtc::Mutex mutex_;
  mutable webrtc::Mutex volume_mutex_ RTC_ACQUIRED_AFTER(mutex_);

  IMMDeviceEnumerator* _ptrEnumerator;
  IMMDeviceCollection* _ptrRenderCollection;
  IMMDeviceCollection* _ptrCaptureCollection;
  IMMDevice* _ptrDeviceOut;
  IMMDevice* _ptrDeviceIn;

  IAudioClient* _ptrClientOut;
  IAudioClient* _ptrClientIn;
  IAudioRenderClient* _ptrRenderClient;
  IAudioCaptureClient* _ptrCaptureClient;
  IAudioEndpointVolume* _ptrCaptureVolume;
  ISimpleAudioVolume* _ptrRenderSimpleVolume;

  // DirectX Media Object (DMO) for the built-in AEC.
  rtc::scoped_refptr<IMediaObject> _dmo;
  rtc::scoped_refptr<IMediaBuffer> _mediaBuffer;
  bool _builtInAecEnabled;

  HANDLE _hRenderSamplesReadyEvent;
  HANDLE _hPlayThread;
  HANDLE _hRenderStartedEvent;
  HANDLE _hShutdownRenderEvent;

  HANDLE _hCaptureSamplesReadyEvent;
  HANDLE _hRecThread;
  HANDLE _hCaptureStartedEvent;
  HANDLE _hShutdownCaptureEvent;

  HANDLE _hMmTask;

  UINT _playAudioFrameSize;
  uint32_t _playSampleRate;
  uint32_t _devicePlaySampleRate;
  uint32_t _playBlockSize;
  uint32_t _devicePlayBlockSize;
  uint32_t _playChannels;
  uint32_t _sndCardPlayDelay;
  UINT64 _writtenSamples;
  UINT64 _readSamples;

  UINT _recAudioFrameSize;
  uint32_t _recSampleRate;
  uint32_t _recBlockSize;
  uint32_t _recChannels;

  uint16_t _recChannelsPrioList[3];
  uint16_t _playChannelsPrioList[2];

  LARGE_INTEGER _perfCounterFreq;
  double _perfCounterFactor;

  bool _initialized;
  bool _recording;
  bool _playing;
  bool _recIsInitialized;
  bool _playIsInitialized;
  bool _speakerIsInitialized;
  bool _microphoneIsInitialized;

  bool _usingInputDeviceIndex;
  bool _usingOutputDeviceIndex;
  webrtc::AudioDeviceModule::WindowsDeviceType _inputDevice;
  webrtc::AudioDeviceModule::WindowsDeviceType _outputDevice;
  uint16_t _inputDeviceIndex;
  uint16_t _outputDeviceIndex;

  private:
  rtc::scoped_refptr<MicrophoneSource> source = nullptr;
  webrtc::AudioDeviceBuffer* cb = nullptr;
};
}